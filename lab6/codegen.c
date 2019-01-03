#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "codegen.h"
#include "table.h"

static AS_instrList iList, last;
static F_frame frame;

static void emit(AS_instr inst); // add inst to iList
static void munchStm(T_stm stm);
static Temp_temp munchExp(T_exp exp);

// temp or const
#define ISTC(kind) (kind == T_CONST || kind == T_TEMP)
#define MAXNUM 100

#define ARGREGS Temp_TempList(F_RDI(), Temp_TempList(F_RSI(), Temp_TempList(F_RDX(), Temp_TempList(F_RCX(), Temp_TempList(F_R8(),TL(F_R9(), NULL))))))
#define CALLER_SAVE Temp_TempList(F_RAX(), Temp_TempList(F_R10(), Temp_TempList(F_R11(), ARGREGS)))
#define CALLEE_SAVE Temp_TempList(F_RBX(), Temp_TempList(F_RBP(), Temp_TempList(F_R12(), Temp_TempList(F_R13(), Temp_TempList(F_R14(),Temp_TempList(F_R15(), NULL))))))

// add inst to iList
static void emit(AS_instr inst)
{
	if (last != NULL)
		last = last->tail = AS_InstrList(inst, NULL);
	else last = iList = AS_InstrList(inst, NULL); //first add
}

//
static Temp_tempList munchArgs(T_expList args, int index)
{
	if (!args) return NULL;

	Temp_temp src = munchExp(args->head);
	if (index < 6) {
		Temp_temp dst_reg = F_ArgReg(index);
		Temp_tempList temp_args = Temp_TempList(dst_reg, munchArgs(args->tail, index));
		emit(AS_Move("movq `s0, `d0", Temp_TempList(dst_reg, NULL), Temp_TempList(src, NULL)));
		return temp_args;
	}
	else {
		emit(AS_Oper("pushq `s0", Temp_TempList(F_RSP(),NULL), Temp_TempList(src, Temp_TempList(F_RSP(), NULL)), AS_Targets(NULL)));
		return munchArgs(args->tail, index + 1);;
	}
}

static Temp_temp munchExp(T_exp exp)
{
	Temp_temp result = Temp_newtemp();
	switch (exp->kind)
	{
	case T_BINOP: {
		T_binOp op = exp->u.BINOP.op;
		Temp_temp left_temp = munchExp(exp->u.BINOP.left);
		Temp_temp right_temp = munchExp(exp->u.BINOP.right);
		string tmp_inst;
		switch (op)
		{
		case T_plus:tmp_inst = "addq `s0, `d0"; break;
		case T_minus:tmp_inst = "subq `s0, `d0"; break;
		case T_mul:tmp_inst = "imulq `s0, `d0"; break;
		case T_div:
			//TODO:
			emit(AS_Move("movq `s0, `d0", Temp_TempList(F_RAX(), NULL), Temp_TempList(left_temp, NULL)));
			emit(AS_Oper("cqto", Temp_TempList(F_RAX(), Temp_TempList(F_RDX(), NULL)), NULL, NULL)); //extend rdx:rax
			emit(AS_Oper("idivq `s0", Temp_TempList(F_RAX(), NULL),
						Temp_TempList(right_temp, Temp_TempList(F_RAX(),Temp_TempList(F_RDX(), NULL))), NULL));
			return F_RAX();
			break;
		//tiger language has no logic op
		case T_and:
		case T_or:
		case T_lshift:
		case T_rshift:
		case T_arshift:
		case T_xor:
			assert(0);
		}
		emit(AS_Move("movq `s0, `d0", Temp_TempList(result, NULL), Temp_TempList(left_temp, NULL)));
		emit(AS_Oper(tmp_inst, Temp_TempList(result, NULL),
			Temp_TempList(right_temp, Temp_TempList(result, NULL)),
			NULL));
		break;
	}

	case T_MEM:
		emit(AS_Oper("movq (`s0), `d0", Temp_TempList(result, NULL), Temp_TempList(munchExp(exp->u.MEM), NULL), NULL));
		break;
	case T_TEMP: {
		if (exp->u.TEMP == F_FP()) { //FP = rps + framesize
			emit(AS_Oper("movq `s0, `d0", Temp_TempList(result, NULL), Temp_TempList(F_RSP(), NULL), AS_Targets(NULL)));
			char * inst = checked_malloc(MAXNUM * sizeof(char));
			sprintf(inst, "addq $%sframesize, `d0", Temp_labelstring(F_name(frame)));
			emit(AS_Oper(inst, Temp_TempList(result, NULL), NULL, AS_Targets(NULL)));
		}
		else result = exp->u.TEMP;
		break;
	}
	case T_ESEQ: assert(0); //no ESEQ
	case T_NAME: {
		char* tmp_ins = checked_malloc(MAXNUM);
		sprintf(tmp_ins, "movq $%s, `d0", Temp_labelstring(exp->u.NAME));
		emit(AS_Move(tmp_ins, Temp_TempList(result, NULL), NULL));
		break;
	}
	case T_CONST: {
		char* tmp_ins = checked_malloc(MAXNUM);
		sprintf(tmp_ins, "movq $%d, `d0", exp->u.CONST);
		emit(AS_Move(tmp_ins, Temp_TempList(result, NULL), NULL));
		break;
	}
	case T_CALL: {
		T_exp func_exp = exp->u.CALL.fun;
		Temp_tempList regs = munchArgs(exp->u.CALL.args, 0);
		result = F_RV();// reg rax
		char *call_inst = checked_malloc(MAXNUM);
		sprintf(call_inst, "call %s", Temp_labelstring(func_exp->u.NAME));
		emit(AS_Oper(call_inst, Temp_TempList(result, NULL), regs, NULL));
		break;
	}
	default:
		assert(0);
		break;
	}
	return result;
}

