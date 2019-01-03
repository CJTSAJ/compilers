#include <stdio.h>
#include <string.h>

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
#include "table.h"

#define K 15

//data structure page 176
G_nodeList precolored;
G_nodeList initial;
G_nodeList simplifyWorklist; //low degree and not related with move
G_nodeList freezeWorklist; //low dgree and related with mve
G_nodeList spillWorklist; //high dgree
G_nodeList spilledNodes; //the node that will be spilled thie loop
G_nodeList coalescedNodes; //the node that has been coaleced
G_nodeList coloredNodes;
G_nodeList selectStack; //contain temp_node that be deleted from graph

Live_moveList coalescedMoves; //have been coalesced move
Live_moveList constrainedMoves; //
Live_moveList frozenMoves;
Live_moveList worklistMoves; //maybe coalecse move inst
Live_moveList activeMoves;

G_table degree;  // G_table = TABtable, node->degree
G_table alias;
G_table moveList; //G_ndoe -> Live_moveList
G_table color; //G_node -> color
G_table adjList;

static string hard_regs[17] = { "none", "%rax", "%r10", "%r11", "%rbx", "%r12", "%r13", "%r14", "%r15",
										"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9", "%rbp", "%rsp" };

static G_nodeList DiffNodeList(G_nodeList nodes1, G_nodeList nodes2);
static G_nodeList UnionNodeList(G_nodeList nodes1, G_nodeList nodes2);
static bool MoveRelated(G_node n);
static Live_moveList DiffMoveList(Live_moveList moves1, Live_moveList moves2);
static bool InMoveList(Live_moveList moves, G_node src, G_node dst);
static Live_moveList NodeMoves(G_node n);
static G_nodeList RemoveNodeList(G_nodeList nodes, G_node n);
static void printNodeList(G_nodeList nodes);

//the adjacent nodes of the node
static G_nodeList Adjacent(G_node n)
{
	G_nodeList tmp_nodes = G_look(adjList, n);
	return DiffNodeList(tmp_nodes, UnionNodeList(selectStack, coalescedNodes));
}

//when degree reduce, move activeMoves to worklistMoves
static void EnableMoves(G_nodeList nodes)
{
	for(;nodes;nodes = nodes->tail){
		Live_moveList moves = NodeMoves(nodes->head);
		for(;moves;moves = moves->tail){
			if(InMoveList(activeMoves, moves->src, moves->dst)){
				activeMoves = DiffMoveList(activeMoves, Live_MoveList(moves->src, moves->dst, NULL));
				worklistMoves = Live_MoveList(moves->src, moves->dst, worklistMoves);
			}
		}
	}
}


//reduce the degree of node
static void DecrementDegree(G_node node)
{
	printf("DecrementDegree:");
	printNodeList(G_NodeList(node, NULL));
	int* tmp_degree = G_look(degree, node);
	assert(tmp_degree != NULL);
	(*tmp_degree)--;

	//if K-1 add to simplify, if < K-1, it is related with move
	if ((*tmp_degree) == (K - 1)) {
		//printf("DecrementDegree 2\n");
		EnableMoves(G_NodeList(node, Adjacent(node)));
		//printf("DecrementDegree 3\n");
		spillWorklist = RemoveNodeList(spillWorklist, node); //remove the node from spillWorklist
		if (MoveRelated(node))
			freezeWorklist = G_NodeList(node, freezeWorklist);
		else
			simplifyWorklist = G_NodeList(node, simplifyWorklist);

	}
}

static bool InNodeList(G_nodeList nodes, G_node n)
{
	for (; nodes; nodes = nodes->tail) {
		if (nodes->head == n)
			return TRUE;
	}

	return FALSE;
}

static G_node GetAlias(G_node node)
{
	if (InNodeList(coalescedNodes, node)) {
		G_node tmp = G_look(alias, node);
		return GetAlias(tmp);  //find the origin node
	}
	else
		return node;
}



static bool InMoveList(Live_moveList moves, G_node src, G_node dst)
{
	for (; moves; moves = moves->tail) {
		if (moves->src == src && moves->dst == dst)
			return TRUE;
	}

	return FALSE;
}

static Live_moveList UnionMoveList(Live_moveList move1, Live_moveList move2)
{
	Live_moveList result = move1;

	for (; move2; move2 = move2->tail) {
		if (!InMoveList(move1, move2->src, move2->dst))
			result = Live_MoveList(move2->src, move2->dst, result);
	}

	return result;
}

