// translation_unit.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "get_token.h"
#include "translation_unit.h"

e_SynTaxState syntax_state = SNTX_NUL;

int struct_specifier()
{
	return 1;
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

void declarator()
{
	
}

void initializer()
{
	
}

void function_body()
{
	
}

/************************************************************************/
/*  iSaveType e_StorageClass  ¥Ê¥¢¿‡–Õ                                  */
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

