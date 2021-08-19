// translation_unit.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include<stdio.h>
#include<stdlib.h>
#include "get_token.h"
#include "symbol_table.h"
#include "x86_generator.h"

extern int loc;
e_SynTaxState syntax_state = SNTX_NUL;
int syntax_level;

extern Section *sec_text,	// 代码节
		*sec_data,			// 数据节
		*sec_bss,			// 未初始化数据节
		*sec_idata,			// 导入表节
		*sec_rdata,			// 只读数据节
		*sec_rel,			// 重定位信息节
		*sec_symtab,		// 符号表节	
		*sec_dynsymtab;		// 链接库符号节

extern std::vector<Symbol> global_sym_stack;  //全局符号栈
extern std::vector<Symbol> local_sym_stack;   //局部符号栈

extern Type char_pointer_type,		// 字符串指针
	 int_type,				// int类型
	 default_func_type;		// 缺省函数类型

extern std::vector<Operand> operand_stack;
extern std::vector<Operand>::iterator operand_stack_top;
extern std::vector<Operand>::iterator operand_stack_second;

int type_specifier(Type * type);
void declarator(Type * type, int * v, int * force_align);
void direct_declarator(Type * type, int * v, int func_call);
void direct_declarator_postfix(Type * type, int func_call);
void parameter_type_list(Type * type, int func_call); // (int func_call);

void compound_statement(int * bsym, int * csym);
void if_statement();
void break_statement();
void return_statement();
void continue_statement();
void for_statement();
void expression_statement();

void expression();
void assignment_expression();
void equality_expression();
void relational_expression();

void external_declaration(e_StorageClass iSaveType);

int calc_align(int n , int align);

Section * allocate_storage(Type * type, int r, int has_init, int v, int *addr);
void init_variable(Type * type, Section * sec, int c, int v);

void print_error(char * strErrInfo)
{
	printf("<ERROR> %s", strErrInfo);
	exit(1);
}

void print_tab(int nTimes)
{
	int n = 0;
	for (; n < nTimes; n++)
	{
		printf("\t");
	}
}

void syntax_indent()
{
	switch(syntax_state) {
	case SNTX_NUL:
		color_token(LEX_NORMAL, get_current_token_type(), get_current_token());
		break;
	case SNTX_SP:
		printf(" ");
		color_token(LEX_NORMAL, get_current_token_type(), get_current_token());
		break;
	case SNTX_LF_HT:
		{
			if (get_current_token_type() == TK_END)
			{
				syntax_level--;
			}
			printf("\n");
			print_tab(syntax_level);
		}
		color_token(LEX_NORMAL, get_current_token_type(), get_current_token());
		break;
	case SNTX_DELAY:
		break;
	default:
		break;
	}
	syntax_state = SNTX_NUL;
}

/* <function_calling_convention> ::= <KW_CDECL> | <KW_STDCALL>            */
/* Such as __stdcall__  | __cdecl                                         */ 
/*  "__cdecl",       KW_CDECL,		// __cdecl关键字 standard c call      */
/*	"__stdcall",     KW_STDCALL,    // __stdcall关键字 pascal c call      */
void function_calling_convention(int *fc)
{
	*fc = KW_CDECL;  // Set default value with __cdecl
	if(get_current_token_type() == KW_CDECL || get_current_token_type() == KW_STDCALL)
	{
		*fc = get_current_token_type();
		syntax_state = SNTX_SP;
		get_token();
	}
}

/* <struct_member_alignment> ::= <KW_ALIGN><TK_OPENPA><TK_INT><TK_CLOSEPA> */
/* Such as __align(4)                                                        */ 
/*   "__align",       KW_ALIGN,		// __align关键字	                   */
void struct_member_alignment(int * force_align) // (int *fc)
{
	int align = 1;  // Default value of __align is one.
	// Do not need indent or not
	if(get_current_token_type() == KW_ALIGN)
	{
		get_token();
		skip_token(TK_OPENPA);
		if(get_current_token_type() == TK_CINT)
		{
			get_token();
			// ex. : Get 4 for __align(4)
			align = atoi(get_current_token());  // as a TK_CINT
		}
		else
		{
			print_error("Need intergat\n");
		}
		// *fc = token;
		skip_token(TK_CLOSEPA); 
		// Calculate and set force_align.
		if(align !=1 && align !=2 && align !=4)
			align = 1;  // Only support 1, 2, 4.
		align |= ALIGN_SET;
		*force_align = align;
	}
	else
		*force_align = 1; // Default is one.
}

