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
    int iat_index;       			// 符号COFF符号表中序号
    int thk_offset;      			// 代码节的缓冲区写入位置。
    IMAGE_IMPORT_BY_NAME imp_sym;	// 提示/名称表。参见10.2.7.3。
};

/* 导入模块内存存储结构 */
struct ImportInfo 
{
	int dll_index;								// 导入模块索引。
	std::vector<struct ImportSym *> imp_syms;
	IMAGE_IMPORT_DESCRIPTOR imphdr;				// 导入目录表项。参见10.2.7.1。
};

/* PE信息存储结构 */
struct PEInfo 
{
    Section *thunk;
    const char *filename;
    DWORD entry_addr;			// 程序入口点。即入口指令在代码节位置。
    DWORD imp_offs;
    DWORD imp_size;
    DWORD iat_offs;
    DWORD iat_size;
	Section **secs;				
    int   sec_size;				// 节的个数。
	std::vector<struct ImportInfo *> imps;  // 导入模块表
};

char *get_lib_path();
int pe_output_file(char * file_name);
#endif