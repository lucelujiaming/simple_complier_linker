#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include <stdio.h>
#include <vector>
#pragma warning(disable : 4786)

/* ���ʹ洢�ṹ���� */
typedef struct structType
{
    // e_TypeCode t;
	int    t;
    struct Symbol *ref;
} Type;

/* ���Ŵ洢�ṹ���� */
typedef struct Symbol 
{
    int v;						// ���ŵĵ��ʱ��� e_TokenCode
    int r;						// ���Ź����ļĴ����洢���� e_StorageClass
    int c;						// ���Ź���ֵ��
	                            // �������ֵ���ҹ۲죬Ӧ������Ǳ���������¼������ֵ��
	                            // ����ǽṹ�壬��¼�ṹ��Ĵ�С����������飬��¼����Ԫ�ظ�����
    Type type;					// ��������
    struct Symbol *next;		// �������������ţ��ṹ�嶨�������Ա�������ţ��������������������
    struct Symbol *prev_tok;	// ָ��ǰһ�����ͬ������
} Symbol;

/* ���ʴ洢�ṹ���� */
/* ˵һ�������� */
typedef struct TkWord
{
    int  tkcode;					// ���ʱ��� 
    struct TkWord *next;			// ָ���ϣ��ͻ����������
    char *spelling;					// �����ַ��� 
    struct Symbol *sym_struct;		// ָ�򵥴�����ʾ�Ľṹ����
    struct Symbol *sym_identifier;	// ָ�򵥴�����ʾ�ı�ʶ��
} TkWord;

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

Symbol * struct_search(int v);
Symbol * sym_search(int v);
void sym_pop(std::vector<Symbol> * pop, Symbol *b);
Symbol * sym_push(int v, Type * type, int r, int c);

Symbol * sym_direct_push(std::vector<Symbol> &ss, int v, Type * type, int c);
Symbol * func_sym_push(int v, Type * type);

Symbol * var_sym_put(Type * type, int r, int v, int addr);
Symbol * sec_sym_put(char * sec, int c);

void print_all_stack(char* strPrompt);
void mk_pointer(Type *t);
#endif
