#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "helper.h"
#include "env.h"
#include "semant.h"

/*Lab4: Your implementation of lab4*/

expty transRecord(S_table venv, S_table tenv, A_exp a);
expty transOp(S_table venv, S_table tenv, A_exp a);
expty transCall(S_table venv, S_table tenv, A_exp a);
expty transSeq(S_table venv, S_table tenv, A_exp a);
expty transAssign(S_table venv, S_table tenv, A_exp a);
expty transIf(S_table venv, S_table tenv, A_exp a);
expty transWhile(S_table venv, S_table tenv, A_exp a);
expty transWhile(S_table venv, S_table tenv, A_exp a);
expty transFor(S_table venv, S_table tenv, A_exp a);
expty transLet(S_table venv, S_table tenv, A_exp a);
expty transArray(S_table venv, S_table tenv, A_exp a);
Ty_ty transRecordTy(S_table tenv, A_ty a);
void transFunDec(S_table venv, S_table tenv, A_dec d);
void transVarDec(S_table venv, S_table tenv, A_dec d);
void transTypeDec(S_table venv, S_table tenv, A_dec d);
Ty_tyList makeFormals(S_table tenv, A_fieldList params);

//In Lab4, the first argument exp should always be **NULL**.
expty expTy(Tr_exp exp, Ty_ty ty)
{
	expty e;

	e.exp = exp;
	e.ty = ty;

	return e;
}

Ty_ty actual_ty(Ty_ty ty)
{
	Ty_ty result = ty;
	while(result->kind == Ty_name)
		result = result->u.name.ty;
	return result;
}

Ty_ty transTy(S_table tenv, A_ty a)
{
	printf("transTy\n");
	switch (a->kind) {
		case A_nameTy:{
			Ty_ty tmpTy = S_look(tenv, a->u.name);
			if(!tmpTy)
				EM_error(a->pos, "illegal type cycle");
			printf("nameTY\n");
			return Ty_Name(a->u.name, tmpTy);
		}
		case A_recordTy:
			return transRecordTy(tenv, a);
		case A_arrayTy:{
			Ty_ty arrayTy = S_look(tenv, a->u.array);
			if(!arrayTy)
				EM_error(a->pos, "not found type");

			return Ty_Array(arrayTy);
		}
		default:{
			EM_error(a->pos, "undefined type1");
			return Ty_Int();
		}
	}
}

Ty_ty transRecordTy(S_table tenv, A_ty a)
{
	A_fieldList recordFields = a->u.record;
	Ty_fieldList tyList = Ty_FieldList(NULL, NULL);//trans A to TY
	Ty_fieldList tmpTyFieldList = tyList;

	A_fieldList i;
	for(i = recordFields; i; i = i->tail){
		Ty_fieldList tmpTail = Ty_FieldList(NULL, NULL);//alloc tail space

		Ty_ty tmpTy = S_look(tenv, i->head->typ);

		if(!tmpTy){
			EM_error(i->head->pos, "undefined type %s", S_name(i->head->typ));
			return Ty_Int();
		}

		tmpTyFieldList->head = Ty_Field(i->head->name, tmpTy);

		if(i->tail){
			tmpTyFieldList->tail = tmpTail;
		}else{
			tmpTyFieldList->tail = NULL;
		}


		//next ty fieldlist
		tmpTyFieldList = tmpTail;
	}

	return Ty_Record(tyList);
}

expty transExp(S_table venv, S_table tenv, A_exp a)
{
	switch (a->kind) {
		case A_varExp:
			return transVar(venv, tenv, a->u.var);
		case A_nilExp:
			return expTy(NULL, Ty_Nil());
		case A_intExp:
			return expTy(NULL, Ty_Int());
		case A_stringExp:
			return expTy(NULL, Ty_String());
		case A_callExp:
			return transCall(venv, tenv, a);
		case A_opExp:
			return transOp(venv, tenv, a);
		case A_recordExp:
		 	return transRecord(venv, tenv, a);
		case A_seqExp:
			return transSeq(venv, tenv, a);
		case A_assignExp:
			return transAssign(venv, tenv, a);
		case A_ifExp:
			return transIf(venv, tenv, a);
		case A_whileExp:
			return transWhile(venv, tenv, a);
		case A_forExp:
			return transFor(venv, tenv, a);
		case A_breakExp:
			return expTy(NULL, Ty_Void());
		case A_letExp:
			return transLet(venv, tenv, a);
		case A_arrayExp:
			return transArray(venv, tenv, a);
		default:{
			EM_error(a->pos, "strange exp type %d", a->kind);
			return expTy(NULL, Ty_Int());
		}
	}
}

