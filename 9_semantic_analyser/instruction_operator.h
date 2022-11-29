#ifndef INSTRUCTION_OPERATOR_H
#define INSTRUCTION_OPERATOR_H

#include "operand_stack.h"
// 生成二元运算
void gen_op(int op);
// 生成函数调用
void gen_prolog(Type *func_type);
void gen_epilog();
void backpatch(int t, int a);
// 生成跳转
void gen_jmpbackward(int a);
int gen_jmpforward(int t);
int gen_jcc(int t);
// 生成赋值
int load_one(int rc, Operand * opd);
void load_two(int rc1, int rc2);
void store_zero_to_one();
void array_initialize();

#endif