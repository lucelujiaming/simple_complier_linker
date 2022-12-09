// symbol_table.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "pe_generator.h"
#include <vector>
#include "x86_generator.h"
#include "pe_generator.h"

extern std::vector<std::string> vecDllName;
extern std::vector<std::string> vecLib;

extern std::vector<Operand> operand_stack;
extern std::vector<Operand>::iterator operand_stack_top;

std::vector<Symbol *> global_sym_stack;  //全局符号栈
std::vector<Symbol *> local_sym_stack;   //局部符号栈


extern int   g_output_type;
extern short g_subsystem;
extern std::vector<TkWord> tktable;
extern std::vector<char *> vecSrcFiles;

Type char_pointer_type,		// 字符串指针
	 int_type,				// int类型
	 default_func_type;		// 缺省函数类型

Symbol *sym_sec_rdata;			// 只读节符号

extern char *lib_path;

/***********************************************************
 *  功能：将符号放在符号栈中
 *  token_code：   符号编号
 *  type：符号数据类型
 *  related_value：   符号关联值
 **********************************************************/
Symbol * sym_direct_push(std::vector<Symbol *> &sym_stack, int token_code, Type * type, int related_value)
{
	Symbol * symElementPtr = (Symbol *)malloc(sizeof(Symbol)); //, *p;
    memset(symElementPtr, 0, sizeof(Symbol));
	symElementPtr->token_code =token_code;
	symElementPtr->typeSymbol.type = type->type;
	symElementPtr->typeSymbol.ref = type->ref;
	symElementPtr->related_value =related_value;
	symElementPtr->next = NULL;
	sym_stack.push_back(symElementPtr);
	// printf("\t ss.size = %d \n", ss.size());
	if (sym_stack.size() >= 1)
	{
		return sym_stack.back();
		// return &ss[0];
	}
	else
	{
		return NULL;
	}
}

/***********************************************************
 *  功能：将符号放在符号栈中，动态判断是
 *        放入全局符号栈还是局部符号栈
 *  v：   符号编号
 *  type：符号数据类型
 *  r：   符号存储类型
 *  c：   符号关联值
 **********************************************************/
Symbol * sym_push(int token_code, Type * type, int storage_class, int related_value)
{
	Symbol *new_symbol_ptr, **tktable_member_pptr;
	TkWord *ts;
	// 1. 我们在符号栈中添加一个符号。
	if(local_sym_stack.size() == 0)
	{
		new_symbol_ptr = sym_direct_push(local_sym_stack, token_code, type, related_value);
		print_all_stack("sym_push local_sym_stack");
	}
	else
	{
		new_symbol_ptr = sym_direct_push(global_sym_stack, token_code, type, related_value);
		print_all_stack("sym_push global_sym_stack");
	}

	new_symbol_ptr->storage_class = storage_class;

	// 因为不记录结构体成员和匿名符号
	// 所以下面的逻辑分为两个部分。
	// 第一个(token_code & SC_STRUCT)表明token_code这个符号为结构体符号。
	// 第二个(token_code < SC_ANOM)表明token_code这个符号不是匿名符号，结构成员变量，函数参数。
	if((token_code & SC_STRUCT) || (token_code < SC_ANOM))
	{
		// 2. 根据token找到对应的单词结构。
		//    在进入这个函数之前，我们已经做过语法分析。
		//    此时tktable中，符号sym_struct成员或者sym_identifier成员是填充过值的。
		//    但是当时填充的值是不完全。首先storage_class和related_value成员是不知道的。
		//    而token_code也不对。因此上，这些值都需要
		ts = &(TkWord)tktable[token_code & ~SC_STRUCT];
		
		// 下面几句话的结果就是：
		//    ts的sym_struct/sym_identifier成员指向new_symbol，
		//    而new_symbol的prev_tok成员指向sym_struct/sym_identifier成员之前的值。
		// 结果就是ts的sym_struct/sym_identifier成员指向一个链表。
		// 这个链表上都是同一个名字的Symbol。
		// 也就等于，在ts的sym_struct/sym_identifier的链表头上插入当前新添加的符号。
		// 想要看懂这里，需要仔细阅读sym_pop的逻辑。

		// 如果是结构体符号。设置sym_struct成员。
		if(token_code & SC_STRUCT)
			tktable_member_pptr = &ts->sym_struct;
		// 否则就是小于SC_ANOM的符号。设置sym_identifier成员。
		else
			tktable_member_pptr = &ts->sym_identifier;
		// 新添加的符号指向单词表中的标识符符号。
		new_symbol_ptr->prev_tok = *tktable_member_pptr;
		// tktable的符号成员指向新添加的符号。
		*tktable_member_pptr = new_symbol_ptr;
		
		tktable[token_code & ~SC_STRUCT] = *ts;
		printf("sym_push:: (%s, %08X, %08X) \n", tktable[token_code & ~SC_STRUCT].spelling, 
			tktable[token_code & ~SC_STRUCT].sym_struct, tktable[token_code & ~SC_STRUCT].sym_identifier);
	}
	// 返回新添加的符号
	return new_symbol_ptr;
}

