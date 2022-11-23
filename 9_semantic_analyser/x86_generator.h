#ifndef X86_GENERATOR_H
#define X86_GENERATOR_H

#include "operand_stack.h"
#include "opcode_generator.h"


#define OPCODE_STORE_T_SHORT_PREFIX			0x66
#define OPCODE_STORE_CHAR_OPCODE			0x88
#define OPCODE_STORE_SHORT_INT_OPCODE       0x89

#define OPCODE_IMUL_HIGH_BYTE  				0x0f
#define OPCODE_IMUL_LOW_BYTE   				0xaf
#define OPCODE_CDQ             				0x99
#define OPCODE_IDIV            				0xf7


extern std::vector<Operand> operand_stack;
extern std::vector<Operand>::iterator operand_stack_top ;
extern std::vector<Operand>::iterator operand_stack_last_top ;

extern std::vector<Section> vecSection;
extern  Section *sec_text,			// 代码节
		*sec_data,			// 数据节
		*sec_bss,			// 未初始化数据节
		*sec_idata,			// 导入表节
		*sec_rdata,			// 只读数据节
		*sec_rel,			// 重定位信息节
		*sec_symtab,		// 符号表节	
		*sec_dynsymtab;		// 链接库符号节



void gen_modrm(int mod, int reg_opcode, int r_m, Symbol * sym, int c);
void load(int r, Operand * opd);
void store(int r, Operand * opd);
int load_one(int rc, Operand * opd);
void load_two(int rc1, int rc2);
void store_zero_to_one();
void gen_op(int op);
void gen_opInteger(int op);
void gen_opTwoInteger(int opc, int op);
void backpatch(int t, int a);
void gen_jmpbackward(int a);
void gen_addsp(int val);
void gen_invoke(int nb_args);
void gen_call();

void gen_prolog(Type *func_type);
void gen_epilog();

int gen_jcc(int t);
int gen_jmpforward(int t);
void gen_jmpbackward(int a);
Type *pointed_type(Type *t);

#endif
