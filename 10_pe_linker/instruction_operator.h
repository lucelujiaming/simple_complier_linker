#ifndef INSTRUCTION_OPERATOR_H
#define INSTRUCTION_OPERATOR_H

#include "operand_stack.h"
// 生成二元运算
void gen_op(int op);
// 生成函数调用
void gen_prologue(Type *func_type);
void gen_epilogue();
void gen_invoke(int nb_args);
// 生成跳转
void gen_jmpbackward(int target_address);
int gen_jmpforward(int target_address);
int gen_jcc(int jmp_addr);
// 生成赋值
int load_one(int rc, Operand * opd);
void load_two(int rc1, int rc2);
void store_zero_to_one();
void array_initialize();

#endif