/***********************************************************
 *  功能：将函数符号放入全局符号表中
 *  token_code：   符号编号
 *  type：符号数据类型
 *  函数放入符号表使用func_sym_push函数，
 *  这个函数保证函数符号都存放在全局符号栈，
 **********************************************************/
Symbol * func_sym_push(int token_code, Type * type)
{
	Symbol *new_func_symbol, **tktable_member_pptr;
	new_func_symbol = sym_direct_push(global_sym_stack, token_code, type, 0);
	print_all_stack("sym_push global_sym_stack");
	printf("tktable[%d].spelling = %s \n", token_code, tktable[token_code].spelling);
	// ps = &((TkWord *)&tktable[v])->sym_identifier;

	tktable_member_pptr = &(tktable[token_code].sym_identifier);
	// 在sym_identifier？？？上插入。
	while(*tktable_member_pptr != NULL)
		tktable_member_pptr = &(* tktable_member_pptr)->prev_tok;
	new_func_symbol->prev_tok = NULL;
	*tktable_member_pptr = new_func_symbol;
	return new_func_symbol;
}

/***********************************************************
 *  变量放入符号表通过var_sym_put函数，
 *  这个函数会根据变量是局部变量还是全局变量，放入相应的符号栈中。
 **********************************************************/
Symbol * var_sym_put(Type * type, int storage_class, int token_code, int addr)
{
	Symbol *sym = NULL;
	if((storage_class & SC_VALMASK) == SC_LOCAL)
	{
		sym = sym_push(token_code, type, storage_class, addr);
	}
	else if((storage_class & SC_VALMASK) == SC_GLOBAL	// 是全局变量，
		 && (token_code != TK_CSTR))					// 但不是字符串常量
	{
		sym = sym_search(token_code);
		if(sym)
			printf("Error dual defined");
		else
			sym = sym_push(token_code, type, storage_class | SC_SYM, 0);
	}
	return sym;
}

/***********************************************************
 *  功能：将节名称放入全局符号表
 *  sec： 节名称
 *  c：   符号关联值
 **********************************************************/
Symbol * sec_sym_put(char * sec, int related_value)
{
	TkWord * tp;
	Symbol *sym;
	Type typeCurrent;
	typeCurrent.type = T_INT;
	tp = tkword_insert(sec); // , TK_CINT);
	// token_type = tp->tkcode ;
	set_current_token_type(tp->tkcode);
	// s = sym_push(token_type, &typeCurrent, SC_LOCAL, c);
	sym = sym_push(get_current_token_type(), 
					&typeCurrent, SC_LOCAL, related_value);
	return sym;
}

//单词编码
//指向哈希冲突的其他单词
//单词字符串
//指向单词所表示的结构定义
//指向单词所表示的标识符


/***********************************************************
 *  功能： 弹出栈中符号直到栈顶符号为'b
 *  p_top：符号栈栈顶
 *  b：    符号指针
 **********************************************************/
void sym_pop(std::vector<Symbol *> * pop, Symbol *new_top)
{
	Symbol *sym, **symTktablePtr;
	TkWord * ts;
	int token_code;

	// s = &(pop->back());
	sym = *(local_sym_stack.end() - 1);
	while(sym != new_top)
	{
		token_code = sym->token_code;
		// 不记录结构体成员和匿名符号
		if((token_code & SC_STRUCT) || token_code < SC_ANOM)
		{
			ts = &(TkWord)tktable[token_code & ~SC_STRUCT];
			if(token_code & SC_STRUCT)
				symTktablePtr = &ts->sym_struct;
			else
				symTktablePtr = &ts->sym_identifier;

			*symTktablePtr = sym->prev_tok ;
			tktable[token_code & ~SC_STRUCT] = *ts;
			printf("sym_pop:: (%s, %08X, %08X) \n", tktable[token_code & ~SC_STRUCT].spelling, 
				tktable[token_code & ~SC_STRUCT].sym_struct, tktable[token_code & ~SC_STRUCT].sym_identifier);
		}
		// pop->erase(pop->begin());
		free(sym);
		pop->pop_back();
		if(pop->size() > 0)
		{
			// s = &(pop->back());
			sym = *(local_sym_stack.end() - 1);
		}
		else
			break;
	}
}

/***********************************************************
 *  功能：查找结构定义
 *  v：   符号编号
 **********************************************************/
