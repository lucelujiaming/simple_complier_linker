#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include <stdio.h>
#include <vector>
#pragma warning(disable : 4786)

/* 寄存器编码 */
// 这个的取值是写死的。参见书上的表8.4。
// 也就是Intel白皮书的第二卷的第二章的Table 2-2。
enum e_Register
{
    REG_EAX = 0,  // 前四个是可以使用的基础寄存器。
    REG_ECX,
    REG_EDX,
	REG_EBX,
	              // 下面的寄存器：
	REG_ESP,      //   用于函数调用的堆栈。参见gen_epilog的实现。
	REG_EBP,      //   用于函数调用的堆栈。参见gen_epilog的实现。
	REG_ESI,      //   被MOVS指令使用。用于字符串赋值。
	REG_EDI,      //   被MOVS指令使用。还用于除法。
	REG_ANY       // 任意的基础寄存器。即EAX，ECX，EDX，EBX。
};

#define REG_IRET  REG_EAX	// 存放函数返回值的寄存器

/* 存储类型 */
enum e_StorageClass
{
	// 如果比SC_GLOBAL小，且小于0x2C(44)。说明存储在寄存器中。取值如下：
	// REG_EAX = 0, REG_ECX,    
    // REG_EDX,     REG_EBX,
	// REG_ESP,     REG_EBP,    
	// REG_ESI,     REG_EDI,
	// REG_ANY
	SC_GLOBAL  = 0x00f0,		 // 包括：包括整型常量，字符常量，
								 //       字符串常量，全局变量，函数定义
	SC_LOCAL   = 0x00f1,		 // 栈中变量
	SC_LLOCAL  = 0x00f2,   		 // 寄存器溢出存放栈中
	SC_CMP     = 0x00f3,   		 // 使用标志寄存器
	SC_VALMASK = 0x00ff,   		 // 存储类型掩码

	SC_LVAL    = 0x0100,   		 // 左值
	SC_SYM     = 0x0200,   		 // 符号

	SC_ANOM	   = 0x10000000,     // 匿名符号
	SC_STRUCT  = 0x20000000,     // 结构体符号
	SC_MEMBER  = 0x40000000,     // 结构成员变量
	SC_PARAMS  = 0x80000000,     // 函数参数
};

/* 类型编码 */
enum e_TypeCode
{
	T_INT    =  0,			// 整型
	T_CHAR   =  1,			// 字符型
	T_SHORT  =  2,			// 短整型
	T_VOID   =  3,			// 空类型
	T_PTR    =  4,			// 指针
	T_FUNC   =  5,			// 函数
	T_STRUCT =  6,			// 结构体

	T_BTYPE  =  0x000f,		// 基本类型掩码
	T_ARRAY  =  0x0010,		// 数组
};

#define ALIGN_SET 0x100  // 强制对齐标志

/* 类型存储结构定义 */
typedef struct structType
{
    // e_TypeCode t;
	int    type;
    struct Symbol *ref;
} Type;

/* 符号存储结构定义 */
// 这个结构体有两种用法：
//   一种是作为Type类型对应的符号。
//   一种是作为Operand操作数对应的符号。
//   一种是作为TkWord单词存储结构对应的符号。
typedef struct Symbol
{
    int token_code;				// 符号的单词编码 e_TokenCode
    int storage_class;			// 符号关联的寄存器或存储类型 e_StorageClass
    int related_value;			// 符号关联值。这个关联值据我观察：
	                            //     如果是变量，则会记录变量的值。
	                            //     如果是结构体，记录结构体的大小。
	                            //     如果是函数，记录的是符号COFF符号表中序号。
								//     如果是数组，记录数组元素个数。
								//     如果是静态字符串。则会被当成数组指针。此时这个的值为-1。
								
    Type typeSymbol;			// 符号类型
    struct Symbol *next;		// 关联的其它符号，结构体定义关联成员变量符号，函数定义关联参数符号
    struct Symbol *prev_tok;	// 指向前一定义的同名符号。
	                            // 例如函数定义中，一个参数会有同名的形参和实参。
} Symbol;

/* 单词存储结构定义 */
/* 说一下如何理解 */
typedef struct TkWord
{
    int  tkcode;					// 单词编码
    struct TkWord *next;			// 指向哈希冲突的其它单词
    char *spelling;					// 单词字符串
    struct Symbol *sym_struct;		// 指向单词所表示的结构定义
    struct Symbol *sym_identifier;	// 指向单词所表示的标识符
} TkWord;

enum e_AddrForm
{
	ADDR_OTHER,
	/* 这个值来自于书上的表8.4。或者查看Intel白皮书509页的后8行。     */
	ADDR_REG = 3
};

Symbol * struct_search(int token_code);
Symbol * sym_search(int token_code);
void sym_pop(std::vector<Symbol> * pop, Symbol *b);
Symbol * sym_push(int token_code, Type * type, int r, int c);

Symbol * sym_direct_push(std::vector<Symbol> &sym_stack, int token_code, Type * type, int c);
Symbol * func_sym_push(int token_code, Type * type);

Symbol * var_sym_put(Type * type, int r, int token_code, int addr);
Symbol * sec_sym_put(char * sec, int c);

void print_all_stack(char* strPrompt);
void mk_pointer(Type *t);

int type_size(Type * type, int * align);
int calc_align(int n , int align);

Type *pointed_type(Type *t);
int pointed_size(Type *t);

void init();
#endif
