#ifndef COFF_GENERATOR_H
#define COFF_GENERATOR_H
#include <stdio.h>
#include <windows.h>

#include "get_token.h"

#define MAXKEY	1024				// ��ϣ������

enum e_Sec_Storage{
	SEC_BSS_STORAGE   = 0,
	SEC_DATA_STORAGE  = 1,
	SEC_RDATA_STORAGE = 2
};

typedef struct Section_t{
	int data_offset;           //��ǰ����ƫ��λ��     
	char * data;               //������
	int data_allocated;        //�����ڴ�ռ�
	char index;                //�����
	struct Section_t * link;   //�����������ڣ����Žڹ����ַ�����
	int * hashtab;             //��ϣ��ֻ���ڴ洢���ű�
	IMAGE_SECTION_HEADER sh;   //��ͷ
} Section;

/* COFF���Žṹ���塣����μ���7.9 ���ű��¼��Ա����˵�� */
typedef struct CoffSym 
{
    DWORD name;					// �������ƣ��ַ������е�һ��ƫ�Ƶ�ַ��
								//           ����Է��ű��¼��ʽ��һ��С�޸ġ�����COFF�ļ��淶��
								//           ��������С�ڳ���8ʱ����������ֱ�Ӵ���ShortName�У�
								//           ������8ʱ��Short�ֶ�Ϊ0��Long�ֶα�ʾ�˷����������ַ������е�ƫ��λ�á�
								//           ���ڸ�Ϊ���۷������Ƴ����Ƿ����8�����ű����ƶ������ַ������У�
								//           Name�ֶα�ʾ�����������ַ������е�ƫ��λ�ã�
								//           �����ӵ�Next�ֶ����������ϣ��ͻ����
	DWORD next;					// ���ڱ����ͻ����
    /* 
    struct {
		DWORD   Short;			// if 0, use LongName
        DWORD   Long;			// offset into string table
    } name;
	*/
    DWORD   coff_sym_value;		// �������ص�ֵ�����ֵʹ��coffsym_add_update���á�
                                //                 ����ȫ�ֱ�����ʾ���ŵ�ַ��
                                //                 ���ں�����ʾ��ǰָ���ڴ����λ��
								//                 ������������Section Number��Storage Class��������
								//                 ��ͨ����ʾ���ض�λ�ĵ�ַ��
    short   shortSection;		// ����˷��ŵĽ���������������������ǽڱ������(��1��ʼ)�����Ա�ʶ����˷��ŵĽ�
								//                     С��1��ֵ������ĺ��壬��ϸ��Ϣ��ο���7.10��Section Number���ֵ��
    WORD    typeFunc;			// COFF�������ͣ�һ����ʾ���͵����֡�
	                            //               ����ֵĵ��ֽڶ��������������һ��ָ�룬�������ػ����������ַ��
	                            //               Microsoft���߽�ʹ�ô��ֶ���ָʾ�����Ƿ�Ϊ������
								//               ���ԣ�ֻ���������ֵ��0x0��0x20�� 
								//               ���ǣ��������߿���ʹ�ô��ֶ������������Ϣ��
	                            //               ���磬Visual C++ ������Ϣ����ָʾ���͡�
								//               ��ȷָ���������Էǳ���Ҫ�� ����������Ҫ����Ϣ��������������
	                            //               ����ĳЩ��ϵ�ṹ��������Ҫ��Щ��Ϣ������������Ŀ�ġ�
								// �μ���https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
    BYTE    storageClass;		// COFF���Ŵ洢�������һ����ʾ�洢����ö������ֵ��
	                            //                    Ҫ��ȡ������Ϣ����ο���7.11���洢���ȡֵ��
    BYTE    numberOfAuxSymbols;	// ���ڱ���¼����ĸ������ű���ĸ���
} CoffSym;

#define CST_FUNC    0x20  //Coff�������ͣ�����
#define CST_NOTFUNC 0     //Coff�������ͣ��Ǻ���

/* �ض�λ�ṹ���� */
typedef struct CoffReloc 
{
    DWORD offset;	// ��Ҫ�����ض�λ�Ĵ�������ݵĵ�ַ��
	                // ���Ǵӽڿ�ͷ�����ƫ�ƣ����Ͻڵ�RV A/Offset���ֵ��
    DWORD cfsym;	// ���ű������(��0��ʼ)��������Ÿ����������ض�λ�ĵ�ַ��
                    // ������ָ�����ŵĴ洢���Ϊ�ڣ���ô���ĵ�ַ���ǵ�һ������ͬ���Ľڵĵ�ַ��
	BYTE  section;  // ��Ҫ�ض�λ���ݵĽڱ�š�
    BYTE  type;     // �ض�λ���͡��Ϸ����ض�λ���������ڻ������ͣ�
	                // ��ο���7.13���ض�λ����ָʾ��ȡֵ��
} CoffReloc;

// �μ�7.1.4�ڡ����磬 ����ڵĵ�һ���ֽڵĵ�ַ��0xl0����ô�������ֽڵĵ�ַ����0x12





void * section_ptr_add(Section * sec, int increment);
void section_realloc(Section * sec, int new_size);
void coffreloc_add(Section * sec, Symbol * sym, int offset, char type);
int coffsym_add(Section * symtab, char * name, int val, int sec_index,
		WORD typeFunc, char StorageClass);
		// short typeFunc, char StorageClass);
void coffsym_add_update(Symbol *sym, int val, int sec_index,
		WORD typeFunc, char StorageClass);
		// short typeFunc, char StorageClass);

void init_coff();
int load_objfile(char * file_name);
void output_objfile(char * name);
void free_sections();

int  make_jmpaddr_list(int jmp_addr);
void jmpaddr_backstuff(int fill_offset, int jmp_addr);

void *mallocz(int size);

int coffsym_search(Section * symtab, char * name);
char * coffstr_add(Section * strtab, char * name);
void coffreloc_redirect_add(int offset, int cfsym, char section, char type);

void fpad(FILE * fp, int new_pos);
#endif