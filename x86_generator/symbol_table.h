#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include <stdio.h>
#include <vector>
#pragma warning(disable : 4786)
#include "translation_unit.h"
#include "coff_generator.h"



typedef struct Operand_t{
	Type type;              // ��������
	unsigned short r;       // �Ĵ�����洢����
	int value;              // ����ֵ
	struct Symbol * sym;    // ��������
} Operand;

/* �Ĵ������� */
enum e_Register
{
    REG_EAX = 0,
    REG_ECX,
    REG_EDX,
	REG_EBX,
	REG_ESP,
	REG_EBP,
	REG_ESI,
	REG_EDI,
	REG_ANY
};
#define REG_IRET  REG_EAX	// ��ź�������ֵ�ļĴ���

enum e_AddrForm
{
	ADDR_OTHER,
	ADDR_REG = 3
};

#endif
