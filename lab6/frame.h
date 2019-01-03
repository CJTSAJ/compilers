
/*Lab5: This header file is not complete. Please finish it with more definition.*/

#ifndef FRAME_H
#define FRAME_H

#include "tree.h"
#include "assem.h"

Temp_map F_tempMap;

typedef struct F_frame_ *F_frame;

typedef struct F_access_ *F_access;
typedef struct F_accessList_ *F_accessList;

struct F_accessList_ {F_access head; F_accessList tail;};

struct F_frame_{
	Temp_label name;
	F_accessList formals;
	F_accessList locals;
	int argSize;
	int stackOff;
};

//varibales
struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset; //inFrame
		Temp_temp reg; //inReg
	} u;
};

//my code
F_accessList F_AccessList(F_access head, F_accessList tail);
F_frame F_newFrame(Temp_label name, U_boolList formals);
Temp_label F_name(F_frame f);
F_accessList F_formals(F_frame f);
F_access F_allocLocal(F_frame f, bool escape);

//reg tempmap
Temp_map Reg_map;

//current value of fp(use temp express) it's too early to know exact value of reg
Temp_temp F_FP(void);
extern const int F_wordSize;
T_exp F_Exp(F_access access, T_exp fp);
/* declaration for fragments */
typedef struct F_frag_ *F_frag;
struct F_frag_ {enum {F_stringFrag, F_procFrag} kind;
			union {
				struct {Temp_label label; string str;} stringg;
				struct {T_stm body; F_frame frame;} proc;
			} u;
};

void F_init(void); //init all register temp
F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);
Temp_temp F_RV(void); //the return reg
Temp_temp F_ArgReg(int); //alloac the regs  for args

typedef struct F_fragList_ *F_fragList;
struct F_fragList_
{
	F_frag head;
	F_fragList tail;
};

T_exp F_externalCall(string s, T_expList args);
F_fragList F_FragList(F_frag head, F_fragList tail);

Temp_temp F_RAX(void);
Temp_temp F_RBX(void);
Temp_temp F_RCX(void);
Temp_temp F_RDX(void);
Temp_temp F_RSI(void);
Temp_temp F_RDI(void);
Temp_temp F_RBP(void);
Temp_temp F_RSP(void);
Temp_temp F_R8(void);
Temp_temp F_R9(void);
Temp_temp F_R10(void);
Temp_temp F_R11(void);
Temp_temp F_R12(void);
Temp_temp F_R13(void);
Temp_temp F_R14(void);
Temp_temp F_R15(void);

AS_proc F_procEntryExit3(F_frame f, AS_instrList body);

#endif
