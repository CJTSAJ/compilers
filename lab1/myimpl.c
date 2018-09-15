#include "prog1.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int isAllowPrint = 0;
static int argNum = 0;

/*define the type of Table_*/
typedef struct table{
	string id;
	int value;
	struct table *tail;
} *Table_;

Table_ Table(string id, int value, Table_ tail)
{
	Table_ t = malloc(sizeof(*t));
	t->value = value;
	t->id = id;
	return t;
}

typedef struct{
	int i;
	Table_ t;
} IntAndTable;

//declaration
Table_ interpStm(A_stm s, Table_ t);

/*find the value according to the key*/
int lookup(Table_ t, string key)
{
	while(t){
		if(!strcmp(t->id, key)){
			return t->value;
		}
		t = t->tail;
	}
}

int operation(int left, int op, int right)
{
	switch (op) {
		case A_plus:{
			return left + right;
		}
		case A_minus:{
			return left - right;
		}
		case A_times:{
			return left * right;
		}
		default:{//A_div
			return left / right;
		}
	}
}

IntAndTable interpExp(A_exp e, Table_ t){
	IntAndTable result;
	result.t = t;
	switch (e->kind) {
		case A_idExp:{
			result.i = lookup(t, e->u.id);
			return result;
		}
		case A_numExp:{
			result.i = e->u.num;
			return result;
			break;
		}
		case A_opExp:{
			/*table changed ????*/
			IntAndTable left = interpExp(e->u.op.left, t);
			IntAndTable right = interpExp(e->u.op.right, t);

			result.i = operation(left.i, e->u.op.oper, right.i);
			return result;
		}
		default:{//A_eseqExp
			Table_ tempT = interpStm(e->u.eseq.stm, t);
			return interpExp(e->u.eseq.exp, tempT);
		}
	}
}

void printStm(A_expList exps, Table_ t, int count)
{
	if(count > argNum) argNum = count;

	switch (exps->kind) {
		case A_pairExpList:{
			IntAndTable temp = interpExp(exps->u.pair.head, t);
			if(isAllowPrint) printf("%d", temp.i);
			printStm(exps->u.pair.tail, t, count + 1);
			break;
		}
		default:{// A_lastExpList
			IntAndTable temp = interpExp(exps->u.last, t);
			if(isAllowPrint) printf("%d\n", temp.i);
			break;
		}
	}
}

Table_ interpStm(A_stm s, Table_ t)
{
	switch (s->kind) {
		case A_compoundStm:{
			return interpStm(s->u.compound.stm2, interpStm(s->u.compound.stm1, t));
			break;
		}
		case A_assignStm:{
			IntAndTable iat = interpExp(s->u.assign.exp, t);
			return Table(s->u.assign.id, iat.i, iat.t);
			break;
		}
		default:{// A_printStm
			printStm(s->u.print.exps, t, 1);
			return t;
		}
	}
}
int maxargs(A_stm stm)
{
	//no side effect
	isAllowPrint = 0;
	argNum = 0;
	interpStm(stm, NULL);
	return argNum;
}

void interp(A_stm stm)
{
	//allow print
	isAllowPrint = 1;
	argNum = 0;
	interpStm(stm, NULL);
}
