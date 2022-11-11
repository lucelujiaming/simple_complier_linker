#ifndef X86_GENERATOR_H
#define X86_GENERATOR_H

typedef struct Operand_t{
	Type type;              // 数据类型
	unsigned short r;       // 寄存器或存储类型
	int value;              // 常量值
	struct Symbol * sym;    // 关联符号
} Operand;

void operand_push(Type* type, int r, int value);
void operand_pop();
void operand_swap();
void operand_assign(Operand *opd, int t, int r, int v);
void cancel_lvalue();
void indirection();

#endif
