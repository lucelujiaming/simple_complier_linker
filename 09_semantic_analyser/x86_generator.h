#ifndef X86_GENERATOR_H
#define X86_GENERATOR_H

#include "operand_stack.h"
#include "opcode_generator.h"


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



void gen_modrm(int mod, int reg_opcode, int r_m, Symbol * sym, int value);
void load(int r, Operand * opd);
void store(int r, Operand * opd);
void gen_opInteger(int op);
void gen_opTwoInteger(int opc, int op);
void gen_addsp(int val);
void gen_invoke(int nb_args);
void gen_call();

#endif