//type-id[exp1] of exp2
expty transArray(S_table venv, S_table tenv, A_exp a)
{
	printf("transArray");
	Ty_ty arrayTy = S_look(tenv, a->u.array.typ);

	expty sizeTy = transExp(venv, tenv, a->u.array.size);
	expty initTy = transExp(venv, tenv, a->u.array.init);
	if(actual_ty(arrayTy)->kind != Ty_array)
		EM_error(a->pos, "not a array type");

	if(actual_ty(sizeTy.ty)->kind != Ty_int){
		EM_error(a->u.array.size->pos, "type mismatch");
		return expTy(NULL, Ty_Int());
	}

	if(actual_ty(initTy.ty)->kind != Ty_int){
		EM_error(a->u.array.init->pos, "type mismatch");
		return expTy(NULL, Ty_Int());
	}
	return expTy(NULL, Ty_Array(arrayTy));
}

expty transLet(S_table venv, S_table tenv, A_exp a)
{
	printf("transLet");
	A_decList letDecs = a->u.let.decs;

	S_beginScope(tenv); S_beginScope(venv);
	while(letDecs){
		transDec(venv, tenv, letDecs->head);
		letDecs = letDecs->tail;
	}

	expty result = transExp(venv, tenv, a->u.let.body);
	S_endScope(venv); S_endScope(tenv);
	return result;
}

expty transFor(S_table venv, S_table tenv, A_exp a)
{
	S_beginScope(venv);

	expty forLoTy = transExp(venv, tenv, a->u.forr.lo);
	expty forHiTy = transExp(venv, tenv, a->u.forr.hi);

	if(actual_ty(forLoTy.ty)->kind != Ty_int || actual_ty(forHiTy.ty)->kind != Ty_int)
		EM_error(a->pos, "for exp's range type is not integer\nloop variable can't be assigned");

	S_enter(venv, a->u.forr.var, forLoTy.ty);

	S_endScope(venv);

	return expTy(NULL, Ty_Void());
}

//while body no value
expty transWhile(S_table venv, S_table tenv, A_exp a)
{
	A_exp whileTest = a->u.whilee.test;
	A_exp whileBody = a->u.whilee.body;

	expty whileBodyTy = transExp(venv, tenv, whileBody);
	if(whileBodyTy.ty->kind != Ty_void)
		EM_error(a->pos, "while body must produce no value");

	return expTy(NULL, Ty_Void());
}

//then type must be same as else type
expty transIf(S_table venv, S_table tenv, A_exp a)
{
	printf("transIf\n");
	A_exp ifTest = a->u.iff.test;
	A_exp ifThen = a->u.iff.then;
	A_exp ifElse = a->u.iff.elsee;

	expty ifTestTy = transExp(venv, tenv, ifTest);
	expty ifElseTy = transExp(venv, tenv, ifElse);
	expty ifThenTy = transExp(venv, tenv, ifThen);
	if(actual_ty(ifElseTy.ty) != actual_ty(ifThenTy.ty))
		EM_error(a->pos, "if-then exp's body must produce no value");

	return expTy(NULL, ifElseTy.ty);
}

expty transAssign(S_table venv, S_table tenv, A_exp a)
{
	A_var assignVar = a->u.assign.var;
	A_exp assignExp = a->u.assign.exp;

	expty assignExpTy = transExp(venv, tenv, assignExp);
	expty assignVarTy = transVar(venv, tenv, assignVar);

	if(actual_ty(assignExpTy.ty) != actual_ty(assignVarTy.ty)){
		EM_error(a->pos, "unmatched assign exp");
		return expTy(NULL, Ty_Int());
	}

	return expTy(NULL, assignVarTy.ty);
}