static void munchStm(T_stm stm)
{
	switch (stm->kind){
	case T_SEQ: {
		assert(0);
		break;
	}
	case T_LABEL: {
		Temp_label lab = stm->u.LABEL;
		emit(AS_Label(Temp_labelstring(lab), lab));
		break;
	}
	case T_JUMP: {
		emit(AS_Oper("jmp `j0", NULL, NULL, AS_Targets(stm->u.JUMP.jumps)));
		break;
	}
	case T_CJUMP: {
		T_relOp op = stm->u.CJUMP.op;

		string tmpStr;
		switch (op){
			case T_eq:
				tmpStr = "je `j0";
				break;
			case T_ne:
				tmpStr = "jne `j0";
				break;
			case T_lt:
				tmpStr = "jl `j0";
				break;
			case T_gt:
				tmpStr = "jg `j0";
				break;
			case T_le:
				tmpStr = "jle `j0";
				break;
			case T_ge:
				tmpStr = "jge `j0";
				break;
			case T_ult:
				tmpStr = "jb `j0"; //jb: below
				break;
			case T_ule:
				tmpStr = "jbe `j0";
				break;
			case T_ugt:
				tmpStr = "ja `j0";
				break;
			case T_uge:
				tmpStr = "jae `j0";
				break;
			default:
				assert(0);
				break;
		}
		//AS_targets: Temp_labellist
		emit(AS_Oper(tmpStr, NULL, NULL, AS_Targets(Temp_LabelList(stm->u.CJUMP.true, NULL))));
		break;
	}
	case T_MOVE: {//dst must be mem or temp
		Temp_temp temp_src = munchExp(stm->u.MOVE.src);
		T_exp dst = stm->u.MOVE.dst;

		if (dst->kind == T_TEMP) {
			emit(AS_Move("movq `s0, `d0", Temp_TempList(dst->u.TEMP, NULL), Temp_TempList(temp_src, NULL)));
			return;
		}
		else if (dst->kind == T_MEM) {
			T_exp tmpMem = dst->u.MEM;
			switch (tmpMem->kind){
			case T_TEMP: {
				emit(AS_Move("movq `s0 (`s1)", NULL, Temp_TempList(temp_src, Temp_TempList(tmpMem->u.TEMP, NULL))));
				break;
			}
			case T_CONST: {
				Temp_temp cons = munchExp(tmpMem);
				emit(AS_Move("MOVQ `s0 (`s1)", NULL, Temp_TempList(temp_src, Temp_TempList(cons, NULL))));
				break;
			}
			case T_BINOP: { //BINOP(o, e1, e2)
				T_binOp op = tmpMem->u.BINOP.op;
				T_exp leftExp = tmpMem->u.BINOP.left;
				T_exp rightExp = tmpMem->u.BINOP.right;

				string ins = checked_malloc(MAXNUM);
				if (rightExp->kind == T_CONST) {
					sprintf(ins, "movq `s0 %d(`s1)", rightExp->u.CONST);
					emit(AS_Oper(ins, NULL, Temp_TempList(temp_src, Temp_TempList(munchExp(leftExp), NULL)), NULL));
				}
				else if (leftExp->kind == T_CONST) {
					sprintf(ins, "movq `s0 %d(`s1)", leftExp->u.CONST);
					emit(AS_Oper(ins, NULL, Temp_TempList(temp_src, Temp_TempList(munchExp(rightExp), NULL)), NULL));
				}
				else assert(0);

				break;
			}
			default:
				assert(0);
				break;
			}
		}
		else assert(0); //dst must be mem or temp
		break;
	}
	case T_EXP: {
		Temp_temp src = munchExp(stm->u.EXP);
		Temp_temp dst = Temp_newtemp();
		emit(AS_Move("movq `s0, `d0", Temp_TempList(dst, NULL), Temp_TempList(src, NULL)));
		break;
	}
	default:
		break;
	}
}

