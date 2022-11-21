#ifndef OPERAND_STACK_H
#define OPERAND_STACK_H

#include "get_token.h"

#define OPSTACK_SIZE 256 // + 1

typedef struct Operand_t{
	Type type;							// 数据类型
	unsigned short storage_class;       // 寄存器或存储类型
	int value;							// 常量值
	struct Symbol * sym;				// 关联符号
} Operand;

// Operand operation functions
void operand_push(Type* type, int r, int value);
void operand_pop();
void operand_swap();
void operand_assign(Operand *opd, int t, int r, int v);

#endif

