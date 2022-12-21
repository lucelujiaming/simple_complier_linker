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
    int iat_index;       			// ����COFF���ű������
    int thk_offset;      			// ����ڵĻ�����д��λ�á�
    IMAGE_IMPORT_BY_NAME imp_sym;	// ��ʾ/���Ʊ��μ�10.2.7.3��
};

/* ����ģ���ڴ�洢�ṹ */
struct ImportInfo 
{
	int dll_index;								// ����ģ��������
	std::vector<struct ImportSym *> imp_syms;
	IMAGE_IMPORT_DESCRIPTOR imphdr;				// ����Ŀ¼����μ�10.2.7.1��
};

/* PE��Ϣ�洢�ṹ */
struct PEInfo 
{
    Section *thunk;
    const char *filename;
    DWORD entry_addr;			// ������ڵ㡣�����ָ���ڴ����λ�á�
    DWORD imp_offs;
    DWORD imp_size;
    DWORD iat_offs;
    DWORD iat_size;
	Section **secs;				
    int   sec_size;				// �ڵĸ�����
	std::vector<struct ImportInfo *> imps;  // ����ģ���
};

char *get_lib_path();
int pe_output_file(char * file_name);
#endif