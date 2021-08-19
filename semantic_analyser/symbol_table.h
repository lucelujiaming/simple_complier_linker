#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include <stdio.h>
#include <vector>
#pragma warning(disable : 4786)
#include "translation_unit.h"



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

Symbol * sym_search(int v);
void sym_pop(std::vector<Symbol> * pop, Symbol *b);
Symbol * sym_push(int v, Type * type, int r, int c);
Symbol * sym_direct_push(std::vector<Symbol> &ss, int v, Type * type, int c);
Symbol * var_sym_put(Type * type, int r, int v, int addr);
Symbol * struct_search(int v);
Symbol * func_sym_push(int v, Type * type);
#endif