static Live_moveList IntersectMoveList(Live_moveList move1, Live_moveList move2)
{
	Live_moveList result = NULL;
	for (; move2; move2 = move2->tail) {
		if (InMoveList(move1, move2->src, move2->dst))
			result = Live_MoveList(move2->src, move2->dst, result);
	}

	return result;
}

static G_nodeList RemoveNodeList(G_nodeList nodes, G_node n)
{
	G_nodeList result = nodes;

	if (nodes->head == n) {
		result = nodes->tail;
		return result;
	}

	G_nodeList tmp_nodes = nodes->tail;
	for (; tmp_nodes; tmp_nodes = tmp_nodes->tail) {
		if (tmp_nodes->head == n)
			nodes->tail = tmp_nodes->tail;
	}

	return result;
}

static G_nodeList DiffNodeList(G_nodeList nodes1, G_nodeList nodes2)
{
	G_nodeList result = NULL;
	for (; nodes1; nodes1 = nodes1->tail) {
		if (!InNodeList(nodes2, nodes1->head))
			result = G_NodeList(nodes1->head, result);
	}

	return result;
}

static G_nodeList UnionNodeList(G_nodeList nodes1, G_nodeList nodes2)
{
	G_nodeList result = nodes1;
	for (; nodes2; nodes2 = nodes2->tail) {
		if (!InNodeList(nodes1, nodes2->head))
			result = G_NodeList(nodes2->head, result);
	}

	return result;
}

//search in movelist
static Live_moveList NodeMoves(G_node n)
{
	Live_moveList move = G_look(moveList, n);
	Live_moveList uni = UnionMoveList(activeMoves, worklistMoves);

	return IntersectMoveList(uni, move);
}

//whether related to move instr
static bool MoveRelated(G_node n)
{
	return NodeMoves(n) != NULL;
}

static void Simplify()
{
	G_node n = simplifyWorklist->head;
	simplifyWorklist = simplifyWorklist->tail; //delete the node

	selectStack = G_NodeList(n, selectStack); //push the node to selectStack

	//reduce the degree of adjacent nodes
	for (G_nodeList nodes = Adjacent(n); nodes; nodes = nodes->tail)
		DecrementDegree(nodes->head);
}

//add to simplify worklist
static void AddWorklist(G_node n)
{
	if (!InNodeList(precolored, n) && !MoveRelated(n) && *(int*)G_look(degree, n) < K) {
		freezeWorklist = RemoveNodeList(freezeWorklist, n);
		simplifyWorklist = G_NodeList(n, simplifyWorklist);
	}
}


static bool OK(G_node v, G_node u)
{
	//check all adj of v
	for (G_nodeList nodes = Adjacent(v); nodes; nodes = nodes->tail) {
		if (*(int*)G_look(degree, nodes->head) < K || InNodeList(precolored, nodes->head) || InNodeList(G_adj(u), v))
			continue;
		else
			return FALSE;
	}

	return TRUE;
}

static void AddEdge(G_node u, G_node v)
{
	if (!InNodeList(G_adj(v), u) && u != v) {
		if (!InNodeList(precolored, u)) {
			G_addEdge(u, v);
			(*(int*)G_look(degree, u))++;
		}
		if (!InNodeList(precolored, v)) {
			G_addEdge(v, u);
			(*(int*)G_look(degree, v))++;
		}
	}
}

static void Combine(G_node u, G_node v)
{
	if (InNodeList(freezeWorklist, v))
		freezeWorklist = DiffNodeList(freezeWorklist, G_NodeList(v, NULL));
	else
		spillWorklist = DiffNodeList(spillWorklist, G_NodeList(v, NULL));

	printf("combine remove\n");
	coalescedNodes = G_NodeList(v, coalescedNodes);
	G_enter(alias, v, u);
	printf("combine 1\n");
	for (G_nodeList adj = Adjacent(v); adj; adj = adj->tail) {
		printf("combine 3\n");
		AddEdge(adj->head, u);
		printf("combine 4\n");
		DecrementDegree(adj->head);
		printf("combine 5\n");
	}

	//check the u node, fresh the freezeWorklist and spillWorklist
	int* tmp_degree = (int*)G_look(degree, u);
	if (*tmp_degree >= K && InNodeList(freezeWorklist, u)) {
		freezeWorklist = RemoveNodeList(freezeWorklist, u);
		spillWorklist = G_NodeList(u, spillWorklist);
	}
}

