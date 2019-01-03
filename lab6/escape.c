#include <stdio.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "escape.h"
#include "table.h"

static void traverseExp(S_table env, int depth, A_exp e);
typedef struct escapeEntry_ * escapeEntry;

struct escapeEntry_ {
	int depth;
	bool *escape;
};

static escapeEntry EscapeEntry(int depth, bool *escape)
{
	escapeEntry entry = checked_malloc(sizeof(*entry));
	entry->depth = depth;
	entry->escape = escape;
	return entry;
}

static void traverseDec(S_table env, int depth, A_dec d)
{
	switch (d->kind)
	{
	case A_functionDec:{
		A_fundecList funList = d->u.function;
		for (; funList; funList = funList->tail) {
			A_fundec tmp_fun = funList->head;
			A_fieldList paraList = tmp_fun->params;
			S_beginScope(env);
			for (; paraList; paraList = paraList->tail) {
				paraList->head->escape = FALSE;
				S_enter(env, paraList->head->name, EscapeEntry(depth + 1, &(paraList->head->escape)));
			}
			traverseExp(env, depth + 1, tmp_fun->body);
			S_endScope(env);
		}
	}
	case A_varDec: {
		traverseExp(env, depth, d->u.var.init);
		d->u.var.escape = FALSE;
		S_enter(env, d->u.var.var, EscapeEntry(depth, &(d->u.var.escape)));
		return;
	}
	case A_typeDec:
		return;
	}

	assert(0);
}
static void traverseVar(S_table env, int depth, A_var v)
{
	switch (v->kind)
	{
	case A_simpleVar: {
		escapeEntry entry = S_look(env, v->u.simple);

		if (entry->depth < depth)
			*(entry->escape) = TRUE;

		return;
	}
	case A_fieldVar: {
		traverseVar(env, depth, v->u.field.var);
		return;
	}
	case A_subscriptVar: {
		traverseVar(env, depth, v->u.subscript.var);
		traverseExp(env, depth, v->u.subscript.exp);
		return;
	}
	}

	assert(0);
}

static void traverseExp(S_table env, int depth, A_exp e)
{
	switch (e->kind)
	{
	case A_varExp: {
		traverseVar(env, depth, e->u.var);
		return;
	}
	case A_callExp: {
		A_expList tmp_exps = e->u.call.args;
		for (; tmp_exps; tmp_exps = tmp_exps->tail)
			traverseExp(env, depth, tmp_exps->head);

		return;
	}
	case A_opExp: {
		traverseExp(env, depth, e->u.op.left);
		traverseExp(env, depth, e->u.op.right);
		return;
	}
	case A_recordExp: {
		A_efieldList tmp_fields = e->u.record.fields;
		for (; tmp_fields; tmp_fields = tmp_fields->tail)
			traverseExp(env, depth, tmp_fields->head->exp);

		return;
	}
	case A_seqExp: {
		A_expList tmp_exps = e->u.seq;
		for (; tmp_exps; tmp_exps = tmp_exps->tail)
			traverseExp(env, depth, tmp_exps->head);
		return;
	}
	case A_assignExp: {
		traverseVar(env, depth, e->u.assign.var);
		traverseExp(env, depth, e->u.assign.exp);
		return;
	}
	case A_ifExp: {
		traverseExp(env, depth, e->u.iff.test);
		traverseExp(env, depth, e->u.iff.then);
		if (e->u.iff.elsee) traverseExp(env, depth, e->u.iff.elsee);
		return;
	}
	case A_whileExp: {
		traverseExp(env, depth, e->u.whilee.test);
		traverseExp(env, depth, e->u.whilee.body);
		return;
	}
	case A_forExp: {
		e->u.forr.escape = FALSE;
		S_enter(env, e->u.forr.var, EscapeEntry(depth, &(e->u.forr.escape)));
		traverseExp(env, depth, e->u.forr.lo);
		traverseExp(env, depth, e->u.forr.hi);

		//a higher depth
		S_beginScope(env);
		traverseExp(env, depth, e->u.forr.body);
		S_endScope(env);
	}
	case A_letExp: {
		S_beginScope(env);
		A_decList decs = e->u.let.decs;
		for (; decs; decs = decs->tail)
			traverseDec(env, depth, decs->head);
		traverseExp(env, depth, e->u.let.body);
		S_endScope(env);
		return;
	}
	case A_arrayExp: {
		traverseExp(env, depth, e->u.array.size);
		traverseExp(env, depth, e->u.array.init);
		return;
	}
	default:
		return;
	}

	assert(0);
}

void Esc_findEscape(A_exp exp) {
	S_table env = S_empty();
	traverseExp(env, 0, exp);
}
