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

expty transRecord(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab);
expty transOp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab);
expty transCall(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab);
expty transSeq(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab);
expty transAssign(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab);
expty transIf(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab);
expty transWhile(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab);
expty transWhile(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab);
expty transFor(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab);
expty transLet(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab);
expty transArray(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab);
Ty_ty transRecordTy(S_table tenv, A_ty a);
Tr_exp transFunDec(S_table venv, S_table tenv, A_dec d, Tr_level l, Temp_label lab);
Tr_exp transVarDec(S_table venv, S_table tenv, A_dec d, Tr_level l, Temp_label lab);
Tr_exp transTypeDec(S_table venv, S_table tenv, A_dec d);
Ty_tyList makeFormals(S_table tenv, A_fieldList params);
U_boolList makeBoolList(A_fieldList params);

//In Lab4, the first argument exp should always be **NULL**.
//in lab5, add the the first argument exp
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
	//printf("transTy\n");
	switch (a->kind) {
		case A_nameTy:{
			Ty_ty tmpTy = S_look(tenv, a->u.name);
			if(!tmpTy)
				EM_error(a->pos, "illegal type cycle");
			return Ty_Name(a->u.name, tmpTy);
		}
		case A_recordTy:
			return transRecordTy(tenv, a);
		case A_arrayTy:{
			Ty_ty arrayTy = S_look(tenv, a->u.array);
			//printf("transTy arrayTy: %x\n", arrayTy);
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

//lab5: label for break, jump out loop
expty transExp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab)
{
	//if(!a) return expTy(NULL, Ty_Int());
	//printf("transExp: kind: %d\n", a->kind);

	switch (a->kind) {
		case A_varExp:
			return transVar(venv, tenv, a->u.var, l, lab);
		case A_nilExp:
			return expTy(Tr_nilExp(), Ty_Nil());
		case A_intExp:
			return expTy(Tr_intExp(a->u.intt), Ty_Int());
		case A_stringExp:
			return expTy(Tr_stringExp(a->u.stringg), Ty_String());
		case A_callExp:
			return transCall(venv, tenv, a, l, lab);
		case A_opExp:
			return transOp(venv, tenv, a, l, lab);
		case A_recordExp:
		 	return transRecord(venv, tenv, a, l, lab);
		case A_seqExp:
			return transSeq(venv, tenv, a, l, lab);
		case A_assignExp:
			return transAssign(venv, tenv, a, l, lab);
		case A_ifExp:
			return transIf(venv, tenv, a, l, lab);
		case A_whileExp:
			return transWhile(venv, tenv, a, l, lab);
		case A_forExp:
			return transFor(venv, tenv, a, l, lab);
		case A_breakExp:
			return expTy(Tr_breakExp(lab), Ty_Void());
		case A_letExp:
			return transLet(venv, tenv, a, l, lab);
		case A_arrayExp:
			return transArray(venv, tenv, a, l, lab);
		default:{
			EM_error(a->pos, "strange exp type %d", a->kind);
			return expTy(NULL, Ty_Int());
		}
	}
}

//type-id[exp1] of exp2
expty transArray(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab)
{
	//printf("transArray");
	Ty_ty arrayTy = S_look(tenv, a->u.array.typ);
	//printf("transArray actual_ty(actual_ty(arrayTy)->u.array)): %x\n", actual_ty(actual_ty(arrayTy)->u.array));
	expty sizeTy = transExp(venv, tenv, a->u.array.size, l, lab);
	expty initTy = transExp(venv, tenv, a->u.array.init, l, lab);
	if(actual_ty(arrayTy)->kind != Ty_array)
		EM_error(a->pos, "not a array type");

	if(actual_ty(sizeTy.ty)->kind != Ty_int){
		EM_error(a->u.array.size->pos, "type mismatch1");
		return expTy(NULL, Ty_Int());
	}

	if(actual_ty(initTy.ty)->kind != Ty_int){
		EM_error(a->u.array.init->pos, "type mismatch2");
		return expTy(NULL, Ty_Int());
	}

	Tr_exp trExp = Tr_arrayExp(sizeTy.exp, initTy.exp);
	return expTy(trExp, arrayTy);
}

expty transLet(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab)
{
	//printf("transLet\n");
	A_decList letDecs = a->u.let.decs;

	S_beginScope(tenv); S_beginScope(venv);
	while(letDecs){
		transDec(venv, tenv, letDecs->head, l, lab);
		letDecs = letDecs->tail;
	}
	expty result = transExp(venv, tenv, a->u.let.body, l, lab);
	S_endScope(venv); S_endScope(tenv);
	return result;
}

//break? done?
expty transFor(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab)
{
	expty forLoTy = transExp(venv, tenv, a->u.forr.lo, l, lab);
	expty forHiTy = transExp(venv, tenv, a->u.forr.hi, l, lab);

	S_beginScope(venv);
	Temp_label done = Temp_newlabel();

	Tr_access acc = Tr_allocLocal(l, a->u.forr.escape);
	E_enventry e = E_ROVarEntry(acc, Ty_Int());
	S_enter(venv, a->u.forr.var, e);
	expty forBodyTy = transExp(venv, tenv, a->u.forr.body, l, done);

	if(actual_ty(forLoTy.ty)->kind != Ty_int || actual_ty(forHiTy.ty)->kind != Ty_int)
		EM_error(a->pos, "for exp's range type is not integer\nloop variable can't be assigned");


	S_endScope(venv);

	Tr_exp trExp = Tr_forExp(forLoTy.exp, forHiTy.exp, forBodyTy.exp, done);
	return expTy(trExp, Ty_Void());
}

//while body no value
// break? done?
expty transWhile(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab)
{
	A_exp whileTest = a->u.whilee.test;
	A_exp whileBody = a->u.whilee.body;

	Temp_label done = Temp_newlabel();

	expty whileBodyTy = transExp(venv, tenv, whileBody, l, done);
	expty whileTestTy = transExp(venv, tenv, whileTest, l, lab);
	if(whileBodyTy.ty->kind != Ty_void){
		EM_error(a->pos, "while body must produce no value");
		return expTy(Tr_err(), Ty_Int());
	}

	Tr_exp trExp = Tr_whileExp(whileTestTy.exp, whileBodyTy.exp, done);

	return expTy(trExp, Ty_Void());
}

//then type must be same as else type
expty transIf(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab)
{
	//printf("transIf\n");
	A_exp ifTest = a->u.iff.test;
	A_exp ifThen = a->u.iff.then;
	A_exp ifElse = a->u.iff.elsee;

	expty ifTestTy = transExp(venv, tenv, ifTest, l, lab);
	expty ifThenTy = transExp(venv, tenv, ifThen, l, lab);
	expty ifElseTy = transExp(venv, tenv, ifElse, l, lab);

	if(actual_ty(ifElseTy.ty)->kind == Ty_nil){
		//printf("else ty_nil\n");
		//EM_error(a->pos, "if-then exp's body must produce no value");
		return expTy(Tr_ifExp(ifTestTy.exp, ifThenTy.exp, NULL), ifThenTy.ty);
	}
	else{
		if(actual_ty(ifElseTy.ty) != actual_ty(ifThenTy.ty)){
			EM_error(a->pos, "then exp and else exp type mismatch3");
			return expTy(NULL, Ty_Int());
		}
	}

	Tr_exp trExp = Tr_ifExp(ifTestTy.exp, ifThenTy.exp, ifElseTy.exp);
	return expTy(trExp, ifElseTy.ty);
}

expty transAssign(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab)
{
	//printf("transAssign: \n");
	A_var assignVar = a->u.assign.var;
	A_exp assignExp = a->u.assign.exp;

	expty assignExpTy = transExp(venv, tenv, assignExp, l, lab);
	expty assignVarTy = transVar(venv, tenv, assignVar, l, lab);

	if(actual_ty(assignExpTy.ty) != actual_ty(assignVarTy.ty)){
		EM_error(a->pos, "unmatched assign exp");
		return expTy(NULL, Ty_Int());
	}

	Tr_exp trExp = Tr_assignExp(assignVarTy.exp, assignExpTy.exp);
	return expTy(trExp, Ty_Void());
}

//return type of the last exp
expty transSeq(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab)
{
	//printf("transSeq\n");
	A_expList seqExp = a->u.seq;
	if(!seqExp) {

		return expTy(Tr_nilExp(), Ty_Void());
	}

	expty result;
	while(seqExp){
		result = transExp(venv, tenv, seqExp->head, l, lab);
		seqExp = seqExp->tail;
	}

	return result;
}
//check whether the type of each field of record is same as tyfields
// type-id{id=exp1,id=exp2....}
expty transRecord(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab)
{
	//printf("transRecord\n");
	S_symbol recordSym = a->u.record.typ;
	A_efieldList recordFields = a->u.record.fields;

	Ty_ty originTy = S_look(tenv, recordSym);

	if(!originTy){
		EM_error(a->pos, "undefined type rectype");
		return expTy(NULL, Ty_Int());
	}

	//Ty_ty originTy = x->u.var.ty;
	if(actual_ty(originTy)->kind != Ty_record)
		EM_error(a->pos, "not a record");

	Ty_fieldList originFields = actual_ty(originTy)->u.record;

	Tr_expList trFields = NULL;//for translate
	Ty_field tmpTyField = NULL;
	A_efield tmpEfield = NULL;
	while(recordFields && originFields){
		tmpEfield = recordFields->head;
		tmpTyField = originFields->head;

		//name must be same
		if(tmpEfield->name != tmpTyField->name)
			EM_error(a->pos, "field %s doesn't exist", S_name(tmpEfield->name));

		expty tmpExpTy = transExp(venv, tenv, tmpEfield->exp, l, lab);
		trFields = Tr_ExpList(tmpExpTy.exp, trFields);

		if(actual_ty(tmpExpTy.ty) != actual_ty(tmpTyField->ty))
			EM_error(a->pos, "record type not match");

		recordFields = recordFields->tail;
		originFields = originFields->tail;
	}

	if(originFields || recordFields)
		EM_error(a->pos, "the number of field not match");

	Tr_exp trExp = Tr_recordExp(trFields);
	return expTy(trExp, originTy);
}

//the type of L$R must be int when the op is +-*/
//else the type of L$R must be the same;
expty transOp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab)
{
	//printf("transOp\n");
	A_exp leftExp = a->u.op.left;
	A_exp rightExp = a->u.op.right;
	A_oper oper = a->u.op.oper;

	Tr_exp trExp;
	expty leftExpTy = transExp(venv, tenv, leftExp, l, lab);
	expty rightExpTy = transExp(venv, tenv, rightExp, l, lab);
	if(oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper == A_divideOp){
		if(actual_ty(leftExpTy.ty)->kind != Ty_int){
			EM_error(leftExp->pos, "integer required");
			return expTy(NULL, Ty_Int());
		}

		if(actual_ty(rightExpTy.ty)->kind != Ty_int){
			EM_error(rightExp->pos, "integer required");
			return expTy(NULL, Ty_Int());
		}

		trExp = Tr_arithExp(oper, leftExpTy.exp, rightExpTy.exp);
	}else{
		//printf("leftExpTy: %x\n", actual_ty(leftExpTy.ty));
		//printf("rightExpTy: %x\n", actual_ty(rightExpTy.ty));
		if(actual_ty(leftExpTy.ty) == actual_ty(rightExpTy.ty)
				|| leftExpTy.ty->kind == Ty_nil || rightExpTy.ty->kind == Ty_nil ){
			trExp = Tr_compExp(oper, leftExpTy.exp, rightExpTy.exp);
		}else{
			EM_error(a->pos, "same type required");
			return expTy(Tr_err(), Ty_Int());
		}
	}

	return expTy(trExp, Ty_Int());
}

