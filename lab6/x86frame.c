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


Temp_temp F_RV(void)
{
	return F_RAX();
}

T_exp F_externalCall(string s, T_expList args)
{
	return T_Call(T_Name(Temp_namedlabel(s)), args);
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

F_accessList makeAccList(U_boolList formals)
{
	if(!formals)
		return NULL;

	F_access tmpAccess;int count = 0;
	if(formals->head){//if escape
		tmpAccess = InFrame(count * F_wordSize);
		count++;
	}else	tmpAccess = InReg(Temp_newtemp());

	return F_AccessList(tmpAccess, makeAccList(formals->tail));
}

F_frame F_newFrame(Temp_label name, U_boolList formals)
{
	F_frame resultFrame = checked_malloc(sizeof(*resultFrame));
	resultFrame->name = name;
	resultFrame->locals = NULL;
	resultFrame->stackOff = 0;

	/*F_accessList resultFormals = F_AccessList(NULL, NULL);
	F_accessList iterator = resultFormals;*/
	resultFrame->formals = makeAccList(formals);

	//build formals of result
	/*F_access tmpAccess; int count = 0;
	for(; formals; formals = formals->tail){
		if(formals->head){//if escape
			count++;
			tmpAccess = InFrame(count * F_wordSize);
		}else	tmpAccess = InReg(Temp_newtemp());

		//iterator = iterator->tail;
		resultFormals = F_AccessList(tmpAccess, resultFormals);
	}
	//free the first node
	F_accessList old = resultFormals;
	resultFormals = resultFormals->tail;
	free(old);*/

	return resultFrame;
}
Temp_temp F_ArgReg(int index)
{
	switch (index)
	{
	case 0:
		return F_RDI();
	case 1:
		return F_RSI();
	case 2:
		return F_RDX();
	case 3:
		return F_RCX();
	case 4:
		return F_R8();
	case 5:
		return F_R9();
	default:
		assert(0);
	}

	assert(0);
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
	//printf("%d\n", escape);

	//printf("%d\n",f->stackOff );
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

Temp_temp F_FP(void)
{
	return F_RBP();
}

Temp_temp F_RAX(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%rax"));
	}

	return t;
}
Temp_temp F_RBX(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%rbx"));
	}
	return t;
}
Temp_temp F_RCX(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%rcx"));
	}
	return t;
}
Temp_temp F_RDX(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%rdx"));
	}
	return t;
}
Temp_temp F_RSI(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%rsi"));
	}
	return t;
}
Temp_temp F_RDI(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%rdi"));
	}
	return t;
}
Temp_temp F_RBP(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%rbp"));
	}
	return t;
}
Temp_temp F_RSP(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%rsp"));
	}
	return t;
}
Temp_temp F_R8(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%r8"));
	}
	return t;
}
Temp_temp F_R9(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%r9"));
	}
	return t;
}
Temp_temp F_R10(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%r10"));
	}
	return t;
}
Temp_temp F_R11(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%r11"));
	}
	return t;
}
Temp_temp F_R12(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%r12"));
	}
	return t;
}
Temp_temp F_R13(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%r13"));
	}
	return t;
}
Temp_temp F_R14(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%r14"));
	}
	return t;
}
Temp_temp F_R15(void)
{
	static Temp_temp t = NULL;
	if (!t) {
		t = Temp_newtemp();
		Temp_enter(F_tempMap, t, String("%r15"));
	}
	return t;
}

AS_proc F_procEntryExit3(F_frame f, AS_instrList body)
{
	char inst1[300],inst2[300];

	sprintf(inst1, "addq $%d %%rsp\n", f->stackOff);
	sprintf(inst2, "");

	return AS_Proc(String(inst1), body, String(inst2));
}
