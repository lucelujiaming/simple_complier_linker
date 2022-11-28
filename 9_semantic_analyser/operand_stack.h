#ifndef OPERAND_STACK_H
#define OPERAND_STACK_H

#include "get_token.h"

#define OPSTACK_SIZE 256 // + 1

typedef struct Operand_t{
	Type type;						// 数据类型
	unsigned short storage_class;   // 寄存器或存储类型
	
	int value;						// 常量值。
									// 如果这个变量的寄存器或存储类型是：
	                                // 1. 立即数，那就是立即数本身。
	                                // 2. 栈中变量，那就是栈内地址。
	                                // 3. 寄存器溢出存放栈中，那就是局部变量在栈中位置。
	                                // 4. 使用标志寄存器，
	                                //    那就是在gen_opTwoInteger中生成的比较结果。
									
	struct Symbol * sym;			// 关联符号
} Operand;

// Operand operation functions
void operand_push(Type* type, int r, int value);
void operand_pop();
void operand_swap();
void operand_assign(Operand *opd, int t, int r, int v);

#endif

