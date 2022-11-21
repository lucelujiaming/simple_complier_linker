#ifndef X86_GENERATOR_H
#define X86_GENERATOR_H

#include "operand_stack.h"

// Operand functions
void gen_byte(char c);
void gen_prefix(char opcode);
void gen_opcodeOne(unsigned char opcode);
void gen_opcodeTwo(unsigned char first, unsigned char second);
void gen_modrm(int mod, int reg_opcode, int r_m, Symbol * sym, int c);
void gen_dword(unsigned int c);
void gen_addr32(int r, Symbol * sym, int c);
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
void spill_reg(char r);
void spill_regs();

void gen_prolog(Type *func_type);
void gen_epilog();

int gen_jcc(int t);
int gen_jmpforward(int t);
void gen_jmpbackward(int a);
Type *pointed_type(Type *t);

int allocate_reg(int rc);
#endif