/*********************************************************** 
 * 功能:	生成指针类型
 * t:		原数据类型
 **********************************************************/
void mk_pointer(Type *t)
{
	Symbol *s;
    s = sym_push(SC_ANOM, t, 0, -1);
    t->t = T_PTR ;
    t->ref = s;
}

/* <declarator> ::= {<TK_STAR>}[<function_calling_convention>]               */
/*                    [<struct_member_alignment>]<direct_declarator>         */
/* Such as * __cdecl __align(4) function(int a, int b)                       */ 
void declarator(Type * type, int * v, int * force_align)
{
	int fc ;
	while(get_current_token_type() == TK_STAR)		// * 星号
	{
		mk_pointer(type);
		get_token();
	}
	function_calling_convention(&fc);
	if(force_align)
	{
		struct_member_alignment(force_align);  // &fc
	}
	direct_declarator(type, v, fc);
}

/* <direct_declarator> ::= <IDENTIFIER><direct_declarator_postfix>              */
void direct_declarator(Type * type, int * v, int func_call)   // Not support argv[] usage
{
	if(get_current_token_type() >= TK_IDENT)  // 函数名或者是变量名，不可以是保留关键字。 
	{
		*v = get_current_token_type();
		get_token();
	}
	else
	{
		print_error("direct_declarator can not be TK_IDENT\n");
	}
	direct_declarator_postfix(type, func_call);
}

/* <direct_declarator_postfix> :: = {<TK_OPENBR><TK_CINT><TK_CLOSEBR>       (1)  */
/*                         | <TK_OPENBR><TK_CLOSEBR>                        (2)  */
/*                         | <TK_OPENPA><parameter_type_list><TK_CLOSEPA>， (3)  */
/*                         | <TK_OPENPA><TK_CLOSEPA>}                       (4)  */
/* Such as var, var[5], var(), var(int a, int b), var[]                          */ 
void direct_declarator_postfix(Type * type, int func_call)
{
	int n;
	Symbol * s;
	if(get_current_token_type() == TK_OPENPA)         // <TK_OPENPA><parameter_type_list><TK_CLOSEPA> | <TK_OPENPA><TK_CLOSEPA>
	{
		parameter_type_list(type, func_call);
	}
	else if(get_current_token_type() == TK_OPENBR)   // <TK_OPENBR><TK_CINT><TK_CLOSEBR> | <TK_OPENBR><TK_CLOSEBR>
	{
		get_token();
		n = -1;
		if(get_current_token_type() == TK_CINT)
		{
			get_token();
			// n = tkvalue;  // ??
			n = atoi(get_current_token());  // as a TK_CINT
		}
		else
		{
			print_error("Need intergat\n");
		}
		skip_token(TK_CLOSEBR);
		direct_declarator_postfix(type, func_call);    // Nesting calling
		s = sym_push(SC_ANOM, type, 0, n);
		type->t = T_ARRAY | T_PTR;
		type->ref = s;
	}
}

