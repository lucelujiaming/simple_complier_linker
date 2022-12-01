#ifndef OPERAND_STACK_H
#define OPERAND_STACK_H

#include "get_token.h"

#define OPSTACK_SIZE 256 // + 1

typedef struct Operand_t{
	Type type;						// ��������
	unsigned short storage_class;   // �Ĵ�����洢���� e_StorageClass
	
	int operand_value;				// ����ֵ��������SC_GLOBAL��
									// �����������ļĴ�����洢�����ǣ�
	                                // 1. ���������Ǿ�������������
	                                // 2. ջ�б������Ǿ���ջ�ڵ�ַ��
	                                // 3. �Ĵ���������ջ�У��Ǿ��Ǿֲ�������ջ��λ�á�
	                                // 4. ʹ�ñ�־�Ĵ�����
	                                //    �Ǿ�����gen_opTwoInteger�����ɵıȽϽ����
									// ����Ƿ������ã�����ֵ��Ч��
	struct Symbol * sym;			// �������ţ�������(SC_SYM | SC_GLOBAL)
	// ����������
	char token_str[128];
} Operand;

// Operand operation functions
void operand_push(Type* type, int r, int operand_value);
void operand_pop();
void operand_swap();
void operand_assign(Operand *opd, int token_code, int storage_class, int operand_value);

#endif

