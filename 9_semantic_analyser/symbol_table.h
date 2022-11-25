#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include <stdio.h>
#include <vector>
#pragma warning(disable : 4786)


/* �洢���� */
enum e_StorageClass
{
	SC_GLOBAL =   0x00f0,		// �������������ͳ������ַ��������ַ�������,ȫ�ֱ���,��������
	SC_LOCAL  =   0x00f1,		// ջ�б���
	SC_LLOCAL =   0x00f2,       // �Ĵ���������ջ��
	SC_CMP    =   0x00f3,       // ʹ�ñ�־�Ĵ���
	SC_VALMASK=   0x00ff,       // �洢��������             
	SC_LVAL   =   0x0100,       // ��ֵ       
	SC_SYM    =   0x0200,       // ����	

	SC_ANOM	  = 0x10000000,     // ��������
	SC_STRUCT = 0x20000000,     // �ṹ�����
	SC_MEMBER = 0x40000000,     // �ṹ��Ա����
	SC_PARAMS = 0x80000000,     // ��������
};

/* ���ͱ��� */
enum e_TypeCode
{
	T_INT    =  0,			// ����                     
	T_CHAR   =  1,			// �ַ���                 
	T_SHORT  =  2,			// ������                       
	T_VOID   =  3,			// ������                        
	T_PTR    =  4,			// ָ��                          
	T_FUNC   =  5,			// ����                    
	T_STRUCT =  6,			// �ṹ�� 
	
	T_BTYPE  =  0x000f,		// ������������          
	T_ARRAY  =  0x0010,		// ����
};

#define ALIGN_SET 0x100  // ǿ�ƶ����־

/* ���ʹ洢�ṹ���� */
typedef struct structType
{
    // e_TypeCode t;
	int    type;
    struct Symbol *ref;
} Type;

/* ���Ŵ洢�ṹ���� */
typedef struct Symbol 
{
    int token_code;				// ���ŵĵ��ʱ��� e_TokenCode
    int storage_class;			// ���Ź����ļĴ����洢���� e_StorageClass
    int related_value;			// ���Ź���ֵ��
	                            // �������ֵ���ҹ۲죬Ӧ������Ǳ���������¼������ֵ��
	                            // ����ǽṹ�壬��¼�ṹ��Ĵ�С����������飬��¼����Ԫ�ظ�����
								// ����Ǿ�̬�ַ�������ᱻ��������ָ�롣��ʱ�����ֵΪ-1��
    Type typeSymbol;			// ��������
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

/* �Ĵ������� */
// �����ȡֵ��д���ġ��μ����ϵı�8.4��
// Ҳ����Intel��Ƥ��ĵڶ���ĵڶ��µ�Table 2-2��
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
	/* ���ֵ���������ϵı�8.4�����߲鿴Intel��Ƥ��509ҳ�ĺ�8�С�     */
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

int type_size(Type * type, int * align);
int calc_align(int n , int align);

Type *pointed_type(Type *t);
int pointed_size(Type *t);

void init();
#endif