static bool Conservative(G_nodeList nodes)
{
	int k = 0;
	for(;nodes;nodes = nodes->tail){
		int* degree_num = (int*)G_look(degree, nodes->head);
		if((*degree_num) >= K)
			k++;
	}

	return k < K;
}

static void Coalesce()
{
	G_node x = worklistMoves->src;
	G_node y = worklistMoves->dst;
	G_node u, v;
	if (InNodeList(precolored, GetAlias(y))) {
		u = GetAlias(y);
		v = GetAlias(x);
	}
	else {
		u = GetAlias(x);
		v = GetAlias(y);
	}
	printf("Coalesce 1\n");
	worklistMoves = worklistMoves->tail; //delete the node

	if (u == v) {
		printf("Coalesce 2\n");
		coalescedMoves = Live_MoveList(x, y, coalescedMoves);
		AddWorklist(u);
	}
	else if (InNodeList(precolored, v) || InNodeList((G_nodeList)G_look(adjList, u), v)) {
		printf("Coalesce 3\n");
		constrainedMoves = Live_MoveList(x, y, constrainedMoves);
		AddWorklist(u);
		AddWorklist(v);
	}
	else if ((InNodeList(precolored, u) && OK(v, u)) ||
						(!InNodeList(precolored, u), Conservative(UnionNodeList(Adjacent(u),Adjacent(v))))) { //whether to combine
		printf("Coalesce 4\n");
		coalescedMoves = Live_MoveList(x, y, coalescedMoves);
		Combine(u, v);
		AddWorklist(u);

	}
	else {
		printf("Coalesce 5\n");
		activeMoves = Live_MoveList(x, y, activeMoves);
	}
}

static Live_moveList DiffMoveList(Live_moveList moves1, Live_moveList moves2)
{
	Live_moveList result = NULL;
	for (; moves1; moves1 = moves1->tail) {
		if (!InMoveList(moves2, moves1->src, moves2->dst))
			result = Live_MoveList(moves1->src, moves1->dst, result);
	}

	return result;
}

static void FreezeMoves(G_node u)
{
	//for all moves related to u
	for (Live_moveList m = NodeMoves(u); m; m = m->tail) {
		G_node x = m->src;
		G_node y = m->dst;
		G_node v;
		if (GetAlias(y) == GetAlias(u)) //if u is dst
			v = GetAlias(x);
		else
			v = GetAlias(y);

		activeMoves = DiffMoveList(activeMoves, Live_MoveList(x, y, NULL));
		frozenMoves = Live_MoveList(x, y, frozenMoves); // no consider coalescing

		//check and remove node from freezelist and add node to simplifylist
		if (NodeMoves(v) == NULL && (*(int*)G_look(degree, v)) < K) {
			freezeWorklist = RemoveNodeList(freezeWorklist, v);
			simplifyWorklist = G_NodeList(v, simplifyWorklist);
		}
	}
}

static void Freeze()
{
	G_node u = freezeWorklist->head;
	freezeWorklist = freezeWorklist->tail; //delete the node
	simplifyWorklist = G_NodeList(u, simplifyWorklist); //add it to simplify
	FreezeMoves(u); //check all moves of u
}

//select a node from spillWorklist,remove it and add it to simplify
static void SelectSpill()
{
	//select node whose degree is highest
	G_node m = NULL;
	int max_degree = 0;
	for (G_nodeList nodes = spillWorklist; nodes; nodes = nodes->tail) {
		int* tmp_degree = (int *)G_look(degree, nodes->head);
		if (*tmp_degree > max_degree) {
			max_degree = *tmp_degree;
			m = nodes->head;
		}
	}

	spillWorklist = RemoveNodeList(spillWorklist, m);
	simplifyWorklist = G_NodeList(m, simplifyWorklist);
	FreezeMoves(m);
}

