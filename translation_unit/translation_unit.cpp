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

void syntax_indent()
{

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
			printf("Need intergat");
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
void direct_declarator()
{
	if(get_current_token_type() >= TK_IDENT)  // 函数名或者是变量名，不可以是保留关键字。 
	{
		get_token();
	}
	else
	{
		printf("direct_declarator can not be TK_IDENT");
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
			printf("Need intergat");
		}
		skip_token(TK_CLOSEBR);
		direct_declarator_postfix();    // Nesting calling
	}
	else
	{
		printf("Wrong direct_declarator_postfix grammer");
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
	while(get_current_token_type() == TK_CLOSEPA)   // get_token until meet ) 右圆括号
	{
		if(!type_specifier())
		{
			printf("Invalid type_specifier");
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





/************************************************************************/
/*  <struct_declaration> ::= <>                  */
/*                               {<>}                 */
/************************************************************************/
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
		printf("Need struct name");
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

void initializer()
{
	
}

void function_body()
{
	
}

/************************************************************************/
/*  iSaveType e_StorageClass  存储类型                                  */
/************************************************************************/
void external_declaration(e_StorageClass iSaveType)
{
	if (!type_specifier())
	{
		printf("Need type token");
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
				printf("Not nexsting function");
			}
			function_body();
			break;
		}
		else
		{
			if (get_current_token_type() == TK_ASSIGN)
			{
				get_token();
				initializer();
			}
			
			if(get_current_token_type() ==TK_COMMA)
			{
				get_token();
			}
			else
			{

			}
		}
	}
}

/************************************************************************/
/* <translation_unit> ::= { <external_declaration> } | <TK_EOF>         */
/************************************************************************/
void translation_unit()
{
	if (get_current_token_type() != TK_EOF)
	{
		external_declaration(SC_GLOBAL);
	}
}

int main(int argc, char* argv[])
{
	printf("Hello World!\n");
	return 0;
}

