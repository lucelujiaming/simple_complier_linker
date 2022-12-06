// pe_linker.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "get_token.h"
#include "translation_unit.h"

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		print_error("No program");
	}
	init();
	token_init(argv[1]);
	get_token();
	translation_unit();
	printf("Hello World!\n");
	free_sections();
	token_cleanup();
	return 0;
}

