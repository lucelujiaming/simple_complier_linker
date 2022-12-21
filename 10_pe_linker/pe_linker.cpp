// pe_linker.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#pragma warning(disable : 4786)
#include "get_token.h"
#include "translation_unit.h"
#include "symbol_table.h"
#include "pe_generator.h"

std::vector<char *> vecSrcFiles;
char *output_file;					// 输出文件名

extern std::vector<std::string> vecDllName;
extern std::vector<std::string> vecLib;
extern short g_subsystem;

extern int   g_output_type;

char *get_file_ext(char *fname);
int process_command(int argc, char** argv);
void compile(char * file_name);

/************************************************************************/
/* 这个程序的用法如下：                                                 */
/*   1. 设定启动参数为-o HelloWorld.obj -c HelloWorld.c，               */
/*      生成目标文件。                                                  */
/*   2. 设定启动参数为-lmsvcrt -o HelloWorld.exe HelloWorld.obj，       */
/*      生成可执行文件。                                                */
/************************************************************************/
int main(int argc, char* argv[])
{
	int i,opind;
	char *ext;
	char * file_name;
	if (argc < 2)
	{
		print_error("No program", "");
	}
	init();
	opind = process_command(argc, argv);
	if (opind == 0)
	{
		cleanup();
		return 0;
	}
	for (i = 0; i < vecSrcFiles.size(); i++)
	{
		file_name = vecSrcFiles[i];
		ext = get_file_ext(file_name);
		if (!strcmp(ext, "c"))
		{
			compile(file_name);
		}
		else if (!strcmp(ext, "obj"))
		{
			load_obj_file(file_name);
		}
	}
	if (g_output_type == OUTPUT_OBJ)
	{
		output_obj_file(output_file);
	}
	else
	{
		pe_output_file(output_file);
	}
	token_cleanup();
	return 0;
}

int process_command(int argc, char** argv)
{
	int i ;
	for (i = 1; i< argc; i++)
	{
		if (argv[i][0] == '-')
		{
			char *pParam = &argv[i][1];
			int c = *pParam ;
			switch(c) {
			case 'o':
				output_file = argv[++i];
				break;
			case 'c':
				vecSrcFiles.push_back(argv[++i]);
				g_output_type = OUTPUT_OBJ;
				return 1;
			case 'l':
				vecLib.push_back(std::string(&argv[i][2]));
				break;
			case 'G':
				g_subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
				break;
			case 'v':
				printf("Version Info");
				return 0;
			case 'h':
				printf("usage: scc [-c infile] [-o outfile] [-llib] [infile1 infile2...] \n");
				return 0;
			default:
				printf("unsupported command line option");
				return 0;
			}
		}
		else
		{
			vecSrcFiles.push_back(argv[i]);
		}
	}
	return 1;
}

/***********************************************************
 * 功能:	得到文件扩展名
 * fname:	文件名称
 **********************************************************/
char *get_file_ext(char *fname)
{
	char *p;
	p = strrchr(fname,'.');
	return p+1;
}

void compile(char * file_name)
{
	// FILE * compile_fp = fopen(file_name, "rb");
	// if (!compile_fp)
	//		printf("Error open %s \r\n", file_name);

	token_init(file_name);
	get_token();
	translation_unit();
	printf("Hello World!\n");
}