/*  功能：解析形参类型表                                                  */
/*  func_call：函数调用约定                                               */
/*                                                                        */
/*  <parameter_type_list> ::= <parameter_list>                            */
/*             | <parameter_list><TK_COMMA><TK_ELLIPSIS>                  */
/*                 TK_ELLIPSIS is "...",   ... 省略号                     */
/*  <parameter_list> ::= <parameter_declaration>                          */
/*                      {<TK_COMMA><parameter_declaration>}               */
/*      Such as func(int a), func(int a, int b)                           */
/*  <parameter_declaration> ::= <type_specifier>{<declarator>}            */
/*      Such as  int a                                                    */
/*  等价转换后文法：                                                      */
/*  <parameter_type_list>::=<type_specifier>{<declarator>}                */
/*   {<TK_COMMA><type_specifier>{<declarator>} } <TK_COMMA><TK_ELLIPSIS>  */
void parameter_type_list(Type * type, int func_call) // (int func_call)
{
	int n;
	Symbol **plast, *s, *first;
	Type typeCurrent ;
	get_token();
	first = NULL;
	plast = &first;
	
	while(get_current_token_type() != TK_CLOSEPA)   // get_token until meet ) 右圆括号
	{
		if(!type_specifier(&typeCurrent))
		{
			print_error("Invalid type_specifier\n");
		}
		declarator(&typeCurrent, &n, NULL);			// Translate one parameter declaration
		s = sym_push(n|SC_PARAMS, &typeCurrent, 0, 0);
		*plast = s;
		plast  = &s->next;
		
		if(get_current_token_type() == TK_CLOSEPA) // We encounter the ) after one parameter
		{
			break;
		}
		skip_token(TK_COMMA);
		if(get_current_token_type() == TK_ELLIPSIS) // We encounter ... 
		{
			// func_call = KW_CDECL ; // record the function_calling_convention
			get_token();
			break;
		}
	}
	syntax_state = SNTX_DELAY;
	skip_token(TK_COMMA);
	if(get_current_token_type() == TK_BEGIN)            // func(int a) {
		syntax_state = SNTX_LF_HT;
	else                             // func(int a);
		syntax_state = SNTX_NUL;
	syntax_indent();
	
	s = sym_push(SC_ANOM, type, func_call, 0);
	s->next = first;
	type->t = T_FUNC;
	type->ref = s;
}

// ------------------ func_body 
/************************************************
 *  <func_body> ::= <compound_statement>
 *  因为只要见到<{ 左大括号>就是复合语句，所以<复合语句>的概念比<函数体>概念要大，
 *  <函数体>属于一类比较特殊的<复合语句>。
 ***********************/
void func_body(Symbol * sym)
{
	sym_direct_push(local_sym_stack, SC_ANOM, &int_type, 0);
	compound_statement(NULL, NULL);
	sym_pop(&local_sym_stack, NULL);
}

// 初值符的代码如下所示。
/***********************************************************
* <initializer> ::= <assignment_expression>
*********************************************/
void initializer(Type * type, int c, Section * sec)
{
	if(type->t & T_ARRAY)
	{
		memcpy(sec->data + c, get_current_token(), strlen(get_current_token()));
		get_token();
	}
	else
	{
		assignment_expression();
		init_variable(type, sec, c, 0);
	}
}

/************************************************************************/
/* 功能：变量初始化                                                     */
/* type：变量类型                                                       */
/* sec：变量所在节                                                      */
/* c：变量相关值                                                        */
/* v：变量符号编号                                                      */
/************************************************************************/
void init_variable(Type * type, Section * sec, int c, int v)
{

}

// ------------------ 语句：statement
/***********************************************************
 * 功能:	解析语句
 * bsym:	break跳转位置。暂时没有用。
 * csym:	continue跳转位置。暂时没有用。
 *
 * <statement>::=<compound_statement>    
 *             | <if_statement>          
 *             | <return_statement>      
 *             | <break_statement>       
 *             | <continue_statement>    
 *             | <for_statement>         
 *             | <expression_statement>  
 **********************************************************/
void statement()
{
	switch(get_current_token_type())
	{
	case TK_BEGIN:
		compound_statement(NULL, NULL);			// 复合语句
		break;
	case KW_IF:
		if_statement();
		break;
	case KW_RETURN:
		return_statement();
		break;
	case KW_BREAK:
		break_statement();
		break;
	case KW_CONTINUE:
		continue_statement();
		break;
	case KW_FOR:
		for_statement();
		break;
	default:
		expression_statement();
		break;
	}
}

/***********************************************************
 * <compound_statement> ::= <TK_BEGIN>{<declaration>} {<statement>} <TK_END>
 **********************************************************/
int is_type_specifier(int token_type)
{
	switch(token_type)
	{
	case KW_CHAR:
	case KW_SHORT:
	case KW_INT:
	case KW_VOID:
	case KW_STRUCT:
		return 1;
	default:
		break;
	}
	return 0;
}

void compound_statement(int * bsym, int * csym)
{
	Symbol *s;
	s = &(local_sym_stack[0]);
	local_sym_stack.erase(&local_sym_stack[0]);
	
	syntax_state = SNTX_LF_HT;
	syntax_level++;

	get_token();
	while(is_type_specifier(get_current_token_type()))
	{
		external_declaration(SC_LOCAL);
	}

	while(get_current_token_type() != TK_END)
	{
		statement();
	}
	syntax_state = SNTX_LF_HT;
	sym_pop(&local_sym_stack, s);
	get_token();
}

