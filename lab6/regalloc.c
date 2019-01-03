#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "liveness.h"
#include "color.h"
#include "regalloc.h"
#include "table.h"
#include "flowgraph.h"

static bool InTemp(Temp_tempList tl, Temp_temp t)
{
	for (; tl; tl = tl->tail) {
		if (tl->head == t)
			return TRUE;
	}

	return FALSE;
}


static void ReplaceTemp(Temp_tempList tl, Temp_temp old, Temp_temp new_)
{
	for (; tl; tl = tl->tail) {
		if (tl->head == old)
			tl->head = new_;
	}
}

//for all spillnode
AS_instrList RewriteProgram(F_frame f, AS_instrList il, Temp_tempList spills)
{

	for (; spills; spills = spills->tail) {
		Temp_temp cur_spill = spills->head;
		AS_instrList insts = il;

		for (; insts; insts = insts->tail) {
			AS_instr cur_inst = insts->head;


			if (cur_inst->kind == I_OPER || cur_inst->kind == I_MOVE) { //OPER and MOVE have familiar structure
				if (InTemp(cur_inst->u.OPER.dst, cur_spill)) {
					Temp_temp new_temp = Temp_newtemp(); //use new temp, when dst write it in stack, when src readit from stack
					ReplaceTemp(cur_inst->u.OPER.dst, cur_spill, new_temp);

					//put it on the stack
					char tmp_inst[100];
					sprintf(tmp_inst, "movq `s0, %d(`s1)", f->stackOff);

					//add it to il
					AS_instr new_ins = AS_Oper(String(tmp_inst), NULL, Temp_TempList(new_temp, Temp_TempList(F_RBP(), NULL)), AS_Targets(NULL));
					insts->tail = AS_InstrList(new_ins, insts->tail);
				}
				else if (InTemp(cur_inst->u.OPER.src, cur_spill)) {
					Temp_temp new_temp = Temp_newtemp();
					ReplaceTemp(cur_inst->u.OPER.src, cur_spill, new_temp);

					//read it from stack
					char tmp_inst[100];
					sprintf(tmp_inst, "movq %d(`s0), `d0", f->stackOff);
					AS_instr fetch_inst = AS_Oper(String(tmp_inst), Temp_TempList(new_temp, NULL),
																				Temp_TempList(F_RBP(), NULL), AS_Targets(NULL));

					insts->head = fetch_inst;
					insts->tail = AS_InstrList(cur_inst, insts->tail);
				}
			}
		}
	}
}

struct RA_result RA_regAlloc(F_frame f, AS_instrList il) {
	//your code here
	G_graph flow_graph = FG_AssemFlowGraph(il, f);
	printf("RA_regAlloc flow_graph done\n");
	struct Live_graph live_graph = Live_liveness(flow_graph);
	printf("RA_regAlloc live_graph done\n");
	struct COL_result col_result = COL_color(live_graph.graph, F_tempMap, NULL, live_graph.moves);
	printf("RA_regAlloc COL_color done\n");
	while (col_result.spills != NULL) {
		AS_instrList new_il = RewriteProgram(f, il, col_result.spills);
		return RA_regAlloc(f, new_il);
	}

	//if the dst and src of I_MOVE use the same reg, delete the instr
	AS_instrList* tmp_instlist = &il;


	struct RA_result ret;
	ret.coloring = col_result.coloring;
	ret.il = il;
	return ret;
}
