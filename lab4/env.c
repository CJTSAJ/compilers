#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "env.h"

/*Lab4: Your implementation of lab4*/

E_enventry E_VarEntry(Ty_ty ty)
{
	E_enventry entry = checked_malloc(sizeof(*entry));
	entry->kind = E_varEntry;
	entry->u.var.ty = ty;
	return entry;
}

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result)
{
	E_enventry entry = checked_malloc(sizeof(*entry));
	entry->kind = E_funEntry;
	entry->u.fun.formals = formals;
	entry->u.fun.result = result;
	return entry;
}

S_table E_base_tenv(void)
{
	S_table table = S_empty();
	S_symbol symbol_int = S_Symbol("int");
	S_symbol symbol_string = S_Symbol("string");

	S_enter(table, symbol_int, Ty_Int());
	S_enter(table, symbol_string, Ty_String());
	return table;
}

S_table E_base_venv(void)
{
	S_table table = S_empty();

	return table;
}