/***********************************************************
 * <expression_statement> ::= <TK_SEMICOLON>|<expression><TK_SEMICOLON>
 *   TK_SEMICOLON 是 ; 分号
 **********************************************************/
void expression_statement()
{
	if(get_current_token_type() != TK_SEMICOLON)
	{
		expression();
	}
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);

}

/***********************************************************
 * <if_statement> ::= <KW_IF><TK_OPENPA><expression>
 *          <TK_CLOSEPA><statement>[<KW_ELSE><statement>]
 **********************************************************/
void if_statement()
{
	syntax_state = SNTX_SP;

	get_token();
	skip_token(TK_OPENPA);
	expression();

	syntax_state = SNTX_LF_HT;
	skip_token(TK_CLOSEPA);
	statement();

	if(get_current_token_type() == KW_ELSE)
	{
		syntax_state = SNTX_LF_HT;
		get_token();
		statement();
	}
}	

/***********************************************************
 *  <for_statement> ::= <KW_FOR><TK_OPENPA><expression_statement>
 *        <expression_statement><expression><TK_CLOSEPA><statement>
 **********************************************************/
void for_statement()
{
	get_token();
	skip_token(TK_OPENPA);

	// Chapter I
	if(get_current_token_type() != TK_SEMICOLON)   // Such as for(n = 0; n < 100; n++)
	{
		expression();
		skip_token(TK_SEMICOLON);
	}
	else 						// Such as for(; n < 100; n++)
		skip_token(TK_SEMICOLON);

	// Chapter II
	if(get_current_token_type() != TK_SEMICOLON)   // Such as for(n = 0; n < 100; n++)
	{
		expression();
		skip_token(TK_SEMICOLON);
	}
	else 						// Such as for(n = 0; ; n++)
		skip_token(TK_SEMICOLON);

	// Chapter III
	if(get_current_token_type() != TK_CLOSEPA)   // Such as for(n = 0; n < 100; n++)
	{
		expression();
		syntax_state = SNTX_LF_HT;
		skip_token(TK_CLOSEPA);
	}
	else 						// Such as for(n = 0; n < 100;)
	{
		syntax_state = SNTX_LF_HT;
		skip_token(TK_CLOSEPA);
	}
	// Deal with the body of for_statement 
	statement(); 
}

/***********************************************************
 *  <while_statement> ::= <KW_WHILE><TK_OPENPA><expression><TK_CLOSEPA><statement>
 **********************************************************/
void while_statement()
{
	get_token();
	skip_token(TK_OPENPA);
	// Chapter I
	expression();
	skip_token(TK_CLOSEPA);
	syntax_state = SNTX_LF_HT;
	// Deal with the body of while_statement 
	statement(); 
}

/***********************************************************
 *  <continue_statement> ::= <KW_CONTINUE><TK_SEMICOLON>
 **********************************************************/
void continue_statement()
{
	get_token();
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
}

/***********************************************************
 *  <break_statement> ::= <KW_CONTINUE><TK_SEMICOLON>
 **********************************************************/
void break_statement()
{
	get_token();
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
}

/***********************************************************
 *  <return_statement> ::= <KW_RETURN><TK_SEMICOLON>
 *                       | <KW_RETURN><expression><TK_SEMICOLON>
 **********************************************************/
void return_statement()
{
	syntax_state = SNTX_DELAY;
	get_token();
	if(get_current_token_type() == TK_SEMICOLON)   // 
	{
		syntax_state = SNTX_NUL;
	}
	else 						// 
	{
		syntax_state = SNTX_SP;
	}
	syntax_indent();
	if(get_current_token_type() != TK_SEMICOLON)   // 
	{
		expression();
	}
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
}


/***********************************************************
 *  <expression> ::= <assignment_expression>
 *        {<TK_COMMA><assignment_expression>}
 **********************************************************/
void expression()
{
	while (1)
	{
		assignment_expression();
		// Exit except this kind of code : int a = 1, b = 2; 
		if (get_current_token_type() != TK_COMMA)   
		{
			break;
		}
		get_token();
	}
}

