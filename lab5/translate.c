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

static F_fragList frags;//code and string

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

struct Tr_expList_ {
	Tr_exp head;
	Tr_expList tail;
}

//Tr_exp constructor
static Tr_exp Tr_Ex(T_exp ex)
{
	Tr_exp te = checked_malloc(sizeof(*te));
	te->kind = Tr_ex;
	te->u.ex = ex;
	return te;
}

static Tr_exp Tr_Nx(T_stm nx)
{
	Tr_exp te = checked_malloc(sizeof(*te));
	te->kind = Tr_nx;
	te->u.nx = nx;
	return te;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm)
{
	Tr_exp te = checked_malloc(sizeof(*te));
	te->kind = Tr_cx;
	te->u.cx.trues = trues;
	te->u.cx.falses = falses;
	te->u.cx.stm = stm;
	return te;
}

static Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail)
{
	Tr_expList el = checked_malloc(sizeof(*el));
	el->head = head;
	el->tail = tail;
	return el;
}

//transform a form to another form
//for example: flag := (a>b | c<d)
static T_exp unEx(Tr_exp e)
{
	switch (e->kind) {
		case Tr_ex:
			return e->u.ex;
		case Tr_Cx:{
			Temp_temp r = Temp_newtemp();
			Temp_label t = Temp_newlabel(), f = Temp_newlabel();
			doPatch(e->u.cx.trues, t);
			doPatch(e->u.cx.falses, f);
			return T_Eseq(T_Move(T_Temp(r), T_Const(1)),
							T_Eseq(e->u.cx.stm,
						   T_Eseq(T_Label(f),	//if false jump to here r = 0
			 				  T_Eseq(T_Move(T_Temp(r), T_Const(0)),
						 		 T_Eseq(T_Label(t), //if true jump to here r = 1
									T_Temp(r))))));
		}
		case Tr_nx:
			return T_Eseq(e->u.nx, T_Const(0));
	}

	assert(0); //can't get here
}

static struct Cx unCx(Tr_exp e)
{
	switch (e->kind) {
		case Tr_Cx:
			return e->u.cx;
		case Tr_Ex:{
			struct Cx c;
			c.stm = T_Cjump(T_ne, e->u.ex, T_Const(0), NULL, NULL);
			c.trues = PatchList(&(c->u.stm.u.CJUMP.true), NULL);
			c.falses = PatchList(&(c->u.stm.u.CJUMP.true), NULL);
			return c;
		}
		case Tr_Nx:
			assert(0); //can not transform to Cx
	}
	assert 0;
}

static T_stm unNx(Tr_exp e)
{
	switch (e->kind) {
		case Tr_Nx:
			return e->u.nx;
		case Tr_Ex:
			return T_Exp(e->u.ex); //T_Exp calculate exp but not return result
		case Tr_Cx:{
			Temp_label empty = Temp_newlabel();
			// no matter true or false, just jump to empty
			doPatch(e->u.cx.trues, empty);
			doPatch(e->u.cx.falses, empty);
			return T_Seq(e->u.cx.stm, T_Label(empty));
		}
	}
	assert(0);
}

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

//trans var
Tr_exp Tr_simpleVar(Tr_access acc, Tr_level l)
{
	T_exp fp = T_Temp(F_FP());
	while(acc->level != l){ //track up
		//static link in a word off fp
		fp = T_Mem(T_Binop(T_plus, fp, T_Const(F_wordSize)));
		l = l->parent;
	}

	return Tr_Ex(F_Exp(acc->access, fp));
}

Tr_exp Tr_fieldVar(Tr_exp addr, int num)
{
	//Tr_exp accAddr = Tr_simpleVar(acc, l);
	T_exp valueExp = T_Mem(T_Binop(T_plus, unEx(addr), F_wordSize*num));
	return Tr_Ex(valueExp);
}

Tr_exp Tr_subscriptVar(Tr_exp addr, Tr_exp e)
{
	//Tr_exp accAddr = Tr_simpleVar(acc, l);
	T_exp valueExp = T_Mem(T_Binop(T_plus, unEx(addr), unEx(e)));
	return Tr_Ex(valueExp);
}

//trans exp
Tr_exp Tr_nilExp()
{
    return Tr_Ex(T_Const(0));
}

Tr_exp Tr_intExp(int i)
{
    return Tr_Ex(T_Const(i));
}

Tr_exp Tr_stringExp(string s)
{
	Temp_label lab = Temp_newlabel();
	F_frag sFrag = F_StringFrag(lab, str);
	frags = F_FragList(sFrag, frags);
	return Tr_Ex(T_Name(lab));
}

//pass the static link to callee
Tr_exp Tr_callExp(Temp_label func, Tr_expList args, Tr_level callerLevel, Tr_level calleeLevel)
{
	//trans args
	T_expList argsTree = NULL; //tail = NULL
	while(args){
		argsTree = T_ExpList(unEx(args->head), argsTree);
		args = args->tail;
	}

	//pass static link
	T_exp fp = T_Temp(F_FP());//current fp
	Tr_level calleeParent = calleeLevel->parent;
	Tr_level iterator = callerLevel;
	while(iterator != calleeParent){
		fp = T_Mem(T_Binop(T_plus, fp, F_wordSize));
		iterator = iterator->parent;
	}

	//add sl to args
	argsTree = T_expList(fp, argsTree);

	return Tr_Ex(T_Call(T_Label(func), argsTree));
}

