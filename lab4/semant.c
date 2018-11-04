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
	switch (a->kind) {
		case A_nameTy:{
			S_look(tenv, a->u.name);
		}
		case A_recordTy:
			return transRecordTy(tenv, a);
		case A_arrayTy:
			return transArrayTy(tenv, a);
		default:{
			EM_error(a->pos, "undefined type");
			return Ty_Int();
		}
	}
}

Ty_ty transArrayTy(S_table tenv, A_ty a)
{

}

Ty_ty transRecordTy(S_table tenv, A_ty a)
{

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
		default:
			return NULL;
	}
}

//type-id[exp1] of exp2
expty transArray(S_table venv, S_table tenv, A_exp a)
{
	Ty_ty arrayTy = S_look(tenv, a->u.array.typ);

	expty sizeTy = transExp(venv, tenv, a->u.array.size);
	expty initTy = transExp(venv, tenv, a->u.array.init);
	if(actual_ty(arrayTy)->kind != Ty_array)
		EM_error(a->pos, "not a array type");

	if(actual_ty(sizeTy)->kind != Ty_int)
		EM_error(a->u.array.size.pos, "type not int");

	if(actual_ty(initTy)->kind != Ty_int)
		EM_error(a->u.array.init.pos, "type not int");
	return expTy(NULL, Ty_Array(arrayTy));
}

expty transLet(S_table venv, S_table tenv, A_exp a)
{
	A_decList letDecs = a->u.let.decs;

	while(letDecs){
		transDec(venv, tenv, letDecs->head);
		letDecs = letDecs->tail;
	}

	return transExp(venv, tenv, a->u.let.body);
}

expty transFor(S_table venv, S_table tenv, A_exp a)
{
	S_beginScope(venv);

	expty forLoTy = transExp(venv, tenv, a->u.forr.lo);
	expty forHiTy = transExp(venv, tenv, a->u.forr.hi);

	if(actual_ty(forLoTy.ty)->kind != Ty_int || actual_ty(forHiTy.ty)->kind != Ty_int)
		EM_error(a->pos, "not int");

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
	if(expty.ty->kind != Ty_void)
		EM_error(a->pos, "while body must produce no value");

	return expTy(NULL, Ty_Void());
}

expty transIf(S_table venv, S_table tenv, A_exp a)
{
	A_exp ifTest = a->u.iff.test;
	A_exp ifThen = a->u.iff.then;
	A_exp ifElse = a->u.iff.elsee;

	expty ifElseTy = transExp(venv, tenv, ifElse);

	return expTy(NULL, ifElseTy.ty);
}

expty transAssign(S_table venv, S_table tenv, A_exp a)
{
	A_var assignVar = a->u.assign.var;
	A_exp assignExp = a->u.assign.exp;

	expty assignExpTy = transExp(venv, tenv, assignExp);
	expty assignVarTy = transVar(venv, tenv, assignVar);

	if(actual_ty(assignExpTy.ty) != actual_ty(assignVarTy.ty))
		EM_error(a->pos, "assign type not same");

	return expTy(NULL, assignVarTy.ty);
}

//return type of the last exp
expty transSeq(S_table venv, S_table tenv, A_exp a)
{
	A_expList seqExp = a->u.seq;

	expty result = NULL;
	while(seqExp){
		result = transExp(venv, tenv, seqExp->head);
		seqExp = seqExp->tail;
	}

	return result;
}
//check whether the type of each field of record is same as tyfields
expty transRecord(S_table venv, S_table tenv, A_exp a)
{
	S_symbol recordSym = a->u.record.typ;
	A_efieldList recordFields = a->u.record.fields;

	E_enventry x = S_look(venv, recordSym);

	if(!x || x->kind != E_varEntry){
		EM_error(a->pos, "undefined variable");
		return expTy(NULL, Ty_Int());
	}

	Ty_ty originTy = x->u.var.ty;
	if(actual_ty(originTy)->kind != Ty_record)
		EM_error(a->pos, "not a record");

	Ty_fieldList originFields = originTy.record;

	Ty_field tmpTyField = NULL;
	A_efield tmpEfield = NULL;
	while(recordFields && originFields){
		tmpEfield = recordFields->head;
		tmpTyField = originFields->head;

		//name must be same
		if(tmpEfield->name != tmpTyField->name)
			EM_error(a->pos, "field %s doesn't exist", S_name(tmpEfield->name));

		Ty_ty tmpExpTy = transExp(venv, tenv, tmpEfield->exp);

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
		if(actual_ty(leftExpTy.ty) != actual_ty(rightExpTy.ty))
			EM_error(a->pos, "not same type");
	}

	return expTy(NULL, Ty_Int());
}