/***********************************************************
 *  <assignment_expression> ::= <equality_expression>
 *           |<unary_expression><TK_ASSIGN><assignment_expression>：
 *  非等价变换后文法：
 *  <assignment_expression> ::= <equality_expression>
 *                  {<TK ASSIGN><assignment expression>} 
 **********************************************************/
void assignment_expression()
{
	equality_expression();
	if (get_current_token_type() == TK_ASSIGN)
	{
		get_token();
		assignment_expression();
	}
}

/***********************************************************
 *  <equality expression> ::= <relational_expression>
 *                    {<TK_EQ><relational_expression>
 *                    |<TK_NEQ><relational_expression>}
 **********************************************************/
void equality_expression()
{
	relational_expression();
//	if (get_current_token_type() == TK_EQ 
//		|| get_current_token_type() == TK_NEQ)
	while (get_current_token_type() == TK_EQ 
		|| get_current_token_type() == TK_NEQ)
	{
		get_token();
		relational_expression();
	}
}

void additive_expression();
void multiplicative_expression();
void unary_expression();
/***********************************************************
 *  <relational_expression> ::= <additive_expression>{
 *       <TK_LT><additive_expression>    // < 小于号      TK_LT, "<", 
 *     | <TK_GT><additive_expression>    // <= 小于等于号 TK_LEQ, "<=",
 *     | <TK_LEQ><additive_expression>   // > 大于号      TK_GT, ">", 
 *     | <TK_GEQ><additive_expression>}  // >= 大于等于号 TK_GEQ, ">=",
 **********************************************************/
void relational_expression()
{
	additive_expression();
	while(get_current_token_type() == TK_LT || get_current_token_type() == TK_LEQ 
		|| get_current_token_type() == TK_GT || get_current_token_type() == TK_GEQ)
	{
		get_token();
		additive_expression();
	}
}
 
/***********************************************************
 * <additive_expression> ::= <multiplicative_expression>
 *                {<TK_PLUS><multiplicative_expression>
 *                 <TK_MINUS><multiplicative_expression>)
 **********************************************************/
void additive_expression()
{
	multiplicative_expression();
	while(get_current_token_type() == TK_PLUS || get_current_token_type() == TK_MINUS)
	{
		get_token();
		multiplicative_expression();
	}
}

/***********************************************************
* <multiplicative_expression> ::= <unary_expression>
*                   {<TK_STAR><unary_expression>
*                   |<TK_DIVIDE><unary_expression>
*                   |<TK_MOD><unary_expression>}
**********************************************************/
void multiplicative_expression()
{
	unary_expression();
	while(get_current_token_type() == TK_STAR || get_current_token_type() == TK_DIVIDE || get_current_token_type() == TK_MOD)
	{
		get_token();
		unary_expression();
	}
}

void primary_expression();
void postfix_expression();
void sizeof_expression();
void argument_expression_list();
/***********************************************************
* <unary_expression> ::= <postfix_expression>
*                 | <TK_AND><unary_expression>
*                 | <TK_STAR><unary_expression>
*                 | <TK_PLUS><unary_expression>
*                 | <TK_MINUS><unary_expression>
*                 | <sizeof_expression>
**********************************************************/
void unary_expression()
{
	switch(get_current_token_type())
	{
	case TK_AND:
	case TK_STAR:
	case TK_PLUS:
	case TK_MINUS:
		get_token();
		unary_expression();
		break;
	case KW_SIZEOF:
		sizeof_expression();
		break;
	default:
		postfix_expression();
		break;
	}
}

int type_size(Type * type, int * align);
/***********************************************************
 *  <sizeof_expression> ::= 
 *     <KW_SIZEOF><TK_OPENPA><type_specifier><TK_CLOSEPA>
 **********************************************************/
void sizeof_expression()
{
	int align, size;
	Type typeCurrent ;
	
	get_token();
	skip_token(TK_OPENPA);
	type_specifier(&typeCurrent);
	skip_token(TK_CLOSEPA);
	
	size = type_size(&typeCurrent, &align);
	if(size < 0)
		print_error("sizeof failed.");
}

