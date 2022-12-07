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

/* COFF���Žṹ���� */
typedef struct CoffSym 
{

    DWORD name;					// ��������
	DWORD next;					// ���ڱ����ͻ����*/
    /* 
    struct {
		DWORD   Short;			// if 0, use LongName
        DWORD   Long;			// offset into string table
    } name;
	*/
    DWORD   coff_sym_value;		// �������ص�ֵ��
                                // ���ֵʹ��coffsym_add_update���á�
                                // ����ȫ�ֱ�����ʾ���ŵ�ַ��
                                // ���ں�����ʾ��ǰָ���ڴ����λ��
    short   shortSection;		// �ڱ������(��1��ʼ),���Ա�ʶ����˷��ŵĽ�*/
    WORD    type;				// һ����ʾ���͵�����
    BYTE    storageClass;		// ����һ����ʾ�洢����ö������ֵ
    BYTE    numberOfAuxSymbols;	// ���ڱ���¼����ĸ������ű���ĸ���
} CoffSym;

#define CST_FUNC    0x20  //Coff�������ͣ�����
#define CST_NOTFUNC 0     //Coff�������ͣ��Ǻ���

/* �ض�λ�ṹ���� */
typedef struct CoffReloc 
{
    DWORD offset;	// ��Ҫ�����ض�λ�Ĵ�������ݵĵ�ַ
    DWORD cfsym;				// ���ű������(��0��ʼ)
	BYTE  section;  // �˴���һ��Ϊʲô��COFF�ض�λ�ṹ�����޸ļ�¼Section��Ϣ*/
    BYTE  type;    
} CoffReloc;

void section_realloc(Section * sec, int new_size);
void coffreloc_add(Section * sec, Symbol * sym, int offset, char type);
void coffsym_add_update(Symbol *sym, int val, int sec_index,
		short type, char StorageClass);

void init_coff();
void write_obj(char * name);
void free_sections();

int  make_jmpaddr_list(int jmp_addr);
void jmpaddr_backstuff(int fill_offset, int jmp_addr);
#endif