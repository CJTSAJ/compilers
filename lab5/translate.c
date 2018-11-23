#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"
#include "frame.h"
#include "translate.h"

//LAB5: you can modify anything you want.

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

typedef struct patchList_ *patchList;
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


static patchList PatchList(Temp_label *head, patchList tail)
{
	patchList list;

	list = (patchList)checked_malloc(sizeof(struct patchList_));
	list->head = head;
	list->tail = tail;
	return list;
}

void doPatch(patchList tList, Temp_label label)
{
	for(; tList; tList = tList->tail)
		*(tList->head) = label;
}

patchList joinPatch(patchList first, patchList second)
{
	if(!first) return second;
	for(; first->tail; first = first->tail);
	first->tail = second;
	return first;
}


//Activation Records
Tr_level Tr_outermost(void)
{
	return Tr_newLevel(NULL, Temp_newlabel(), NULL);
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals)
{
	Tr_level tmpLevel = checked_malloc(sizeof(tmpLevel));
	tmpLevel->parent = parent;
	F_frame tmpFrame = F_newFrame(name, formals);
	return tmpLevel;
}

Tr_access Tr_allocLocal(Tr_level level, bool escape)
{
	Tr_access tmpAccess = checked_malloc(sizeof(*tmpAccess));
	tmpAccess = F_allocLocal(level, escape);
	return tmpAccess;
}

Tr_accessList FA2TrAccessLis(F_accessList fa, Tr_level level)
{
	Tr_access ta = checked_malloc(sizeof(*ta));
	ta->level = level;
	ta->access = fa->head;
	if(fa->tail)
		return Tr_AccessList(ta, FA2TrAccessLis(fa->tail, level));
	else
		return Tr_AccessList(ta, NULL);
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail)
{
	Tr_accessList ta = checked_malloc(sizeof(*ta));
	ta->head = head;
	ta->tail = tail;
	return ta;
}

Tr_accessList Tr_formals(Tr_level level)
{
	F_accessList fa = F_formals(level->frame);
	return FA2TrAccessLis(fa, level);
}
