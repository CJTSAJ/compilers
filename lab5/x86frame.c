#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"

const int F_wordSize = 8;//64 bit
/*Lab5: Your implementation here.*/
struct F_frame_{
	Temp_label name;
	F_accessList formals;
	F_accessList locals;
	int argSize;
	int stackOff;
};

//varibales
struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset; //inFrame
		Temp_temp reg; //inReg
	} u;
};

T_exp F_externalCall(string s, T_expList args)
{
	return T_Call(T_Name(Temp_namedlabel(s)), args);
}

Temp_temp F_FP(void)
{
	return Temp_newtemp();
}

T_exp F_Exp(F_access access, T_exp fp)
{
	if(access->kind == inFrame)
		return T_Mem(T_Binop(T_plus, fp, T_Const(access->u.offset)));
	else
		return T_Temp(access->u.reg);
}

F_accessList F_AccessList(F_access head, F_accessList tail)
{
	F_accessList fa = checked_malloc(sizeof(*fa));
	fa->head = head;
	fa->tail = tail;
	return fa;
}

static F_access InFrame(int offset)
{
	F_access tmpAccess = checked_malloc(sizeof(*tmpAccess));
	tmpAccess->kind = inFrame;
	tmpAccess->u.offset = offset;
	return tmpAccess;
}

static F_access InReg(Temp_temp reg)
{
	F_access tmpAccess = checked_malloc(sizeof(*tmpAccess));
	tmpAccess->kind = inReg;
	tmpAccess->u.reg = reg;
	return tmpAccess;
}

F_frame F_newFrame(Temp_label name, U_boolList formals)
{
	F_frame resultFrame = checked_malloc(sizeof(*resultFrame));
	resultFrame->name = name;
	resultFrame->locals = NULL;
	resultFrame->stackOff = 0;


	F_accessList resultFormals = F_accessList(NULL, NULL), iterator = resultFormals;

	//build formals of result
	F_access tmpAccess; int count = 0;
	for(; formals; formals = formals->tail){
		if(formals->head){//if escape
			count++;
			tmpAccess = InFrame(count * F_wordSize);
		}else	tmpAccess = InReg(Temp_newtemp());

		iterator = iterator->tail;
		iterator = F_accessList(tmpAccess, NULL);
	}
	//free the first node
	int old = resultFormals;
	resultFormals = resultFormals->tail;
	free(old);

	resultFrame->formals = resultFormals;

	return resultFrame;
}

Temp_label F_name(F_frame f)
{
	return f->name;
}

F_accessList F_formals(F_frame f)
{
	return f->formals;
}

F_access F_allocLocal(F_frame f, bool escape)
{
	F_access tmpAccess;
	if(escape)
		tmpAccess = InFrame(f->stackOff);
	else
		tmpAccess = InReg(Temp_newtemp());

	f->stackOff = f->stackOff - F_wordSize;
	f->locals = F_AccessList(tmpAccess, f->locals);
	return tmpAccess;
}

F_frag F_StringFrag(Temp_label label, string str)
{
	F_frag sFrag = checked_malloc(sizeof(*sFrag));
	sFrag->kind = F_stringFrag;
	sFrag->u.stringg.label = label;
	sFrag->u.stringg.str = str;
	return sFrag;
}

F_frag F_ProcFrag(T_stm body, F_frame frame)
{
	F_frag pFrag = checked_malloc(sizeof(*pFrag));
	pFrag->kind = F_procFrag;
	pFrag->u.proc.body = body;
	pFrag->u.proc.frame = frame;
	return pFrag;
}

F_fragList F_FragList(F_frag head, F_fragList tail)
{
	F_fragList l = checked_malloc(sizeof(*l));
	l->head = head;
	l->tail = tail;
	return l;
}
