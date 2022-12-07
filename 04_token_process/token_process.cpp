// token_process.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "get_token.h"


int main(int argc, char* argv[])
{

	token_init(argv[1]);

	do{
		get_token();
		push_token(get_current_token());
		color_token(LEX_NORMAL, get_current_token_type(), get_current_token());
	} while(get_current_token_type() != TK_EOF);

	token_cleanup();
	printf("Success !\n");
	return 1;
}
