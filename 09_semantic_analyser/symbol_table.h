#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include <stdio.h>
#include <vector>
#pragma warning(disable : 4786)

/* �Ĵ������� */
// �����ȡֵ��д���ġ��μ����ϵı�8.4��
// Ҳ����Intel��Ƥ��ĵڶ���ĵڶ��µ�Table 2-2��
enum e_Register
{
    REG_EAX = 0,  // ǰ�ĸ��ǿ���ʹ�õĻ����Ĵ�����
    REG_ECX,
    REG_EDX,
	REG_EBX,
	              // ����ļĴ�����
	REG_ESP,      //   ���ں������õĶ�ջ���μ�gen_epilog��ʵ�֡�
	REG_EBP,      //   ���ں������õĶ�ջ���μ�gen_epilog��ʵ�֡�
	REG_ESI,      //   ��MOVSָ��ʹ�á������ַ�����ֵ��
	REG_EDI,      //   ��MOVSָ��ʹ�á������ڳ�����
	REG_ANY       // ����Ļ����Ĵ�������EAX��ECX��EDX��EBX��
};

#define REG_IRET  REG_EAX	// ��ź�������ֵ�ļĴ���

/* �洢���� */
/* ���ֵ��һ��32λ��������Ϊ�������֣�
   ��8λΪ�������͡�����F0��F1��F2��F3�Ǳ����֡�������ʾ��
   ֮���8λ����ߵ�8λ������ʾ��չ���͡�
*/
enum e_StorageClass
{
// ��8λΪ�������͡�
	// �����SC_GLOBALС����С��0x2C(44)��˵���洢�ڼĴ����С�ȡֵ���£�
	// REG_EAX = 0, REG_ECX,    
    // REG_EDX,     REG_EBX,
	// REG_ESP,     REG_EBP,    
	// REG_ESI,     REG_EDI,
	// REG_ANY
	SC_GLOBAL  = 0x00f0,		 // �������������ͳ������ַ�������
								 //       �ַ���������ȫ�ֱ�������������
	SC_LOCAL   = 0x00f1,		 // ջ�б���
	SC_LLOCAL  = 0x00f2,   		 // �Ĵ���������ջ�С���ֵ��ʹ���������£�
	                             // 1. ���һ������������ֵ�������Ҫ�����������ڴ�ջ�С�
								 //    ��spill_reg�У����ǻ����store���ɽ����Ļ����롣
								 //    ֮�����ǽ��ѻ���������ΪSC_LLOCAL��������չ���͡�
								 // 2. �����ǽ�ջ�������������ջ���������е�ʱ��
								 //    ������ִ�ջ����������λ��λ�����˵����
								 //    ��ֵ���ڱ������ջ�У������ٴΰ������ص��Ĵ����С�
								 //    ��store_zero_to_one�У����ǻ����load���ɼ��صĻ����롣
								 //    ֮�����ǽ�������������Ϊָ���ļĴ�����
								 //    �������չ���ͣ�ֻ��λSC_LVAL��
	SC_CMP     = 0x00f3,   		 // ʹ�ñ�־�Ĵ���
	SC_VALMASK = 0x00ff,   		 // �洢��������

// ֮���8λ����ߵ�8λ������ʾ��չ���͡�
	SC_LVAL    = 0x0100,   		 // ��ֵ
	SC_SYM     = 0x0200,   		 // ����

	SC_ANOM	   = 0x10000000,     // ��������
	SC_STRUCT  = 0x20000000,     // �ṹ�����
	SC_MEMBER  = 0x40000000,     // �ṹ��Ա����
	SC_PARAMS  = 0x80000000,     // ��������
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
// ����ṹ���������÷���
//   һ������ΪType���Ͷ�Ӧ�ķ��š�
//   һ������ΪOperand��������Ӧ�ķ��š�
//   һ������ΪTkWord���ʴ洢�ṹ��Ӧ�ķ��š�
typedef struct Symbol
{
    int token_code;				// ���ŵĵ��ʱ��� e_TokenCode
    int storage_class;			// ���Ź����ļĴ�����洢���� e_StorageClass
    int related_value;			// ���Ź���ֵ���������ֵ���ҹ۲죺
	                            //     ����Ǳ���������¼������ֵ��
	                            //     ����ǽṹ�壬��¼�ṹ��Ĵ�С��
	                            //     ����Ǻ�������¼���Ƿ���COFF���ű�����š�
								//     ��������飬��¼����Ԫ�ظ�����
								//     ����Ǿ�̬�ַ�������ᱻ��������ָ�롣��ʱ�����ֵΪ-1��
								
    Type typeSymbol;			// ��������
    struct Symbol *next;		// �������������ţ��ṹ�嶨�������Ա�������ţ��������������������
    struct Symbol *prev_tok;	// ָ��ǰһ�����ͬ�����š�
	                            // ���纯�������У�һ����������ͬ�����βκ�ʵ�Ρ�
} Symbol;

/************************************************************************/
/* ���ʴ洢�ṹ���壺                                                   */
/* ˵һ������������ṹ�塣�ڴʷ�������ʱ��                         */
/* ÿ����һ���ʣ�����ӵ�std::vector<TkWord> tktable���档              */
/* ����ϣ������ǽ����﷨������ʱ��ʹ��token�Ϳ��Է��ʵ���Ӧ��Ԫ�ء�
   �����ڴʷ������׶Σ��������޷����ɷ��ű�ġ�
   ���������ṹ�������Symbolָ������﷨�����Ľ׶α���䡣
   ��ӹ���Ҳ�ܼ򵥡������Ȳ鿴Symbolָ���Ƿ�Ϊ�գ����Ϊ�գ�����ӡ�
   ��ô���ǵڶ��ε�ʱ�򣬾Ͳ�������ˣ�����
   */
typedef struct TkWord
{
    int  tkcode;					// ���ʱ���
    struct TkWord *next;			// ָ���ϣ��ͻ����������
    char *spelling;					// �����ַ���
    struct Symbol *sym_struct;		// ָ�򵥴�����ʾ�Ľṹ���塣
                                    // �����ڲ���
    struct Symbol *sym_identifier;	// ָ�򵥴�����ʾ�ı�ʶ����
} TkWord;

enum e_AddrForm
{
	ADDR_OTHER,
	/* ���ֵ���������ϵı�8.4�����߲鿴Intel��Ƥ��509ҳ�ĺ�8�С�     */
	ADDR_REG = 3
};

Symbol * struct_search(int token_code);
Symbol * sym_search(int token_code);
void sym_pop(std::vector<Symbol> * pop, Symbol *new_top);
Symbol * sym_push(int token_code, Type * type, 
				int storage_class, int related_value);

Symbol * sym_direct_push(std::vector<Symbol> &sym_stack, 
				int token_code, Type * type, int related_value);
Symbol * func_sym_push(int token_code, Type * type);

Symbol * var_sym_put(Type * type, int storage_class, int token_code, int addr);
Symbol * sec_sym_put(char * sec, int related_value);

void print_all_stack(char* strPrompt);
void mk_pointer(Type *typePointer);

int type_size(Type * type, int * align);
int calc_align(int value , int align);

Type *pointed_type(Type *typePointer);
int pointed_size(Type *typePointer);

void init();
#endif