//return type of the last exp
expty transSeq(S_table venv, S_table tenv, A_exp a)
{
	A_expList seqExp = a->u.seq;

	expty result;
	while(seqExp){
		result = transExp(venv, tenv, seqExp->head);
		seqExp = seqExp->tail;
	}

	return result;
}
//check whether the type of each field of record is same as tyfields
expty transRecord(S_table venv, S_table tenv, A_exp a)
{
	printf("transRecord\n");
	S_symbol recordSym = a->u.record.typ;
	A_efieldList recordFields = a->u.record.fields;
	//printf("%s\n", S_name(recordSym));

	Ty_ty originTy = S_look(tenv, recordSym);

	if(!originTy){
		EM_error(a->pos, "undefined type rectype");
		return expTy(NULL, Ty_Int());
	}

	//Ty_ty originTy = x->u.var.ty;
	if(actual_ty(originTy)->kind != Ty_record)
		EM_error(a->pos, "not a record");

	Ty_fieldList originFields = actual_ty(originTy)->u.record;

	Ty_field tmpTyField = NULL;
	A_efield tmpEfield = NULL;
	while(recordFields && originFields){
		tmpEfield = recordFields->head;
		tmpTyField = originFields->head;

		//name must be same
		//printf("%s\n", S_name(tmpEfield->name));
		//printf("%s\n", S_name(tmpTyField->name));

		if(tmpEfield->name != tmpTyField->name)
			EM_error(a->pos, "field %s doesn't exist", S_name(tmpEfield->name));
		fflush(stdout);
		expty tmpExpTy = transExp(venv, tenv, tmpEfield->exp);

		if(actual_ty(tmpExpTy.ty) != actual_ty(tmpTyField->ty))
			EM_error(a->pos, "record type not match");

		recordFields = recordFields->tail;
		originFields = originFields->tail;
	}

	if(originFields || recordFields)
		EM_error(a->pos, "the number of field not match");

	return expTy(NULL, originTy);
}

//the type of L$R must be int when the op is +-*/
//else the type of L$R must be the same;
expty transOp(S_table venv, S_table tenv, A_exp a)
{
	printf("transOp\n");
	A_exp leftExp = a->u.op.left;
	A_exp rightExp = a->u.op.right;
	A_oper oper = a->u.op.oper;

	expty leftExpTy = transExp(venv, tenv, leftExp);
	expty rightExpTy = transExp(venv, tenv, rightExp);
	if(oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper == A_divideOp){
		if(actual_ty(leftExpTy.ty)->kind != Ty_int)
			EM_error(leftExp->pos, "integer required");
		if(actual_ty(rightExpTy.ty)->kind != Ty_int)
			EM_error(rightExp->pos, "integer required");
	}else{
		//Ty_print(actual_ty(leftExpTy.ty));
		//Ty_print(actual_ty(rightExpTy.ty));
		if(actual_ty(leftExpTy.ty) != actual_ty(rightExpTy.ty))
			EM_error(a->pos, "same type required");
	}

	return expTy(NULL, Ty_Int());
}

//check whether the type of args is same as formals
expty transCall(S_table venv, S_table tenv, A_exp a)
{
	printf("transCall\n");
	S_symbol funcSym = a->u.call.func;
	A_expList funcArgs = a->u.call.args;

	E_enventry x = S_look(venv, funcSym);
	if (!x || x->kind != E_funEntry) {
		//printf("fun sym:%s\n", S_name(funcSym));
		EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));
		return expTy(NULL, Ty_Int());
	}

	Ty_tyList funcFormals = x->u.fun.formals;

	expty tmpExpTy;
	while(funcArgs && funcFormals){
		tmpExpTy = transExp(venv, tenv, funcArgs->head);
		if(actual_ty(tmpExpTy.ty) != actual_ty(funcFormals->head)){
			EM_error(funcArgs->head->pos, "para type mismatch");
			return expTy(NULL, Ty_Int());
		}

		funcArgs = funcArgs->tail;
		funcFormals = funcFormals->tail;
	}

	if(funcArgs){
		EM_error(a->pos, "too many params in function %s", S_name(funcSym));
		return expTy(NULL, Ty_Int());
	}
	if(funcFormals){
		EM_error(a->pos, "too less params in function %s", S_name(funcSym));
		return expTy(NULL, Ty_Int());
	}

	return expTy(NULL, x->u.fun.result);
}