int type_size(Type * type, int * align)
{
	Symbol *s;
	int bt;
	int PTR_SIZE = 4;
	bt = type->t & T_BTYPE;
	switch(bt)
	{
	case T_STRUCT:
		s = type->ref;
		*align = s->r;
		return s->c ;
	case T_PTR:
		if(type->t & T_ARRAY)
		{
			s = type->ref;
			return type_size(&s->type, align) * s->c;
		}
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
	default:			// char, void, fucntion
		*align = 1;
		return 1 ;
	}
}

/***********************************************************
 *  <postfix_expression> ::= <primary_expression>
 *    { <TK_OPENBR><expression><TK_CLOSEBR>
 *    | <TK_OPENPA><TK_CLOSEPA>
 *    | <TK_OPENPA><argument_expression_list><TK_CLOSEPA>
 *    | <TK_DOT><IDENTIFIER>
 *    | <TK POINTS TO><IDENTIFIER>}
 **********************************************************/
void postfix_expression()
{
	primary_expression();
	while (1)
	{
		if (get_current_token_type() == TK_DOT               // | <TK_DOT><IDENTIFIER>
			|| get_current_token_type() == TK_POINTSTO)      // | <TK_POINTSTO><IDENTIFIER>
		{
			get_token();
			// token |= SC_MEMBER;
			get_token();
		}
		else if (get_current_token_type() == TK_OPENBR)      // <TK_OPENBR><expression><TK_CLOSEBR>
		{
			get_token();
			expression();
			skip_token(TK_CLOSEBR);
		}
		else if (get_current_token_type() == TK_OPENPA)      // <TK_OPENPA><argument_expression_list><TK_CLOSEPA>
		{
			argument_expression_list();
		}
		else
			break;
	}
}

/***********************************************************
 *  <primary_expression> ::= <IDENTIFIER>
 *     | <TK_CINT>
 *     | <TK_CCHAR>
 *     | <TK_CSTR>
 *     | <TK_OPENPA><expression><TK_CLOSEPA>
 **********************************************************/
void primary_expression()
{
	int t , r, addr;
	Type type;
	Symbol *s ;
	Section * sec = NULL;
	
	switch(get_current_token_type()) {
	case TK_CINT:
	case TK_CCHAR:
	case TK_CSTR:
		// get_token();
		t = T_CHAR;
		type.t = t;
		mk_pointer(&type);
		type.t |= T_ARRAY;
		var_sym_put(&type, SC_GLOBAL, 0, addr);
		initializer(&type, addr, sec);
		break;
	case TK_OPENPA:
		get_token();
		expression();
		skip_token(TK_CLOSEPA);
		break;
	default:
		t = get_current_token_type();
		get_token();
		if (t < TK_IDENT) // The problem of String at 07/11, We need the parse_string function
		{
			print_error("Need variable or constant\n");
		}
		s = sym_search(t);
		if(!s)
		{
			if (get_current_token_type() != TK_OPENPA)
			{
				char testStr[128];
				sprintf(testStr, "%s does not declare\n", get_current_token());
				print_error(testStr);
				s = func_sym_push(t, &default_func_type);
				s->r = SC_GLOBAL | SC_SYM;
			}
		}
		break;
	}
}

/***********************************************************
 *  <argument_expression_list> ::= <assignment_expression>
 *                      {<TK_COMMA><assignment_expression>}
 **********************************************************/
void argument_expression_list()
{
	get_token();
	if(get_current_token_type() != TK_OPENPA) 
	{
		for(;;)
		{
			assignment_expression();
			if(get_current_token_type() == TK_CLOSEPA)
				break;
			skip_token(TK_COMMA);
		}
	}
	skip_token(TK_CLOSEPA);
}

/************************************************************************
 *	<struct_declaration> ::= 
 *	<type_specifier><struct_declarator_list><TK_SEMICOLON>*
 *	<struct_declarator_list> ::= <declarator>{<TK_COMMA><declarator>}
 *	等价转换后文法：：
 *	<struct_declaration> ::= 
 *	<type_specifier><declarator>{<TK_COMMA><declarator>}
 *	<TK_SEMICOLON>
 ***********************************************************************/
