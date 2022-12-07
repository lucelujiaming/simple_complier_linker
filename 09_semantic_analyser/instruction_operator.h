#ifndef INSTRUCTION_OPERATOR_H
#define INSTRUCTION_OPERATOR_H

#include "operand_stack.h"
// ���ɶ�Ԫ����
void gen_op(int op);
// ���ɺ�������
void gen_prologue(Type *func_type);
void gen_epilogue();
// ������ת
void gen_jmpbackward(int target_address);
int gen_jmpforward(int target_address);
int gen_jcc(int jmp_addr);
// ���ɸ�ֵ
int load_one(int rc, Operand * opd);
void load_two(int rc1, int rc2);
void store_zero_to_one();
void array_initialize();

#endif