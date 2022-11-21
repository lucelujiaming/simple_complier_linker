// symbol_table.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "get_token.h"
#include <vector>
#include "x86_generator.h"


std::vector<Operand> operand_stack;
extern std::vector<Operand>::iterator operand_stack_top;

std::vector<Symbol> global_sym_stack;  //全局符号栈
std::vector<Symbol> local_sym_stack;   //局部符号栈


extern std::vector<TkWord> tktable;
extern int token_type;

extern Type char_pointer_type,		// 字符串指针
			 int_type,				// int类型
			 default_func_type;		// 缺省函数类型

/***********************************************************
 *  功能：将符号放在符号栈中
 *  token_code：   符号编号
 *  type：符号数据类型
 *  related_value：   符号关联值
 **********************************************************/
Symbol * sym_direct_push(std::vector<Symbol> &ss, int token_code, Type * type, int related_value)
{
	Symbol s; //, *p;
	s.token_code =token_code;
	s.typeSymbol.type = type->type;
	s.typeSymbol.ref = type->ref;
	s.related_value =related_value;
	s.next = NULL;
	ss.push_back(s);
	// printf("\t ss.size = %d \n", ss.size());
	if (ss.size() >= 1)
	{
		return &ss.back();
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
	Symbol *ps, **pps;
	TkWord *ts;
	// std::vector<Symbol> * ss;

	if(local_sym_stack.size() == 0)
	{
		ps = sym_direct_push(local_sym_stack, token_code, type, related_value);
		print_all_stack("sym_push local_sym_stack");
	}
	else
	{
		ps = sym_direct_push(global_sym_stack, token_code, type, related_value);
		print_all_stack("sym_push global_sym_stack");
	}

	ps->storage_class = storage_class;

	// 因为不记录结构体成员和匿名符号
	// 所以下面的逻辑分为两个部分。
	// 第一个(token_code & SC_STRUCT)表明token_code这个符号为结构体符号。
	// 第二个(token_code < SC_ANOM)表明token_code这个符号不是匿名符号，结构成员变量，函数参数。
	if((token_code & SC_STRUCT) || (token_code < SC_ANOM))
	{
		// printf("\n tktable.size = %d \n", tktable.size());
		ts = &(TkWord)tktable[token_code & ~SC_STRUCT];
		// printf("\n\t\t(v & ~SC_STRUCT) = (0x%03X,%d) and ts.tkcode = 0x%03X\n",
		//	token_code & ~SC_STRUCT, token_code & ~SC_STRUCT, ts->tkcode);
		// 如果是结构体符号。设置sym_struct成员。
		if(token_code & SC_STRUCT)
			pps = &ts->sym_struct;
		// 否则就是小于SC_ANOM的符号。
		else
			pps = &ts->sym_identifier;

		ps->prev_tok = *pps;
		*pps = ps;
		tktable[token_code & ~SC_STRUCT] = *ts;
		printf("sym_push:: (%s, %08X, %08X) \n", tktable[token_code & ~SC_STRUCT].spelling, 
			tktable[token_code & ~SC_STRUCT].sym_struct, tktable[token_code & ~SC_STRUCT].sym_identifier);
	}
	return ps;
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
	Symbol *s, **ps;
	s = sym_direct_push(global_sym_stack, token_code, type, 0);
	print_all_stack("sym_push global_sym_stack");
	printf("tktable[%d].spelling = %s \n", token_code, tktable[token_code].spelling);
	// ps = &((TkWord *)&tktable[v])->sym_identifier;
	ps = &(tktable[token_code].sym_identifier);
	while(*ps != NULL)
		ps = &(* ps)->prev_tok;
	s->prev_tok = NULL;
	*ps = s;
	return s;
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
	else if((storage_class & SC_VALMASK) == SC_GLOBAL)
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
Symbol * sec_sym_put(char * sec, int c)
{
	TkWord * tp;
	Symbol *s;
	Type typeCurrent;
	typeCurrent.type = T_INT;
	tp = tkword_insert(sec); // , TK_CINT);
	token_type = tktable[tktable.size() - 1].tkcode ;
	s = sym_push(token_type, &typeCurrent, SC_LOCAL, c);
	return s;
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
void sym_pop(std::vector<Symbol> * pop, Symbol *b)
{
	Symbol *s, **ps;
	TkWord * ts;
	int token_code;

	// s = &(pop->back());
	s = local_sym_stack.end() - 1;
	while(s != b)
	{
		token_code = s->token_code;
		// 不记录结构体成员和匿名符号
		if((token_code & SC_STRUCT) || token_code < SC_ANOM)
		{
			ts = &(TkWord)tktable[token_code & ~SC_STRUCT];
			if(token_code & SC_STRUCT)
				ps = &ts->sym_struct;
			else
				ps = &ts->sym_identifier;

			*ps = s->prev_tok ;
			tktable[token_code & ~SC_STRUCT] = *ts;
			printf("sym_pop:: (%s, %08X, %08X) \n", tktable[token_code & ~SC_STRUCT].spelling, 
				tktable[token_code & ~SC_STRUCT].sym_struct, tktable[token_code & ~SC_STRUCT].sym_identifier);
		}
		// pop->erase(pop->begin());
		pop->pop_back();
		if(pop->size() > 0)
		{
			// s = &(pop->back());
			s = local_sym_stack.end() - 1;
		}
		else
			break;
	}
}

/***********************************************************
 *  功能：查找结构定义
 *  v：   符号编号
 **********************************************************/
Symbol * struct_search(int v)
{
	printf("\n -- struct_search -- ");
	for (int i = 42; i < tktable.size(); i++)
	{
		printf(" (%s, %08X, %08X) ", tktable[i].spelling, tktable[i].sym_struct, tktable[i].sym_identifier);
	}
	printf(" ---- \n");
	if(tktable.size() > v)
		return tktable[v].sym_struct;
	else
		return NULL;
}

/***********************************************************
 *  功能：声明与函数定义
 *  l：   存储类型，局部的还是全局的
 **********************************************************/
Symbol * sym_search(int v)
{
	if(tktable.size() > v)
		return tktable[v].sym_identifier;
	else
		return NULL;
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
	for (int i = 0; i < tktable.size(); i++)
	{
		printf("tktable[%d].spelling = %s \n", i, tktable[i].spelling);
	}
}

void init()
{
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