//check whether the type of args is same as formals
expty transCall(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label lab)
{
	//printf("transCall: %s\n", S_name(a->u.call.func));
	S_symbol funcSym = a->u.call.func;
	A_expList funcArgs = a->u.call.args;

	E_enventry x = S_look(venv, funcSym);
	if (!x || x->kind != E_funEntry) {
		EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));
		return expTy(NULL, Ty_Int());
	}

	Ty_tyList funcFormals = x->u.fun.formals;

	expty tmpExpTy;
	Tr_expList trArgs = NULL;
	while(funcArgs && funcFormals){
		tmpExpTy = transExp(venv, tenv, funcArgs->head, l, lab);
		if(actual_ty(tmpExpTy.ty) != actual_ty(funcFormals->head)){
			EM_error(funcArgs->head->pos, "para type mismatch4");
			return expTy(NULL, Ty_Int());
		}

		funcArgs = funcArgs->tail;
		funcFormals = funcFormals->tail;

		trArgs = Tr_ExpList(tmpExpTy.exp, trArgs);
	}

	if(funcArgs){
		EM_error(a->pos, "too many params in function %s", S_name(funcSym));
		return expTy(NULL, Ty_Int());
	}
	if(funcFormals){
		EM_error(a->pos, "too less params in function %s", S_name(funcSym));
		return expTy(NULL, Ty_Int());
	}

	Tr_exp trExp = Tr_callExp(x->u.fun.label, trArgs, l, x->u.fun.level);

	if(x->u.fun.result)
		return expTy(trExp, x->u.fun.result);
	else
		return expTy(trExp, Ty_Void());
}

