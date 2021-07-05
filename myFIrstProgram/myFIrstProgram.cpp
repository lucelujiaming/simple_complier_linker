// myFIrstProgram.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <ctype.h>

char * program_pointer = NULL;
char  token ;

void statement();
void expression();
void multiplicative_expression();
void primary_expression();

void get_token()
{
	token = program_pointer[0];
	program_pointer++;
}

void error(char * prompt)
{
	printf(prompt);
}


void program()
{
	while(token != '\0')
	{
		statement();
	}
}

void statement()
{
	if(token != '\n')
	{
		expression();
	}
	else
	{
		get_token();
	}
}

void expression()
{
	multiplicative_expression();
	while(token == '+' || token == '-')
	{
		get_token();
		// multiplicative_expression has the higher priority than the primary_expression
		multiplicative_expression();
	}
}

void multiplicative_expression()
{
	primary_expression();
	while(token == '*' || token == '/')
	{
		get_token();
		// multiplicative_expression has the higher priority than the primary_expression
		primary_expression();
	}
}

void primary_expression()
{
	// if(token == INTEGER)
	if(isdigit(token))
	{
		get_token();
	}
	else if(token == '(')
	{
		get_token();
		expression();
		if(token != ')')
			error("No )");
		get_token();
	}
	else
	{
		error("error");
	}
}

int main(int argc, char* argv[])
{
	program_pointer = "(1+2)*3+5\n";
	get_token();
	program();
	printf("Hello World!\n");
	return 0;
}

