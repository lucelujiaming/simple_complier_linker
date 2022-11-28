#ifndef OPERAND_STACK_H
#define OPERAND_STACK_H

#include "get_token.h"

#define OPSTACK_SIZE 256 // + 1

typedef struct Operand_t{
	Type type;						// ��������
	unsigned short storage_class;   // �Ĵ�����洢����
	
	int value;						// ����ֵ��
									// �����������ļĴ�����洢�����ǣ�
	                                // 1. ���������Ǿ�������������
	                                // 2. ջ�б������Ǿ���ջ�ڵ�ַ��
	                                // 3. �Ĵ���������ջ�У��Ǿ��Ǿֲ�������ջ��λ�á�
	                                // 4. ʹ�ñ�־�Ĵ�����
	                                //    �Ǿ�����gen_opTwoInteger�����ɵıȽϽ����
									
	struct Symbol * sym;			// ��������
} Operand;

// Operand operation functions
void operand_push(Type* type, int r, int value);
void operand_pop();
void operand_swap();
void operand_assign(Operand *opd, int t, int r, int v);

#endif

