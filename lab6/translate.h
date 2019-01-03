#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "util.h"
#include "absyn.h"
#include "temp.h"
#include "frame.h"

/* Lab5: your code below */

typedef struct Tr_exp_ *Tr_exp;

typedef struct Tr_access_ *Tr_access;

typedef struct Tr_accessList_ *Tr_accessList;

typedef struct Tr_level_ *Tr_level;

typedef struct Tr_expList_ *Tr_expList;

typedef struct patchList_ *patchList;

struct Tr_access_ {
	Tr_level level;
	F_access access;
};


struct Tr_accessList_ {
	Tr_access head;
	Tr_accessList tail;
};

struct Tr_level_ {
	F_frame frame;
	Tr_level parent;
};

struct patchList_
{
	Temp_label *head;
	patchList tail;
};

struct Cx
{
	patchList trues;
	patchList falses;
	T_stm stm;
};

struct Tr_exp_ {
	enum {Tr_ex, Tr_nx, Tr_cx} kind;
	union {T_exp ex; T_stm nx; struct Cx cx; } u;
};

struct Tr_expList_ {
	Tr_exp head;
	Tr_expList tail;
};

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

Tr_level Tr_outermost(void);

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);

Tr_accessList Tr_formals(Tr_level level);

Tr_access Tr_allocLocal(Tr_level level, bool escape);

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail);

Tr_exp Tr_err();

Tr_exp Tr_simpleVar(Tr_access access, Tr_level level);
Tr_exp Tr_fieldVar(Tr_exp addr, int num);
Tr_exp Tr_subscriptVar(Tr_exp addr, Tr_exp e);
Tr_exp Tr_nilExp();
Tr_exp Tr_intExp(int i);
Tr_exp Tr_stringExp(string s);
Tr_exp Tr_callExp(Temp_label, Tr_expList, Tr_level, Tr_level);
Tr_exp Tr_arithExp(A_oper op, Tr_exp left, Tr_exp right);
Tr_exp Tr_compExp(A_oper op, Tr_exp left, Tr_exp right);
Tr_exp Tr_recordExp(Tr_expList fields);
Tr_exp Tr_assignExp(Tr_exp left, Tr_exp right);
Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee);
Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body, Temp_label doneLab);
Tr_exp Tr_forExp(Tr_exp low, Tr_exp high, Tr_exp body, Temp_label doneLab);
Tr_exp Tr_breakExp(Temp_label doneLab);
Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init);
Tr_exp Tr_typeDec();
Tr_exp Tr_varDec(Tr_access acc, Tr_exp e);
Tr_exp Tr_seq(Tr_exp first, Tr_exp second);
void Tr_procEntryExit(Tr_level level, Tr_exp body);
F_fragList Tr_getResult(void);
#endif
