#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "liveness.h"
#include "table.h"

#define TL_SAME(t1,t2) (Temp_diff(t1,t2) == NULL && Temp_diff(t2,t1) == NULL)

static G_nodeList reverse;
static Live_moveList moves;
typedef struct
{
	Temp_tempList in;
	Temp_tempList out;
} *liveInOut;

// contruct liveInOut
liveInOut LiveInOut(Temp_tempList in, Temp_tempList out)
{
	liveInOut ret = checked_malloc(sizeof(*ret));
	ret->in = in;
	ret->out = out;
	return ret;
}

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail) {
	Live_moveList lm = (Live_moveList) checked_malloc(sizeof(*lm));
	lm->src = src;
	lm->dst = dst;
	lm->tail = tail;
	return lm;
}

//get temp accordding to node n
// *info point to Temp_temp
Temp_temp Live_gtemp(G_node n) {
	return G_nodeInfo(n);
}

//convert flow graph to conflict graph with no edge
// and genarate a reverse nodelist, generate a node-liveInOut table
static G_graph TempGraph(G_graph flow, TAB_table inOutTab, TAB_table temp2NodeTab)
{
	G_graph conflictGraph = G_Graph();
	G_nodeList tmp_nodes = G_nodes(flow);
	for (; tmp_nodes; tmp_nodes = tmp_nodes->tail) {
		Temp_tempList defs = FG_def(tmp_nodes->head);
		Temp_tempList uses = FG_use(tmp_nodes->head);
		for (; defs; defs = defs->tail) {
			G_node tmp_node = G_Node(conflictGraph, defs->head);
			TAB_enter(temp2NodeTab, defs->head, tmp_node);
		}

		for (; uses; uses = uses->tail) {
			G_node tmp_node = G_Node(conflictGraph, uses->head);
			TAB_enter(temp2NodeTab, uses->head, tmp_node);
		}

		//generate a revers nodeList for loop analysis
		reverse = G_NodeList(tmp_nodes->head, reverse);

		//prepare in-out table
		TAB_enter(inOutTab, tmp_nodes->head, LiveInOut(NULL, NULL));
	}

	return conflictGraph;
}


//loop until in and out are unchangable
static void LivenessCompute(TAB_table inOutTab)
{

	//first compute out then in
	while (TRUE) {
		for (G_nodeList it = reverse; it; it = it->tail) {
			liveInOut old_inOut = (liveInOut)TAB_look(inOutTab, it->head);

			Temp_tempList new_out = old_inOut->out;
			//combine in[s] of all succs
			for (G_nodeList tmp_succs = G_succ(it->head); tmp_succs; tmp_succs = tmp_succs->tail) {
				liveInOut tmp_inOut = (liveInOut)TAB_look(inOutTab, tmp_succs->head);
				new_out = Temp_union(new_out, tmp_inOut->in);
			}

			// in[n] = use[n] combine (out[n] - def[n])
			Temp_tempList new_in = Temp_union(FG_use(it->head), Temp_diff(new_out, FG_def(it->head)));

			//check unchangable
			if (TL_SAME(old_inOut->in, new_in) && TL_SAME(old_inOut->out, new_out))
				break;

			//fresh the in-out table
			old_inOut->in = new_in;
			old_inOut->out = new_out;
			//TAB_enter(inOutTab, it->head, LiveInOut(new_in, new_out));
		}
	}
}

//add conflict edge and generate moveList
static void confEdge(TAB_table temp2NodeTab, TAB_table inOutTab)
{
	for (G_nodeList nodes = reverse; nodes; nodes = nodes->tail) {
		//movelist, use the same reg if it work
		if (FG_isMove(nodes->head)) {
			Temp_temp temp_dst = FG_def(nodes->head)->head;
			Temp_temp temp_src = FG_use(nodes->head)->head;
			G_node dst_node = (G_node)TAB_look(temp2NodeTab, temp_dst);
			G_node src_node = (G_node)TAB_look(temp2NodeTab, temp_src);
			moves = Live_MoveList(src_node, dst_node, moves);
		}

		//add conflict edge: defs conflict with out
		Temp_tempList temp_defs = FG_def(nodes->head);
		liveInOut temp_inOut = TAB_look(inOutTab, nodes->head);

		for (Temp_tempList it_defs = temp_defs; it_defs; it_defs = it_defs->tail) {
			G_node node1 = (G_node)TAB_look(temp2NodeTab, it_defs->head);
			for (Temp_tempList outs = temp_inOut->out; outs; outs = outs->tail) {
				G_node node2 = (G_node)TAB_look(temp2NodeTab, outs->head);
				G_addEdge(node1, node2);
			}

		}
	}
}

// Live_graph: G_graph Live_moveList
// every node has preds and succs
struct Live_graph Live_liveness(G_graph flow) {
	//your code here.
	reverse = NULL; //reverse nodelist, and node info point to AS_instr
	TAB_table inOutTab = TAB_empty(); //map flowNode to in-out
	TAB_table temp2NodeTab = TAB_empty(); //map temp to node
	moves = NULL;

	//generate conflict graph without edge
	G_graph conflictGraph = TempGraph(flow, inOutTab, temp2NodeTab);

	//loop analysis until unchangable
	LivenessCompute(inOutTab);

	//add conflict edge for conflictGraph
	confEdge(temp2NodeTab, inOutTab);

	struct Live_graph lg;
	lg.graph = conflictGraph;
	lg.moves = moves;
	return lg;
}
