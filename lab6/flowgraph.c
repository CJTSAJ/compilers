#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "errormsg.h"
#include "table.h"

Temp_tempList FG_def(G_node n) {
	AS_instr ins = (AS_instr)G_nodeInfo(n);
	switch (ins->kind)
	{
	case I_OPER:
		return ins->u.OPER.dst;
	case I_LABEL:
		assert(0);
	case I_MOVE:
		return ins->u.MOVE.dst;
	default:
		assert(0);
	}
	assert(0);
}

Temp_tempList FG_use(G_node n) {
	AS_instr ins = (AS_instr)G_nodeInfo(n);
	switch (ins->kind)
	{
	case I_OPER:
		return ins->u.OPER.src;
	case I_LABEL:
		assert(0);
	case I_MOVE:
		return ins->u.MOVE.src;
	default:
		assert(0);
	}
	assert(0);
}

bool FG_isMove(G_node n) {
	AS_instr ins = (AS_instr)G_nodeInfo(n);
	return ins->kind == I_MOVE;
}

//gen flow graph accordding to AS_instrList
G_graph FG_AssemFlowGraph(AS_instrList il, F_frame f) {
	G_graph graph = G_Graph(); //alloc a new graph
	G_node prev_node = NULL; //there is a edge between 2 insts
	TAB_table lab_table = TAB_empty(); //label inst table

	for (AS_instrList it = il; it; it = it->tail) {
		G_node cur_node = G_Node(graph, it->head); //*info point to AS_instr
		if (prev_node)G_addEdge(prev_node, cur_node); //a edgr from prev to current

		//consider jump inst
		if (it->head->kind == I_OPER && !strncmp("jmp", it->head->u.OPER.assem, 3))  //compare 3 chars "jmp"
			prev_node = NULL; //jmp edge point to another inst
		else
			prev_node = cur_node;

		//label inst: add it to a table
		if (it->head->kind == I_LABEL)
			TAB_enter(lab_table, it->head->u.LABEL.label, cur_node);
	}

	//add the jmp edge
	for (G_nodeList cur_nodes = G_nodes(graph); cur_nodes; cur_nodes = cur_nodes->tail) {
		AS_instr tmp_ins = G_nodeInfo(cur_nodes->head);
		if (tmp_ins->kind == I_OPER && !strncmp("jmp", tmp_ins->u.OPER.assem, 3)) {
			//add edges for all targets
			Temp_labelList targets_temp = tmp_ins->u.OPER.jumps->labels;
			for (Temp_labelList tmp = targets_temp; tmp; tmp = tmp->tail) {
				G_node target_ins = TAB_look(lab_table, tmp->head);
				if(target_ins) G_addEdge(cur_nodes->head, target_ins);
			}
		}
	}
	return graph;
}