Tr_exp Tr_arithExp(A_oper op, Tr_exp left, Tr_exp right)
{
	T_exp tLeft = unEx(left), tRight = unEx(right);
	switch (op) {
		case A_plusOp:
			return Tr_Ex(T_Binop(T_plus, tLeft, tRight));
		case A_minusOp:
			return Tr_Ex(T_Binop(T_minus, tLeft, tRight));
		case A_timesOp:
			return Tr_Ex(T_Binop(T_times, tLeft, tRight));
		case A_divideOp:
			return Tr_Ex(T_Binop(T_devide, tLeft, tRight));
		default:
			assert(0);
	}
	assert(0);
}

Tr_exp Tr_compExp(A_oper op, Tr_exp left, Tr_exp right)
{
	T_exp tLeft = unEx(left), tRight = unEx(right);
	T_relOp treeOp;
	switch (op) {
		case A_eqOp:
			treeOp = T_eq;
			break;
		case A_neqOp:
			treeOp = T_ne;
			break;
		case A_ltOp:
			treeOp = T_lt;
			break;
		case A_leOp:
			treeOp = T_le;
			break;
		case A_gtOp:
			treeOp = T_gt;
			break;
		case A_geOp:
			treeOp = T_ge;
			break;
		default:
			assert(0);
	}

	T_stm cj= T_Cjump(treeOp, tLeft, tRight, NULL, NULL);
	patchList trues= patchList(&(cj->u.CJUMP.true), NULL);
	patchList falses= patchList(&(cj->u.CJUMP.false), NULL);
	return Tr_Cx(trues, falses, cj);
}

//fisrt malloc then move the exp to id , a last return addr
// type-id{id=exp,id=exp....}
Tr_exp Tr_recordExp(Tr_expList fields)
{
	//don't know the addr of record now
	T_exp recordAddr = T_Temp(Temp_newtemp());

	T_stm assign = NULL;
	int count;
	for(count = 0; fields; count++, fields = fields->tail){
		T_exp dst = T_Binop(T_plus, recordAddr, T_Const(count*F_wordSize));
		assign = T_Seq(T_Move(dst, unEx(fields->head)), assign);
	}

	T_exp callMalloc = F_externalCall("malloc", T_ExpList(T_Const(count*F_wordSize), NULL));
	T_stm mallocStm = T_Move(recordAddr, callMalloc);

	return T_Eseq(T_Seq(mallocStm, assign), recordAddr);
}

Tr_exp Tr_assignExp(Tr_exp left, Tr_exp right)
{
	return Tr_Nx(T_Move(unEx(left), unEx(right)));
}

// bug
Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee)
{
	Temp_label truee = Temp_newlabel(), falsee = Temp_newlabel();
	struct Cx testCx = unCx(test);
	doPatch(testCx.trues, truee);doPatch(testCx.falses, falsee);

	T_stm thenStm;
	if(then->kind == Tr_cx)
		thenStm = then->u.cx.stm;
	else
		thenStm = unNx(test);

	T_stm elseStm;
	if(elsee->kind == Tr_cx)
		elseStm = elsee->u.cx.stm;
	else
		elseStm = unNx(test);

	T_stm s =  T_Seq(testCx.stm,
							T_Seq(T_Label(truee),
					 		 T_Seq(thenStm,
					  	 	T_Seq(T_Label(falsee),
						 		 elseStm))));
	return unNx(s);
}

Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body, Temp_label doneLab)
{
	Temp_label testLab = Temp_newlabel(), bodyLab = Temp_newlabel();
	struct Cx testCx = unCx(test);
	doPatch(testCx.trues, bodyLab);
	doPatch(testCx.falses, doneLab);

	T_stm testStm = T_Seq(T_Label(testLab), testCx.stm);
	T_stm bodyStm = T_Seq(T_Label(bodyLab),
									 T_Seq(unNx(body),
									  T_Seq(T_Jump(T_Name(testLab), Temp_LabelList(testLab, NULL)),
										 T_Label(doneLab))));
	return Tr_Nx(T_Seq(testStm, bodyStm));
}

Tr_exp Tr_forExp(Tr_exp low, Tr_exp high, Tr_exp body, Temp_label doneLab)
{
	T_exp iterator = T_Temp(Temp_newtemp());
	T_exp lowTree = unEx(low), highTree = unEx(high);
	Temp_label bodyLab = Temp_newlabel(), testLab = Temp_newlabel();
	T_stm bodyTree = unNx(body);

	//assign the iterator ,put the label and test
	T_stm init = T_Move(iterator, lowTree);
	T_stm test = T_Seq(T_Label(testLab),
							  T_Cjump(T_le, iterator, highTree, bodyLab, doneLab));

	T_stm update = T_Move(iterator, T_Binop(T_plus, iterator, T_Const(1)));
	T_stm toTest = T_Jump(T_Name(testLab), Temp_LabelList(testLab, NULL));

	// put bodyLab ,exe body and jumpto test
	T_stm bodyLoop = T_Seq(T_Label(bodyLab), T_Seq(bodyTree,
									 T_Seq(update,
									  T_Seq(toTest,
										 T_Label(doneLab))));

	//init and test
	T_stm testLoop = T_Seq(init, test);

	return Tr_Nx(T_Seq(testLoop, bodyLoop));
}

Tr_exp Tr_breakExp(Temp_label doneLab)
{
	return Tr_Nx(T_Jump(T_Name(doneLab), Temp_LabelList(doneLab, NULL)))
}

Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init)
{
	T_exp tmp = T_Temp(Temp_newtemp());
	T_exp callResult = F_externalCall("initArray",
							 T_ExpList(unEx(size), T_ExpList(unEx(init), NULL)));

	T_stm assign = T_Move(tmp, callResult);
	return Tr_Ex(T_Eseq(assign, tmp));
}

Tr_exp Tr_typeDec()
{
    return Tr_Ex(T_Const(0));
}
