// semantic_analyser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "get_token.h"
#include "translation_unit.h"


int main(int argc, char* argv[])
{
	token_init(argv[1]);
	get_token();
	translation_unit();
	printf("Hello World!\n");
	token_cleanup();
	return 0;
}


