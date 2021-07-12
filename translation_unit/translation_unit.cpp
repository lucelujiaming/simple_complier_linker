// translation_unit.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "get_token.h"
#include "translation_unit.h"

e_SynTaxState syntax_state = SNTX_NUL;
int syntax_level;

int type_specifier();
void declarator();
void direct_declarator();
void direct_declarator_postfix();
void parameter_type_list(); // (int func_call);

void compound_statement();
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

void print_error(char * strErrInfo)
{
	printf("<ERROR> %s", strErrInfo);
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
void struct_member_alignment() // (int *fc)
{
	// Do not need indent or not
	if(get_current_token_type() == KW_ALIGN)
	{
		get_token();
		skip_token(TK_OPENPA);
		if(get_current_token_type() == TK_CINT)
		{
			get_token();
		}
		else
		{
			print_error("Need intergat\n");
		}
		// *fc = token;
		skip_token(TK_CLOSEPA);
	}
}

/* <declarator> ::= {<TK_STAR>}[<function_calling_convention>]               */
/*                    [<struct_member_alignment>]<direct_declarator>         */
/* Such as * __cdecl __align(4) function(int a, int b)                       */ 
void declarator()
{
	int fc ;
	while(get_current_token_type() == TK_STAR)		// * 星号
	{
		get_token();
	}
	function_calling_convention(&fc);
	struct_member_alignment();  // &fc
	direct_declarator();
}

/* <direct_declarator> ::= <IDENTIFIER><direct_declarator_postfix>              */
void direct_declarator()   // Not support argv[] usage
{
	if(get_current_token_type() >= TK_IDENT)  // 函数名或者是变量名，不可以是保留关键字。 
	{
		get_token();
	}
	else
	{
		print_error("direct_declarator can not be TK_IDENT\n");
	}
	direct_declarator_postfix();
}

/* <direct_declarator_postfix> :: = {<TK_OPENBR><TK_CINT><TK_CLOSEBR>       (1)  */
/*                         | <TK_OPENBR><TK_CLOSEBR>                        (2)  */
/*                         | <TK_OPENPA><parameter_type_list><TK_CLOSEPA>， (3)  */
/*                         | <TK_OPENPA><TK_CLOSEPA>}                       (4)  */
/* Such as var, var[5], var(), var(int a, int b), var[]                          */ 
void direct_declarator_postfix()
{
//	int n;
	if(get_current_token_type() == TK_OPENPA)         // <TK_OPENPA><parameter_type_list><TK_CLOSEPA> | <TK_OPENPA><TK_CLOSEPA>
	{
		parameter_type_list();
	}
	else if(get_current_token_type() == TK_OPENBR)   // <TK_OPENBR><TK_CINT><TK_CLOSEBR> | <TK_OPENBR><TK_CLOSEBR>
	{
		get_token();
		if(get_current_token_type() == TK_CINT)
		{
			get_token();
			// n = tkvalue;  // ??
		}
		else
		{
			print_error("Need intergat\n");
		}
		skip_token(TK_CLOSEBR);
		direct_declarator_postfix();    // Nesting calling
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
void parameter_type_list() // (int func_call)
{
	get_token();
	while(get_current_token_type() != TK_CLOSEPA)   // get_token until meet ) 右圆括号
	{
		if(!type_specifier())
		{
			print_error("Invalid type_specifier\n");
		}
		declarator();			// Translate one parameter declaration
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
}

// ------------------ func_body 
/************************************************
 *  <func_body> ::= <compound_statement>
 *  因为只要见到<{ 左大括号>就是复合语句，所以<复合语句>的概念比<函数体>概念要大，
 *  <函数体>属于一类比较特殊的<复合语句>。
 ***********************/
void func_body()
{
	compound_statement();
}

// 初值符的代码如下所示。
/***********************************************************
* <initializer> ::= <assignment_expression>
*********************************************/
void initializer()
{
	assignment_expression();
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
		compound_statement();			// 复合语句
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

void compound_statement()
{
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

/***********************************************************
 *  <sizeof_expression> ::= 
 *     <KW_SIZEOF><TK_OPENPA><type_specifier><TK_CLOSEPA>
 **********************************************************/
void sizeof_expression()
{
	get_token();
	skip_token(TK_OPENPA);
	type_specifier();
	skip_token(TK_CLOSEPA);
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
	int t ;
	switch(get_current_token_type()) {
	case TK_CINT:
	case TK_CCHAR:
	case TK_CSTR:
		get_token();
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
void struct_declaration(int * max, int * offset)
{
	type_specifier();
	while (1)
	{
		declarator();
		if (get_current_token_type() == TK_SEMICOLON)
		{
			break;
		}
		skip_token(TK_COMMA);
	}
	syntax_state = SNTX_LF_HT;
	skip_token(TK_SEMICOLON);
}

/************************************************************************/
/*  <struct_declaration_list> ::= <struct_declaration>                  */
/*                               {<struct_declaration>}                 */
/************************************************************************/
void struct_declaration_list()
{
	int maxalign, offset;
	syntax_state = SNTX_LF_HT;
	syntax_level++;

	get_token();
	while (get_current_token_type() != TK_END)  // } 右大括号
	{
		struct_declaration(&maxalign, &offset);
	}
	skip_token(TK_END);
	syntax_state = SNTX_LF_HT;
}

/************************************************************************/
/*  <struct_specifier> ::= <KW_STRUCT><IDENTIFIER><TK_BEGIN>            */
/*                         <struct_declaration_list><TK_END>            */
/*                       | <KW_STRUCT><IDENTIFIER>                      */
/************************************************************************/
void struct_specifier()
{
	int v ;
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

	if (get_current_token_type() == TK_BEGIN)
	{
		struct_declaration_list();
	}
}

/************************************************************************/
/* <type_specifier> ::= <KW_CHAR> | <KW_SHORT>                          */
/*                    | <KW_VOID> | <KW_INT> | <struct_specifier>       */
/************************************************************************/
int type_specifier()
{
	int type_found = 0 ;
	switch(get_current_token_type()) {
	case KW_CHAR:    // All of simple_type
	case KW_SHORT:
	case KW_VOID:
	case KW_INT:
		syntax_state = SNTX_SP;
		type_found = 1;
		get_token();
		break;
	case KW_STRUCT:
		syntax_state = SNTX_SP;
		type_found = 1;
		struct_specifier();
		break;
	default:
		break;
	}
	return type_found ;
}

void function_body()
{
	compound_statement();
}

/************************************************************************/
/*  iSaveType e_StorageClass  存储类型                                  */
/************************************************************************/
void external_declaration(e_StorageClass iSaveType)
{
	if (!type_specifier())
	{
		print_error("Need type token\n");
	}

	if (get_current_token_type() == TK_SEMICOLON)
	{
		get_token();
		return;
	}

	while (1)
	{
		declarator();
		if (get_current_token_type() == TK_BEGIN)
		{
			if (iSaveType == SC_LOCAL)
			{
				print_error("Not nexsting function\n");
			}
			function_body();
			break;
		}
		else
		{
			if (get_current_token_type() == TK_ASSIGN)  // int a = 5 ;
			{
				get_token();
				initializer();
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
/* <translation_unit> ::= { <external_declaration> } | <TK_EOF>         */
/************************************************************************/
void translation_unit()
{
	while (get_current_token_type() != TK_EOF)
	{
		external_declaration(SC_GLOBAL);
	}
}



int main(int argc, char* argv[])
{
	token_init(argv[1]);
	get_token();
	translation_unit();
	printf("Hello World!\n");
	token_cleanup();
	return 0;
}