static AS_instr StoreTemp(Temp_temp src,Temp_temp dst)
{
	char ins[MAXNUM];
	sprintf(ins,"moveq `s0, `d0");
	return AS_Move(String(ins),Temp_TempList(dst, NULL), Temp_TempList(src, NULL));
}
//Lab 6: put your code here
AS_instrList F_codegen(F_frame f, T_stmList stmList) {
	iList = NULL;
	frame = f;

	F_accessList formals = f->formals;
	int index = 0;

	for (; formals; formals = formals->tail, index++) {
		Temp_temp arg_temp;
		switch (index) {
			case 0:
				arg_temp = F_RDI();
				break;
			case 1:
				arg_temp = F_RSI();
				break;
			case 2:
				arg_temp = F_RDX();
				break;
			case 3:
				arg_temp = F_RCX();
				break;
			case 4:
			  arg_temp = F_R8();
				break;
			case 5:
				arg_temp = F_R9();
				break;
			default:
				continue;
		}
		if (formals->head->kind == inFrame) {
			T_stm stm = T_Move(T_Mem(T_Binop(T_plus, T_Const(formals->head->u.offset), T_Temp(F_FP()))), T_Temp(arg_temp));
			munchStm(stm);
		}
		else { // kind == InReg
			emit(StoreTemp(arg_temp, formals->head->u.reg));
		}
	}

	//callee save, put it all in new temp
	/*Temp_tempList tmp_callee = CALLEE_SAVE;
	Temp_tempList it_callee = Temp_TempList(Temp_newtemp(), NULL);
	for (; tmp_callee; tmp_callee = tmp_callee->tail) {
		emit(AS_Move("movq `s0, `d0", Temp_TempList(it_callee, NULL), Temp_TempList(it_callee->head, NULL)));
		it_callee = it_callee->tail = Temp_TempList(Temp_newtemp(), NULL);
	}*/


	//callee save registers, store them in temps
	Temp_temp r12_temp = Temp_newtemp();
	Temp_temp r13_temp = Temp_newtemp();
	Temp_temp r14_temp = Temp_newtemp();
	Temp_temp r15_temp = Temp_newtemp();
	Temp_temp rbx_temp = Temp_newtemp();
	emit(StoreTemp(F_R12(), r12_temp));
	emit(StoreTemp(F_R13(), r13_temp));
	emit(StoreTemp(F_R14(), r14_temp));
	emit(StoreTemp(F_R15(), r15_temp));
	emit(StoreTemp(F_RBX(), rbx_temp));

	//munch stm
	while (stmList) {
		munchStm(stmList->head);
		stmList = stmList->tail;
	}

	//restore callee save
	emit(StoreTemp(r12_temp, F_R12()));
	emit(StoreTemp(r13_temp, F_R13()));
	emit(StoreTemp(r14_temp, F_R14()));
	emit(StoreTemp(r15_temp, F_R15()));
	emit(StoreTemp(rbx_temp, F_RBX()));

	return iList;
}