expty transVar(S_table venv, S_table tenv, A_var v)
{
	switch (v->kind) {
		case A_simpleVar:{
			//find the actual ty of simple var
			E_enventry x = S_look(venv, v->u.simple);
			if(x && x->kind == E_varEntry){
				return expTy(NULL, actual_ty(x->u.var.ty));
			}
			else{
				EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
				return expTy(NULL, Ty_Int());
			}
		}
		case A_fieldVar:{
			//find the actual type of field.id
			A_var fieldCase = v->u.field.var; //field.id
			S_symbol idSym = v->u.field.sym;

			expty fieldCaseTy = transVar(venv, tenv, fieldCase);
			Ty_ty fieldActualTy = actual_ty(fieldCaseTy.ty);

			if(fieldActualTy->kind != Ty_record){
				EM_error(v->pos, "not a record type");
				return expTy(NULL, Ty_Int());
			}

			//find the ty from the field case
			Ty_fieldList fieldList = fieldActualTy->u.record;
			Ty_field tmpField = NULL;
			while(fieldList){
				if(fieldList->head->name == idSym){
					tmpField = fieldList->head;
					break;
				}
				fieldList = fieldList->tail;
			}

			//not found
			if(!fieldList){
				EM_error(v->pos, "field %s doesn't exist", S_name(idSym));
				return expTy(NULL, Ty_Int());
			}

			return expTy(NULL, tmpField->ty);
		}
		case A_subscriptVar:{
			//find the actual type of the array
			//the type of exp must be Ty_int and the type of
			A_var arrVar = v->u.subscript.var;
			A_exp arrExp = v->u.subscript.exp;

			expty arrExpTy = transExp(venv, tenv, arrExp);
			expty arrVarTy = transVar(venv, tenv, arrVar);

			if(actual_ty(arrExpTy.ty)->kind != Ty_int){
				EM_error(v->pos, "wrong index");
				return expTy(NULL, Ty_Int());
			}

			if(actual_ty(arrVarTy.ty)->kind != Ty_array){
				EM_error(v->pos, "array type required");
				return expTy(NULL, Ty_Int());
			}

			return expTy(NULL, arrVarTy.ty->u.array);
		}
		default:{
			EM_error(v->pos, "strange variable type %d", v->kind);
			return expTy(NULL, Ty_Int());
		}
	}
}

void transDec(S_table venv, S_table tenv, A_dec d)
{
	printf("transDec\n");
	switch (d->kind) {
		case A_functionDec:
			transFunDec(venv, tenv, d);
			break;
		case A_varDec:
			transVarDec(venv, tenv, d);
			break;
		case A_typeDec:
			transTypeDec(venv, tenv, d);
			break;
		default:
			break;
	}

	return;
}

void transTypeDec(S_table venv, S_table tenv, A_dec d)
{
	printf("transTypeDec\n");

	A_nametyList nameTyList = d->u.type;

	A_nametyList i;
	for(i = nameTyList; i; i = i->tail){

		//existed
		if(S_look(tenv, i->head->name)){
			EM_error(d->pos, "two types have the same name");
			return;
		}

		S_enter(tenv, i->head->name, Ty_Name(i->head->name, NULL));
	}

	//second loop  fresh the type
	for(i = nameTyList; i; i = i->tail){
		Ty_ty tmpTy = S_look(tenv, i->head->name);
		//printf("name: %s\n", S_name(i->head->name));
		tmpTy->u.name.ty = transTy(tenv, i->head->ty);
	}

	//detect the illegle cycle
	for(i = nameTyList; i; i = i->tail){
		printf("loop\n");
		Ty_ty beginTy = S_look(tenv, i->head->name);
		Ty_ty tmpTy = beginTy;
		//printf("name :%s\n", S_name(tmpTy->u.name.sym));
		while(tmpTy->kind == Ty_name){
			printf("name :%s\n", S_name(tmpTy->u.name.sym));
			tmpTy = tmpTy->u.name.ty;
			Ty_print(tmpTy);
			if(tmpTy == beginTy){
				EM_error(d->pos, "illegal type cycle");
				beginTy->u.name.ty = Ty_Int();
				break;
			}
		}
	}
}