static void AssignColors()
{
	printf("AssignColors\n");
	while (selectStack) {
		//pop the stack
		G_node n = selectStack->head;
		selectStack = selectStack->tail;

		//init okColors
		bool okColors[K];
		memset(okColors, TRUE, K);


		for (G_nodeList adj = G_look(adjList, n); adj; adj = adj->tail) {
			int* tmp_color = G_look(color, adj->head);
			if(tmp_color != NULL) okColors[*tmp_color] = FALSE;
		}

		//select color num
		int color_num;
		bool have_color = FALSE;
		for (color_num = 0; color_num < K; color_num++) {
			if (okColors[color_num]) {
				have_color = TRUE;
				break;
			}
		}

		if (have_color) {
			coloredNodes = G_NodeList(n, coloredNodes);
			int * tmp_color = checked_malloc(sizeof(int));
			G_enter(color, n, tmp_color);
		}
		else {
			spilledNodes = G_NodeList(n, spilledNodes);
		}
	}

	G_nodeList nodes = coalescedNodes;
	for(;nodes;nodes = nodes->tail){
		int* color_num = G_look(color, GetAlias(nodes->head));
		assert(color_num);
		G_enter(color, nodes->head, color_num);
	}
}

static void Build(G_graph ig, Temp_map allregs, Live_moveList moves)
{
	G_nodeList nodes = G_nodes(ig);

	//precolored
	for (; nodes; nodes = nodes->tail) {
		Temp_temp temp = Live_gtemp(nodes->head);
		if (Temp_look(allregs, temp) != NULL) { //precolored register
			precolored = G_NodeList(nodes->head, precolored);

			int *color_num = (int*)checked_malloc(sizeof(int));
			if (temp == F_RAX())
			{
				*color_num = 1;
			}
			else if (temp == F_R10())
			{
				*color_num = 2;
			}
			else if (temp == F_R11())
			{
				*color_num = 3;
			}
			else if (temp == F_RBX())
			{
				*color_num = 4;
			}
			else if (temp == F_R12())
			{
				*color_num = 5;
			}
			else if (temp == F_R13())
			{
				*color_num = 6;
			}
			else if (temp == F_R14())
			{
				*color_num = 7;
			}
			else if (temp == F_R15())
			{
				*color_num = 8;
			}
			else if (temp == F_RDI())
			{
				*color_num = 9;
			}
			else if (temp == F_RSI())
			{
				*color_num = 10;
			}
			else if (temp == F_RDX())
			{
				*color_num = 11;
			}
			else if (temp == F_RCX())
			{
				*color_num = 12;
			}
			else if (temp == F_R8())
			{
				*color_num = 13;
			}
			else if (temp == F_R9())
			{
				*color_num = 14;
			}
			else if (temp == F_RBP())
			{
				*color_num = 15;
			}
			else if (temp == F_RSP())
			{
				*color_num = 16;
			}
			else
			{
				assert(0);
			}

			G_enter(color, nodes->head, color_num);
		}
		else {
			//add node to initial, use it to make worklist
			initial = G_NodeList(nodes->head, initial);
		}
	}

	//printf("build precolored done\n");
	//adjList and degree
	nodes = G_nodes(ig);
	/*printf("====================nodes========================\n");
	printNodeList(nodes);*/
	for (; nodes; nodes = nodes->tail) {
		G_nodeList adjs = G_adj(nodes->head);
		G_enter(adjList, nodes->head, adjs);

		//degree
		int* degree_num = checked_malloc(sizeof(int));
		*degree_num = 0;
		for (; adjs; adjs = adjs->tail)
			(*degree_num)++;

		G_enter(degree, nodes->head, degree_num);
	}

	//printf("build degree done\n");

	//init worklistMoves
	Live_moveList tmp_moves = moves;
	for(; tmp_moves; tmp_moves = tmp_moves->tail){
		if(tmp_moves->src != tmp_moves->dst && !InMoveList(worklistMoves, tmp_moves->src, tmp_moves->dst)){
			worklistMoves = Live_MoveList(tmp_moves->src, tmp_moves->dst, worklistMoves);
		}
	}

	//init moveList: the moves related to a node
	tmp_moves = worklistMoves;
	for (; tmp_moves; tmp_moves = tmp_moves->tail) {
		Live_moveList old_src_move = G_look(moveList, tmp_moves->src);
		Live_moveList old_dst_move = G_look(moveList, tmp_moves->dst);

		G_enter(moveList, tmp_moves->src, Live_MoveList(tmp_moves->src, tmp_moves->dst, old_src_move));
		G_enter(moveList, tmp_moves->dst, Live_MoveList(tmp_moves->src, tmp_moves->dst, old_dst_move));
	}

	//printf("build moveList done\n");
}