Symbol * struct_search(int token_code)
{
#if 0
	printf("\n -- struct_search -- ");
	for (int idx = 42; idx < tktable.size(); idx++)
	{
		printf(" (%s, %08X, %08X) ", 
			tktable[idx].spelling, tktable[idx].sym_struct, 
			tktable[idx].sym_identifier);
	}
#endif
	printf(" ---- \n");
	if(tktable.size() > token_code)
		return tktable[token_code].sym_struct;
	else
		return NULL;
}

/***********************************************************
 * 功能:	查找结构定义 
 * v:		符号编号
 **********************************************************/
Symbol * sym_search(int token_code)
{
	if(tktable.size() > token_code)
	{
		TkWord tmpTkWord = tktable[token_code];
		return tmpTkWord.sym_identifier;
	}
	else
		return NULL;
}


/************************************************************************/
/* 功能:		返回类型长度                                            */
/* typeCal:		数据类型指针                                            */
/* align:		对齐值                                                  */
/* 返回值:	                                                            */
/*     指针类型返回-1。其他类型会计算出实际的长度。                     */
/************************************************************************/
int type_size(Type * typeCal, int * align)
{
	Symbol *sym;
	int bt;
	int PTR_SIZE = 4;
	bt = typeCal->type & T_BTYPE;
	switch(bt)
	{
	case T_STRUCT:
		sym = typeCal->ref;
		*align = sym->storage_class;
		return sym->related_value ;
	case T_PTR:
		// 如果是指针数组。这里分两种情况。
		// 1. 一个是数组变量，类似于int AA[717]。
		//    这种情况下，长度等于typeSymbol的大小乘以related_value，
		//    例如int AA[717]的长度就是4 * 717 = 2868。
		// 2. 一个是数组常量，类似于"Hello world!"这种字符串。
        //    这种情况下，related_value等于-1。直接返回负数。由上层处理。
		//    因为这种情况下，长度可以通过token直接计算。
		if(typeCal->type & T_ARRAY)
		{
			sym = typeCal->ref;
			return type_size(&sym->typeSymbol, align) * sym->related_value;
		}
		// 否则指针的长度为32位，也就是4。
		else
		{
			*align = PTR_SIZE;
			return PTR_SIZE;
		}
	case T_INT:
		*align = 4;
		return 4 ;
	case T_SHORT:
		*align = 2;
		return 2 ;
	default:			// char, void, function
		*align = 1;
		return 1 ;
	}
}

/***********************************************************
 * 功能:	返回t所指向的数据类型
 * t:		指针类型
 **********************************************************/
Type *pointed_type(Type *typePointer)
{
    return &typePointer->ref->typeSymbol;
}

/***********************************************************
 * 功能:	返回t所指向的数据类型尺寸
 * t:		指针类型
 **********************************************************/
int pointed_size(Type *typePointer)
{
    int align;
    return type_size(pointed_type(typePointer), &align);
}

/*********************************************************** 
 * 功能:	计算字节对齐位置
 * n:		未对齐前值
 * align:   对齐粒度
 **********************************************************/
int calc_align(int value , int align)
{
	return (value + align -1) & (~(align -1));
}

/*********************************************************** 
 * 功能:	生成指针类型
 * t:		原数据类型
 **********************************************************/
void mk_pointer(Type *typePointer)
{
	Symbol *sym;
    sym = sym_push(SC_ANOM, typePointer, 0, -1);
    typePointer->type = T_PTR ;
    typePointer->ref = sym;
}

/***********************************************************
 * 功能:	词法分析初始化
 **********************************************************/