expty transVar(S_table venv, S_table tenv, A_var v, Tr_level l, Temp_label lab)
{
	//printf("transVar:\n");
	switch (v->kind) {
		case A_simpleVar:{
			//printf("simpleVar: %s\n", S_name(v->u.simple));
			//find the actual ty of simple var
			E_enventry x = S_look(venv, v->u.simple);
			if(x && x->kind == E_varEntry){
				return expTy(Tr_simpleVar(x->u.var.access, l), actual_ty(x->u.var.ty));
			}
			else{
				EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
				return expTy(Tr_err(), Ty_Int());
			}
		}
		case A_fieldVar:{
			//find the actual type of field.id
			A_var fieldCase = v->u.field.var; //field.id
			S_symbol idSym = v->u.field.sym;
			//printf("A_fieldVar: %s\n", S_name(idSym));

			expty fieldCaseTy = transVar(venv, tenv, fieldCase, l, lab);
			Ty_ty fieldActualTy = actual_ty(fieldCaseTy.ty);

			if(fieldActualTy->kind != Ty_record){
				EM_error(v->pos, "not a record type");
				return expTy(NULL, Ty_Int());
			}

			//find the ty from the field case
			Ty_fieldList fieldList = fieldActualTy->u.record;
			Ty_field tmpField = NULL;
			int count = 0;
			while(fieldList){
				if(fieldList->head->name == idSym){
					tmpField = fieldList->head;
					break;
				}
				count ++;
				fieldList = fieldList->tail;
			}

			//not found
			if(!fieldList){
				EM_error(v->pos, "field %s doesn't exist", S_name(idSym));
				return expTy(NULL, Ty_Int());
			}

			Tr_exp trExp = Tr_fieldVar(fieldCaseTy.exp, count);
			return expTy(trExp, tmpField->ty);
		}
		case A_subscriptVar:{
			//find the actual type of the array
			//the type of exp must be Ty_int and the type of
			A_var arrVar = v->u.subscript.var;
			A_exp arrExp = v->u.subscript.exp;

			expty arrVarTy = transVar(venv, tenv, arrVar, l, lab);
			expty arrExpTy = transExp(venv, tenv, arrExp, l, lab);


			if(actual_ty(arrExpTy.ty)->kind != Ty_int){
				EM_error(v->pos, "wrong index");
				return expTy(NULL, Ty_Int());
			}

			if(actual_ty(arrVarTy.ty)->kind != Ty_array){
				EM_error(v->pos, "array type required");
				return expTy(NULL, Ty_Int());
			}

			Tr_exp trExp = Tr_subscriptVar(arrVarTy.exp, arrExpTy.exp);
			//printf("transVar A_subscriptVar: %x\n", actual_ty(actual_ty(arrVarTy.ty)->u.array));
			return expTy(trExp, actual_ty(arrVarTy.ty)->u.array);
		}
		default:{
			EM_error(v->pos, "strange variable type %d", v->kind);
			return expTy(NULL, Ty_Int());
		}
	}
}