void struct_declaration(int * maxalign, int * offset, Symbol *** ps)
{
	int v, size, align;
	Symbol * ss;
	Type typeOne, bType ;
	int force_align;
	
	type_specifier(&bType);
	while (1)
	{
		v = 0 ;
		typeOne = bType;
		declarator(&typeOne, &v, &force_align);
		// Adding Symbol operation
		size = type_size(&typeOne, &align);
		if (force_align & ALIGN_SET)
		{
			align = force_align & ~ALIGN_SET;
		}
		*offset = calc_align(*offset, align);
		if (align > *maxalign)
		{
			*maxalign = align;
		}
		ss = sym_push(v | SC_MEMBER, &typeOne, 0, *offset);
		* offset = size;
		** ps = ss;
		*ps = &ss->next;
		// end of Adding Symbol operation

		if (get_current_token_type() == TK_SEMICOLON)
		{
			break;
		}
		skip_token(TK_COMMA);
	}
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
}

int calc_align(int n , int align)
{
	return (n + align -1) & (~(align -1));
}

/************************************************************************/
/*  <struct_declaration_list> ::= <struct_declaration>                  */
/*                               {<struct_declaration>}                 */
/************************************************************************/
void struct_declaration_list(Type * type)
{
	int maxalign, offset;
	// syntax_state = SNTX_LF_HT;
	// syntax_level++;
	Symbol *s, **ps;
	get_token();
	// Adding Symbol operation
	s = type->ref;
	if (s->c != -1)
	{
		print_error("Has defined");
	}
	maxalign = 1;
	ps = &s->next;
	offset = 0 ;
	// end of Adding Symbol operation
	while (get_current_token_type() != TK_END)  // } 右大括号
	{
		struct_declaration(&maxalign, &offset, &ps);
	}
	skip_token(TK_END);
	// syntax_state = SNTX_LF_HT;
	s->c = calc_align(offset, maxalign);
	s->r = maxalign;
}

/************************************************************************/
/*  <struct_specifier> ::= <KW_STRUCT><IDENTIFIER><TK_BEGIN>            */
/*                         <struct_declaration_list><TK_END>            */
/*                       | <KW_STRUCT><IDENTIFIER>                      */
/************************************************************************/
void struct_specifier(Type * type)
{
	int v ;
	Symbol * s;
	Type typeOne;
	
	get_token();		// Get struct name <IDENTIFIER>
	v = get_current_token_type();
	syntax_state = SNTX_DELAY;
	// DELAY disable print in the get_token
	get_token(); // Get token after the struct name

	if(get_current_token_type() == TK_BEGIN) // Indent at {
		syntax_state = SNTX_LF_HT;
	else if(get_current_token_type() == TK_CLOSEPA) // sizeof(struct_name);
		syntax_state = SNTX_NUL;
	else
		syntax_state = SNTX_SP;

	syntax_indent();
	if (v < TK_IDENT)  // Key word is illegal
	{
		print_error("Need struct name\n");
	}
	// Adding Symbol operation
	s = struct_search(v);
	if (!s)
	{
		typeOne.t = T_STRUCT;
		s = sym_push(v | SC_STRUCT, &typeOne, 0, -1);
		s->r = 0;
	}
	type->t = T_STRUCT;
	type->ref = s;
	// end of Adding Symbol operation
	if (get_current_token_type() == TK_BEGIN)
	{
		struct_declaration_list(type);
	}
}

/************************************************************************/
/* <type_specifier> ::= <KW_CHAR> | <KW_SHORT>                          */
/*                    | <KW_VOID> | <KW_INT> | <struct_specifier>       */
/************************************************************************/
int type_specifier(Type * type)
{
	e_TypeCode t;
	int type_found = 0 ;
	Type typeStruct;
	t = T_INT ;
	
	switch(get_current_token_type()) {
	case KW_CHAR:    // All of simple_type
		t = T_CHAR;
		// syntax_state = SNTX_SP;
		type_found = 1;
		get_token();
		break;
	case KW_SHORT:
		t = T_SHORT;
		// syntax_state = SNTX_SP;
		type_found = 1;
		get_token();
		break;
	case KW_VOID:
		t = T_VOID;
		// syntax_state = SNTX_SP;
		type_found = 1;
		get_token();
		break;
	case KW_INT:
		t = T_INT;
		// syntax_state = SNTX_SP;
		type_found = 1;
		get_token();
		break;
	case KW_STRUCT:
		t = T_STRUCT;
		// syntax_state = SNTX_SP;
		type_found = 1;
		struct_specifier(&typeStruct);
		type->ref = typeStruct.ref;
		break;
	default:
		break;
	}
	type->t = t;
	return type_found ;
}

