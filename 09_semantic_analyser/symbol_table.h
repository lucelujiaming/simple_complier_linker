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
/* 这个值是一个32位整数。分为三个部分：
   低8位为基本类型。其中F0，F1，F2，F3是保留字。如下所示。
   之后的8位和最高的8位用来表示扩展类型。
*/
enum e_StorageClass
{
// 低8位为基本类型。
	// 如果比SC_GLOBAL小，且小于0x2C(44)。说明存储在寄存器中。取值如下：
	// REG_EAX = 0, REG_ECX,    
    // REG_EDX,     REG_EBX,
	// REG_ESP,     REG_EBP,    
	// REG_ESI,     REG_EDI,
	// REG_ANY
	SC_GLOBAL  = 0x00f0,		 // 包括：包括整型常量，字符常量，
								 //       字符串常量，全局变量，函数定义
	SC_LOCAL   = 0x00f1,		 // 栈中变量
	SC_LLOCAL  = 0x00f2,   		 // 寄存器溢出存放栈中。该值的使用流程如下：
	                             // 1. 如果一个操作数是左值，如果需要把他交换到内存栈中。
								 //    在spill_reg中，我们会调用store生成交换的机器码。
								 //    之后我们将把基本类型置为SC_LLOCAL。保留扩展类型。
								 // 2. 当我们将栈顶操作数存入次栈顶操作数中的时候，
								 //    如果发现次栈顶操作数该位置位。这就说明，
								 //    左值现在被溢出到栈中，必须再次把他加载到寄存器中。
								 //    在store_zero_to_one中，我们会调用load生成加载的机器码。
								 //    之后我们将基本类型设置为指定的寄存器。
								 //    并清空扩展类型，只置位SC_LVAL。
	SC_CMP     = 0x00f3,   		 // 使用标志寄存器
	SC_VALMASK = 0x00ff,   		 // 存储类型掩码

// 之后的8位和最高的8位用来表示扩展类型。
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

/************************************************************************/
/* 单词存储结构定义：                                                   */
/* 说一下如何理解这个结构体。在词法分析的时候，                         */
/* 每遇到一个词，被添加到std::vector<TkWord> tktable里面。              */
/* 因此上，在我们进行语法分析的时候，使用token就可以访问到相应的元素。
   但是在词法分析阶段，我们是无法生成符号表的。
   因此上这个结构体里面的Symbol指针会在语法分析的阶段被填充。
   添加过程也很简单。就是先查看Symbol指针是否为空，如果为空，就添加。
   那么岂不是第二次的时候，就不会添加了？？？
   */
typedef struct TkWord
{
    int  tkcode;					// 单词编码
    struct TkWord *next;			// 指向哈希冲突的其它单词
    char *spelling;					// 单词字符串
    struct Symbol *sym_struct;		// 指向单词所表示的结构定义。
                                    // 我们在查找
    struct Symbol *sym_identifier;	// 指向单词所表示的标识符。
} TkWord;

enum e_AddrForm
{
	ADDR_OTHER,
	/* 这个值来自于书上的表8.4。或者查看Intel白皮书509页的后8行。     */
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