//var maybe have no type
void transVarDec(S_table venv, S_table tenv, A_dec d)
{
	printf("transVarDec\n");
	//printf("%s\n", S_name(d->u.var.typ));

	expty initTy = transExp(venv, tenv, d->u.var.init);

	if(d->u.var.typ){
		Ty_ty varTy = S_look(tenv, get_vardec_typ(d));

		//check var type and init type
		if(actual_ty(varTy) != actual_ty(initTy.ty)){
			EM_error(d->pos, "type mismatch");
			return;
		}

		S_enter(venv, d->u.var.var, E_VarEntry(varTy));
	}else{
		S_enter(venv, d->u.var.var, E_VarEntry(initTy.ty));
	}
}

//check params and func body
//type of func body must be same as result
void transFunDec(S_table venv, S_table tenv, A_dec d)
{
	printf("transFunDec\n");
	A_fundecList funcList = d->u.function;

	A_fundec tmpFun;
	while(funcList){
		tmpFun = funcList->head;

		Ty_tyList funFormals = makeFormals(tenv, tmpFun->params);

		Ty_ty funResult;
		if(tmpFun->result){
			funResult = S_look(tenv, tmpFun->result); //fun result type

			//check result type
			if(!funResult)
				EM_error(tmpFun->pos, "undefined result type %s", S_name(tmpFun->result));
		}else{
			funResult = Ty_Void();
		}
		//printf("func name%s\n", S_name(tmpFun->name));
		S_enter(venv, tmpFun->name, E_FunEntry(funFormals, funResult));

		//next function
		funcList = funcList->tail;
	}

	//check func body
	funcList = d->u.function;
	while(funcList){
		tmpFun = funcList->head;

		E_enventry x = S_look(venv, tmpFun->name);

		if(!x || x->kind != E_funEntry) printf("check func body: %s\n", S_name(tmpFun->name));
		Ty_tyList tmpFormals = x->u.fun.formals;

		S_beginScope(tenv);
		S_beginScope(venv);

		Ty_tyList i;
		A_fieldList j;

		//add the params to env
		for(i = tmpFormals, j = tmpFun->params; i; i = i->tail, j = j->tail){
			//printf("S_enter: %s\n", S_name(j->head->name));
			S_enter(venv, j->head->name, E_VarEntry(i->head));
			//Ty_print(i->head);
			//E_enventry x = S_look(venv, j->head->name);
			//if(x && x->kind == E_varEntry) {
				//printf("got it\n");
				//Ty_print(x->u.var.ty);
				//}
		}
		//printf("loop\n");

		//check body
		expty tmpBodyTy = transExp(venv, tenv, tmpFun->body);

		if(actual_ty(tmpBodyTy.ty) != actual_ty(x->u.fun.result))
			EM_error(tmpFun->body->pos, "procedure returns value");

		S_endScope(tenv);
		S_endScope(venv);

		//next function
		funcList = funcList->tail;
	}
}

Ty_tyList makeFormals(S_table tenv, A_fieldList params)
{
	if(params == NULL){
		return NULL;
	}else{
		Ty_ty type = S_look(tenv, params->head->typ);
		return Ty_TyList(type, makeFormals(tenv, params->tail));
	}
}

void SEM_transProg(A_exp exp)
{
	printf("SEM_transProg\n");

	S_table venv = E_base_venv();
	S_table tenv = E_base_tenv();

	transExp(venv, tenv, exp);
}