/************************************************************************/
/*  功能：解析外部声明                                                  */
/*  iSaveType e_StorageClass  存储类型                                  */
/************************************************************************/
void external_declaration(e_StorageClass iSaveType)
{
	Type bTypeCurrent, typeCurrent ;
	int v, has_init, r, addr;
	Symbol * sym;
	Section * sec = NULL;
	
	if (!type_specifier(&bTypeCurrent))
	{
		print_error("Need type token\n");
	}

	if (bTypeCurrent.t = T_STRUCT && get_current_token_type() == TK_SEMICOLON)
	{
		get_token();
		return;
	}

	while (1)
	{
		typeCurrent = bTypeCurrent;
		declarator(&typeCurrent, &v, NULL);
		// XXXXXXXXXXXXXXXXXXXXXXX
		if (get_current_token_type() == TK_BEGIN)
		{
			if (iSaveType == SC_LOCAL)
			{
				print_error("Not nexsting function\n");
			}
			
			func_body(sym);
			break;
		}
		else
		{
			r = 0;
			if (!(typeCurrent.t & T_ARRAY))
			{
				r |= SC_LVAL;
			}
			r |= 1;
			// if (get_current_token_type() == TK_ASSIGN)
			has_init = (get_current_token_type() == TK_ASSIGN);
			if (has_init)  // int a = 5 ;
			{
				get_token();
			//	initializer(&typeCurrent);
			}
			sec = allocate_storage(&typeCurrent, r, has_init, v, &addr);
			sym = var_sym_put(&typeCurrent, r, v, addr);
			if (iSaveType == SC_GLOBAL)
			{
				coffsym_add_update(sym, addr, sec->index, 0, IMAGE_SYM_CLASS_EXTERNAL);
			}

			if (has_init)  // int a = 5 ;
			{
				initializer(&typeCurrent, addr, sec);
			}
			
			if(get_current_token_type() ==TK_COMMA)  // int a, b ;
			{
				get_token();
			}
			else									// int a;
			{
				syntax_state = SNTX_LF_HT;
				skip_token(TK_SEMICOLON);
				break;
			}
		}
	}
}

/************************************************************************/
/* 功能：分配存储空间                                                   */
/* type：变量类型                                                       */
/* r：变量存储类型                                                      */
/* has_in it：是否需要进行初始化                                        */
/* v：变量符号编号                                                      */
/* addr(输出) ：变量存储地址                                            */
/* 返回值：变量存储节                                                   */
/************************************************************************/
Section * allocate_storage(Type * type, int r, int has_init, int v, int *addr)
{
	int size, align;
	Section * sec= NULL;
	size = type_size(type, &align);

	if (size < 0)
	{
		if (type->t & T_ARRAY 
			&& type->ref->type.t == T_CHAR)
		{
			type->ref->c = strlen((char *)get_current_token()) + 1;
			size = type_size(type, &align);
		}
		else
			print_error("Unknown size of type");
	}

	if ((r & SC_VALMASK) == SC_LOCAL)
	{
		loc = calc_align(loc - size, align);
		*addr = loc;
	}
	else
	{
		if (has_init == 1)
		{
			sec = sec_data;
		}
		else if (has_init == 2)
		{
			sec = sec_rdata;
		}
		else
		{
			sec = sec_bss;
		}
		sec->data_offset = calc_align(sec->data_offset, align);
		*addr = sec->data_offset;
		sec->data_offset += size;

		if (sec->sh.Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA
			&& sec->data_offset > sec->data_allocated)
		{
			section_realloc(sec, sec->data_offset);
		}
		if (v == 0)
		{
			operand_push(type, SC_GLOBAL | SC_SYM, *addr);
		//	operand_stack_top->sym = sec_rdata;
		}
	}
	return sec;
}

/************************************************************************/
/* <translation_unit> ::= { <external_declaration> } | <TK_EOF>         */
/************************************************************************/
void translation_unit()
{
	while (get_current_token_type() != TK_EOF)
	{
		external_declaration(SC_GLOBAL);
	}
}