static void MakeWorklist()
{
	//init spillWorklist freezeWorklist simplifyWorklist
	for (; initial; initial = initial->tail) {
		//check degree_num
		int* degree_num = (int*)G_look(degree, initial->head);
		if ((*degree_num) >= K) {
			spillWorklist = G_NodeList(initial->head, spillWorklist);
		}
		else if (MoveRelated(initial->head)) { //degree < K and move related
			freezeWorklist = G_NodeList(initial->head, freezeWorklist);
		}
		else {
			if(!InNodeList(simplifyWorklist, initial->head))
				simplifyWorklist = G_NodeList(initial->head, simplifyWorklist);
		}
	}
}

static void printNodeList(G_nodeList nodes)
{
	Temp_map tmp_map = Temp_layerMap(F_tempMap, Temp_name());

	for(;nodes;nodes = nodes->tail){
		printf("%s, ", Temp_look(tmp_map, (Temp_temp)G_nodeInfo(nodes->head)));
	}
	printf("\n");
}

static void printMoveList(Live_moveList moves)
{
	Temp_map tmp_map = Temp_layerMap(F_tempMap, Temp_name());

	for(;moves;moves = moves->tail){
		printf("%s->%s, ", Temp_look(tmp_map, (Temp_temp)G_nodeInfo(moves->src)),
												Temp_look(tmp_map, (Temp_temp)G_nodeInfo(moves->dst)));
	}
	printf("\n");
}

struct COL_result COL_color(G_graph ig, Temp_map allregs, Temp_tempList regs, Live_moveList moves) {
	//your code here.
	//init all data structure
	precolored = NULL;

	simplifyWorklist = NULL;
	freezeWorklist = NULL;
	spillWorklist = NULL;

	spilledNodes = NULL;
	coalescedNodes = NULL;
	coloredNodes = NULL;
	selectStack = NULL;

	coalescedMoves = NULL;
	constrainedMoves = NULL;
	frozenMoves = NULL;
	worklistMoves = NULL;
	activeMoves = NULL;

	degree = G_empty();
	alias = G_empty();
	moveList = G_empty();
	color = G_empty();
	adjList = G_empty();

	Build(ig, allregs, moves);
	printf("COL_color Build done\n");
	MakeWorklist();
	printf("COL_color MakeWorklist done\n");
	while (simplifyWorklist || worklistMoves || freezeWorklist || spillWorklist) {
		printf("==========================show info========================\n");
		printf("simplifyWorklist\n");
		printNodeList(simplifyWorklist);
		printf("worklistMoves\n");
		printMoveList(worklistMoves);
		printf("freezeWorklist\n");
		printNodeList(freezeWorklist);
		printf("spillWorklist\n");
		printNodeList(spillWorklist);
		printf("spilledNodes\n");
		printNodeList(spilledNodes);

		printf("coalescedNodes\n");
		printNodeList(coalescedNodes);
		printf("selectStack\n");
		printNodeList(selectStack);
		printf("coalescedMoves\n");
		printMoveList(coalescedMoves);
		printf("constrainedMoves\n");
		printMoveList(constrainedMoves);

		if (simplifyWorklist){
			printf("Simplify\n");
			Simplify();
		}
		else if (worklistMoves){
			printf("Coalesce\n");
			Coalesce();
		}
		else if (freezeWorklist){
			printf("Freeze\n");
			Freeze();
		}
		else if (spillWorklist){
			printf("SelectSpill\n");
			SelectSpill();
		}
	}
	printf("COL_color loop done\n");
	AssignColors();
	printf("COL_color AssignColors done\n");
	//coloring
	Temp_map coloring = Temp_empty();
	G_nodeList colored_nodes = UnionNodeList(coloredNodes, coalescedNodes);
	for (; colored_nodes; colored_nodes = colored_nodes->tail) {
		int* color_num = (int*)G_look(color, colored_nodes->head);

		//for debug
		if (!color_num) {
			printf("COL_color: can't find color_num");
			assert(0);
		}

		Temp_enter(coloring, Live_gtemp(colored_nodes->head), hard_regs[*color_num]);
	}

	//actual spilled temp
	Temp_tempList spills = NULL;
	G_nodeList tmp_spills = spilledNodes;
	for (; tmp_spills; tmp_spills = tmp_spills->tail)
		spills = Temp_TempList(Live_gtemp(tmp_spills->head), spills);


	struct COL_result ret;
	ret.coloring = coloring;
	ret.spills = spills;
	return ret;
}
