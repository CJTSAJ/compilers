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
			return transIf();
		case A_whileExp:
			return transWhile();
		case A_forExp:
			return transFor();
		case A_breakExp:
			return transBreak();
		case A_letExp:
			return transLet();
		case A_arrayExp:
			return transArray();
		default:
			return NULL;
	}
}

expty transAssign(S_table venv, S_table tenv, A_exp a)
{

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

}
Ty_ty	transTy (S_table tenv, A_ty a)
{

}
