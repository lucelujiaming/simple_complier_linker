#ifndef PE_GENERATOR_H
#define PE_GENERATOR_H

#include <windows.h>
#pragma warning(disable : 4786)

#include "get_token.h"
#include "translation_unit.h"

/* ������� */
enum e_OutType
{	
    OUTPUT_OBJ,		// Ŀ���ļ�
	OUTPUT_EXE,		// EXE��ִ���ļ�
    OUTPUT_MEMORY	// �ڴ���ֱ�����У������
};

/* ��������ڴ�洢�ṹ */
struct ImportSym 
{
    int iat_index;
    int thk_offset;
    IMAGE_IMPORT_BY_NAME imp_sym;
};

/* ����ģ���ڴ�洢�ṹ */
struct ImportInfo 
{
	int dll_index;
	std::vector<struct ImportSym *> imp_syms;
	IMAGE_IMPORT_DESCRIPTOR imphdr;
};

/* PE��Ϣ�洢�ṹ */
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