void init_lex()
{
	TkWord* tp;
	static TkWord keywords[] = {
	/* 运算符及分隔符 1-25 */
	{TK_PLUS,		NULL,	  "+",	NULL,	NULL},
	{TK_MINUS,		NULL,	  "-",	NULL,	NULL},
	{TK_STAR,		NULL,	  "*",	NULL,	NULL},
	{TK_DIVIDE,		NULL,	  "/",	NULL,	NULL},
	{TK_MOD,		NULL,	  "%",	NULL,	NULL},

	{TK_EQ,			NULL,	  "==",	NULL,	NULL},
	{TK_NEQ,		NULL,	  "!=",	NULL,	NULL},
	{TK_LT,			NULL,	  "<",	NULL,	NULL},
	{TK_LEQ,		NULL,	  "<=",	NULL,	NULL},
	{TK_GT,			NULL,	  ">",	NULL,	NULL},

	{TK_GEQ,		NULL,	  ">=",	NULL,	NULL},
	{TK_ASSIGN,		NULL,	  "=",	NULL,	NULL},
	{TK_POINTSTO,	NULL,	  "->",	NULL,	NULL},
	{TK_DOT,		NULL,	  ".",	NULL,	NULL},
	{TK_AND,		NULL,	  "&",	NULL,	NULL},

	{TK_OPENPA,		NULL,	  "(",	NULL,	NULL},
	{TK_CLOSEPA,	NULL,	  ")",	NULL,	NULL},
	{TK_OPENBR,		NULL,	  "[",	NULL,	NULL},
	{TK_CLOSEBR,	NULL,	  "]",	NULL,	NULL},
	{TK_BEGIN,		NULL,	  "{",	NULL,	NULL},

	{TK_END,		NULL,	  "}",	NULL,	NULL},
	{TK_SEMICOLON,	NULL,	  ";",	NULL,	NULL},
	{TK_COMMA,		NULL,	  ",",	NULL,	NULL},
	{TK_ELLIPSIS,	NULL,	"...",	NULL,	NULL},
	{TK_EOF,		NULL,	 "End_Of_File",	NULL,	NULL},

    /* 常量 26-28 */
	{TK_CINT,		NULL,	 	"整型常量",	NULL,	NULL},
	{TK_CCHAR,		NULL,		"字符常量",	NULL,	NULL},
	{TK_CSTR,		NULL,		"字符串常量",	NULL,	NULL},

	/* 关键字 29-41 */
	{KW_CHAR,		NULL,		"char",	NULL,	NULL},
	{KW_SHORT,		NULL,		"short",	NULL,	NULL},
	{KW_INT,		NULL,		"int",	NULL,	NULL},
	{KW_VOID,		NULL,		"void",	NULL,	NULL},
	{KW_STRUCT,		NULL,		"struct",	NULL,	NULL},

	{KW_IF,			NULL,		"if"	,	NULL,	NULL},
	{KW_ELSE,		NULL,		"else",	NULL,	NULL},
	{KW_FOR,		NULL,		"for",	NULL,	NULL},
	// 这里少了一行。。
	{KW_WHILE,		NULL,		"while",	NULL,	NULL},
	{KW_CONTINUE,	NULL,		"continue",	NULL,	NULL},

	{KW_BREAK,		NULL,		"break",	NULL,	NULL},
	{KW_RETURN,		NULL,		"return",	NULL,	NULL},
	{KW_SIZEOF,		NULL,		"sizeof",	NULL,	NULL},

	/* 关键字 42-44 */
	{KW_ALIGN,		NULL,		"__align",	NULL,	NULL},
	{KW_CDECL,		NULL,		"__cdecl",	NULL,	NULL},
	{KW_STDCALL,	NULL,		"__stdcall",	NULL,	NULL},
	{0,				NULL,	NULL,	NULL,		NULL}
	};

    for (tp = &keywords[0]; tp->spelling != NULL; tp++)
		tktable.push_back(*tp);
	// printf("tktable.size = %d \n", tktable.size());

}

void print_all_TkWord()
{
	for (int idx = 0; idx < tktable.size(); idx++)
	{
		printf("tktable[%d].spelling = %s \n", idx, tktable[idx].spelling);
	}
}

void init()
{
	g_output_type = OUTPUT_EXE;
	g_subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;

	vecSrcFiles.reserve(1);
	vecLib.reserve(4);
	vecDllName.reserve(4);
	operand_stack.reserve(OPSTACK_SIZE);
	// The begin is reserved
	operand_stack_top = operand_stack.begin();
	
	global_sym_stack.reserve(1024);
	local_sym_stack.reserve(1024);
	init_lex();

//	sym_sec_rdata = sec_sym_put(".rdata", 0);
	int_type.type = T_INT;
	char_pointer_type.type = T_CHAR;
	mk_pointer(&char_pointer_type);
	default_func_type.type = T_FUNC;
//	default_func_type.ref =

	init_coff();
	lib_path = get_lib_path();
}

void cleanup()
{
	int idx;
	sym_pop(&global_sym_stack, NULL);
	for(idx = 0; idx < global_sym_stack.size(); ++idx)
	{
		free(global_sym_stack[idx]);
	}
	global_sym_stack.clear();
	for(idx = 0; idx < local_sym_stack.size(); ++idx)
	{
		free(local_sym_stack[idx]);
	}
	local_sym_stack.clear();
	for(idx = 0; idx < vecSection.size(); ++idx)
	{
		free(vecSection[idx]);
	}
	vecSection.clear();
	
//	for(idx = 0; idx < global_sym_stack.size(); ++idx)
//	{
//
//	}
	tktable.clear();
	vecDllName.clear();
	vecSrcFiles.clear();
	vecLib.clear();
}

//	int main(int argc, char* argv[])
//	{
//		init();
//		token_init(argv[1]);
//		get_token();
//		translation_unit();
//		printf("Hello World!\n");
//		token_cleanup();
//		return 0;
//	}

