#ifndef PE_GENERATOR_H
#define PE_GENERATOR_H

#include <windows.h>
#pragma warning(disable : 4786)

#include "get_token.h"
#include "translation_unit.h"

/* 输出类型 */
enum e_OutType
{	
    OUTPUT_OBJ,		// 目标文件
	OUTPUT_EXE,		// EXE可执行文件
    OUTPUT_MEMORY	// 内存中直接运行，不输出
};

/* 导入符号内存存储结构 */
struct ImportSym 
{
    int iat_index;
    int thk_offset;
    IMAGE_IMPORT_BY_NAME imp_sym;
};

/* 导入模块内存存储结构 */
struct ImportInfo 
{
	int dll_index;
	std::vector<struct ImportSym *> imp_syms;
	IMAGE_IMPORT_DESCRIPTOR imphdr;
};

/* PE信息存储结构 */
struct PEInfo 
{
    Section *thunk;
    const char *filename;
    DWORD entry_addr;
    DWORD imp_offs;
    DWORD imp_size;
    DWORD iat_offs;
    DWORD iat_size;
	Section **secs;
    int   sec_size;
	std::vector<struct ImportInfo *> imps;
};

char *get_lib_path();
int load_obj_file(char * file_name);
int pe_output_file(char * file_name);
#endif