Tr_exp transDec(S_table venv, S_table tenv, A_dec d, Tr_level l, Temp_label lab)
{
	//printf("transDec\n");
	switch (d->kind) {
		case A_functionDec:
			return transFunDec(venv, tenv, d, l, lab);
			break;
		case A_varDec:
			return transVarDec(venv, tenv, d, l, lab);
			break;
		case A_typeDec:
			return transTypeDec(venv, tenv, d);
			break;
		default:
			break;
	}

	assert(0);
}

Tr_exp transTypeDec(S_table venv, S_table tenv, A_dec d)
{
	//printf("transTypeDec\n");

	A_nametyList nameTyList = d->u.type;

	A_nametyList i;
	for(i = nameTyList; i; i = i->tail){

		//existed
		if(S_look(tenv, i->head->name)){
			//EM_error(d->pos, "two types have the same name");
			return Tr_typeDec();
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

		Ty_ty beginTy = S_look(tenv, i->head->name);
		Ty_ty tmpTy = beginTy;

		while(tmpTy->kind == Ty_name){

			tmpTy = tmpTy->u.name.ty;

			if(tmpTy == beginTy){
				EM_error(d->pos, "illegal type cycle");
				beginTy->u.name.ty = Ty_Int();
				break;
			}
		}
	}

	return Tr_typeDec();
}

//var maybe have no type
Tr_exp transVarDec(S_table venv, S_table tenv, A_dec d, Tr_level l, Temp_label lab)
{
	//printf("transVarDec\n");
	expty initTy = transExp(venv, tenv, d->u.var.init, l, lab);
	Tr_access acc = Tr_allocLocal(l, d->u.var.escape);

	if(d->u.var.typ){
		Ty_ty varTy = S_look(tenv, get_vardec_typ(d));

		//check var type and init type
		if(actual_ty(varTy) != actual_ty(initTy.ty)){
			EM_error(d->pos, "type mismatch5");
			return Tr_typeDec();
		}

		S_enter(venv, d->u.var.var, E_VarEntry(acc, varTy));
	}else{

		if(initTy.ty){
			//printf("no type var trans\n");
			if(actual_ty(initTy.ty)->kind != Ty_nil){
				/*if(actual_ty(initTy.ty)->kind == Ty_array){
					printf("actual_ty(initTy.ty)->u.array: %x\n", actual_ty(actual_ty(initTy.ty)->u.array));
				}*/
				S_enter(venv, d->u.var.var, E_VarEntry(acc, initTy.ty));
			}
			else{
				EM_error(d->pos, "init should not be nil without type specified");
				return Tr_typeDec();
			}
		}else{
			S_enter(venv, d->u.var.var, E_VarEntry(acc, Ty_Void()));
		}

	}

	return Tr_varDec(acc, initTy.exp);
}

//check params and func body
//type of func body must be same as result
Tr_exp transFunDec(S_table venv, S_table tenv, A_dec d, Tr_level l, Temp_label lab)
{
	//printf("transFunDec\n");
	A_fundecList funcList = d->u.function;

	A_fundec tmpFun;
	while(funcList){
		tmpFun = funcList->head;

		if(S_look(venv, tmpFun->name)){
			EM_error(d->pos, "two functions have the same name");
			return Tr_typeDec();
		}
		Ty_tyList funFormals = makeFormals(tenv, tmpFun->params);

		Ty_ty funResult;
		if(tmpFun->result){
			funResult = S_look(tenv, tmpFun->result); //fun result type

			//check result type
			if(!funResult){
				EM_error(tmpFun->pos, "undefined result type %s", S_name(tmpFun->result));
				return Tr_typeDec();
			}

		}else{
			funResult = Ty_Void();
		}

		//init func
		Temp_label funLab = Temp_newlabel();
		U_boolList escapeList = makeBoolList(tmpFun->params);
		Tr_level funLevel = Tr_newLevel(l, funLab, escapeList);

		S_enter(venv, tmpFun->name, E_FunEntry(funLevel, funLab, funFormals, funResult));

		//next function
		funcList = funcList->tail;
	}

	//check func body
	funcList = d->u.function;
	while(funcList){
		//printf("funcList\n");
		tmpFun = funcList->head;

		E_enventry x = S_look(venv, tmpFun->name);

		Ty_tyList tmpFormals = x->u.fun.formals;

		S_beginScope(tenv);
		S_beginScope(venv);

		Ty_tyList i;
		A_fieldList j;

		Tr_accessList accList = Tr_formals(x->u.fun.level);
		//add the params to env
		for(i = tmpFormals, j = tmpFun->params; i; i = i->tail, j = j->tail){
			S_enter(venv, j->head->name, E_VarEntry(accList->head, i->head));
			accList = accList->tail;
		}

		//check body
		expty tmpBodyTy = transExp(venv, tenv, tmpFun->body, x->u.fun.level, lab);

		S_endScope(tenv);
		S_endScope(venv);
		if(tmpFun->result){
			if(actual_ty(tmpBodyTy.ty) != actual_ty(x->u.fun.result)){
				EM_error(tmpFun->body->pos, "false return type");
				return Tr_typeDec();
			}
		}else{//no type result

			if(actual_ty(tmpBodyTy.ty)->kind != Ty_void){
				EM_error(tmpFun->body->pos, "procedure returns value");
				return Tr_typeDec();
			}

		}

		Tr_procEntryExit(x->u.fun.level, tmpBodyTy.exp);
		//next function
		funcList = funcList->tail;
	}

	return Tr_typeDec();
}

U_boolList makeBoolList(A_fieldList params)
{
	if(!params)
		return NULL;

	return U_BoolList(params->head->escape, makeBoolList(params->tail));
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

F_fragList SEM_transProg(A_exp exp)
{
	//printf("SEM_transProg\n");

	S_table venv = E_base_venv();
	S_table tenv = E_base_tenv();

	Temp_label startLab = Temp_newlabel();

	Tr_level OMLevel = Tr_outermost();

	//printf("%d\n", OMLevel->frame->stackOff);
	expty tranExp = transExp(venv, tenv, exp, OMLevel, startLab);
	Tr_procEntryExit(OMLevel, tranExp.exp);

	return Tr_getResult();
}
