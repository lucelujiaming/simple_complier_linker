#ifndef X86_GENERATOR_H
#define X86_GENERATOR_H

typedef struct Operand_t{
	Type type;              // ��������
	unsigned short r;       // �Ĵ�����洢����
	int value;              // ����ֵ
	struct Symbol * sym;    // ��������
} Operand;

void operand_push(Type* type, int r, int value);
void operand_pop();
void operand_swap();
void operand_assign(Operand *opd, int t, int r, int v);
void cancel_lvalue();
void indirection();

#endif