//check whether the type of args is same as formals
expty transCall(S_table venv, S_table tenv, A_exp a)
{
	S_symbol funcSym = a->u.call.func;
	A_expList funcArgs = a->u.call.args;

	E_enventry x = S_look(venv, v->funcSym);
	//if(!x || x->kind != E_funEntry);

	Ty_tyList funcFormals = x->u.fun.formals;

	expty tmpExpTy = NULL;
	while(funcArgs && funcFormals){
		tmpExpTy = transExp(venv, tenv, funcArgs->head);
		if(actual_ty(tmpExpTy.ty) != actual_ty(funcFormals->head))
			EM_error(funcArgs->head->pos, "args type not same");

		funcArgs = funcArgs->tail;
		funcFormals = funcFormals->tail;
	}

	if(funcArgs || funcFormals)
		EM_error(a->pos, "the number of args is not same");

	return expTy(NULL, x->u.fun.result);
}

expty transVar(S_table venv, S_table tenv, A_var v)
{
	switch (v->kind) {
		case A_simpleVar:{
			//find the actual ty of simple var
			E_enventry x = S_look(venv, v->u.simple);
			if(x && x->kind == E_varEntry)
				return expTy(NULL, actual_ty(x->u.var.ty));
			else{
				EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
				return expTy(NULL, Ty_Int());
			}
		}
		case A_fieldVar:{
			//find the actual type of field.id
			A_var fieldCase = v->u.field.var; //field.id
			S_symbol idSym = v->u.field.sym;

			expty fieldCaseTy = transVar(fieldCase);
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
				EM_error(v->pos, "not Ty_array");
				return expTy(NULL, Ty_Int());
			}

			return expTy(NULL, arrVarTy.ty->u.array);
		}
		default:
			return NULL;
	}
}

void transDec(S_table venv, S_table tenv, A_dec d)
{
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
	A_nametyList nameTyList = d->u.type;

	A_nametyList i;
	for(i = nameTyList; i; i = i->tail){

		//existed
		if(S_look(tenv, i->head->name)){
			EM_error(d->pos, "two types have the same name");
			return;
		}

		S_enter(tenv, i->head->name, Ty_Name(i->head->name, ))
	}
}


void transVarDec(S_table venv, S_table tenv, A_dec d)
{
	Ty_ty varTy = S_look(tenv, d->u.var.typ);
	expty initTy = transExp(venv, tenv, d->u.var.init);

	//check var type and init type
	if(actual_ty(varTy) != actual_ty(initTy.ty)){
		EM_error(d->pos, "type mismatch");
		return;
	}

	S_enter(venv, d->var.var, E_VarEntry(varTy));
}

//check params and func body
//type of func body must be same as result
void transFunDec(S_table venv, S_table tenv, A_dec d)
{
	A_fundecList funcList = d->u.function;

	A_fundec tmpFun = NULL;
	while(funcList){
		tmpFun = funcList->head;

		Ty_tyList funFormals = Ty_TyList(NULL, NULL);
		Ty_tyList funFormalsTail = funFormals;

		A_fieldList funParas = tmpFun->params;
		A_field tmpField = NULL;
		while(funParas){ //build formals of fun and check params
			tmpField = funParas->head;

			Ty_ty tmpTy = S_look(tenv, tmpField->typ);

			//check para type
			if(!tmpTy)
				EM_error(tmpField->pos, "undefined para type %s", S_name(tmpField->typ));

			Ty_tyList tmpTyList = Ty_TyList(tmpTy, NULL);
			tmpTyList->tail = NULL;

			funFormalsTail->tail = tmpTyList;
			funFormalsTail = tmpTyList;

			//next param
			funParas = funParas->tail;
		}

		Ty_ty funResult = S_look(tenv, tmpFun->result); //fun result type

		//check result type
		if(!funResult)
			EM_error(tmpField->pos, "undefined result type %s", S_name(tmpFun->result));

		S_enter(venv, tmpFun->name, E_FunEntry(funFormals, funResult));

		//next function
		funcList = funcList->tail;
	}

	//check func body
	funcList = d->u.function;
	while(funcList){
		tmpFun = funcList->head;

		E_enventry x = S_look(venv, tmpFun->name);
		Ty_tyList tmpFormals = x->u.fun.formals;

		S_beginScope(tenv); S_beginScope(venv);

		Ty_tyList i;
		A_fieldList j;

		//add the params to env
		for(i = tmpFormals, j = tmpFun->params; i; i = i->tail, j = j->tail;)
			S_enter(venv, j->head->name, E_VarEntry(i->head));

		//check body
		expty tmpBodyTy = transExp(tenv, venv, tmpFun->body);

		if(actual_ty(tmpBodyTy.ty) != actual_ty(x->u.fun.result))
			EM_error(tmpFun->body->pos, "mismatch type of result");

		S_endScope(tenv);S_endScope(venv);

		//next function
		funcList = funcList->tail;
	}
}

Ty_ty	transTy (S_table tenv, A_ty a)
{

}
