#ifndef COFF_GENERATOR_H
#define COFF_GENERATOR_H
#include <stdio.h>
#include <windows.h>


#define MAXKEY	1024				// ��ϣ������

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
    DWORD   value;				// �������ص�ֵ
    short   shortSection;		// �ڱ������(��1��ʼ),���Ա�ʶ����˷��ŵĽ�*/
    WORD    type;				// һ����ʾ���͵�����
    BYTE    storageClass;		// ����һ����ʾ�洢����ö������ֵ
    BYTE    numberOfAuxSymbols;	// ���ڱ���¼����ĸ������ű���ĸ���
} CoffSym;

/* ���ʹ洢�ṹ���� */
typedef struct Type 
{
    int t;
    struct Symbol *ref;
} Type;

/* ���Ŵ洢�ṹ���� */
typedef struct Symbol 
{
    int v;						// ���ŵĵ��ʱ���
    int r;						// ���Ź����ļĴ���
    int c;						// ���Ź���ֵ
    Type type;					// ��������
    struct Symbol *next;		// �������������ţ��ṹ�嶨�������Ա�������ţ��������������������
    struct Symbol *prev_tok;	// ָ��ǰһ�����ͬ������
} Symbol;

/* ���ʴ洢�ṹ���� */
typedef struct TkWord
{
    int  tkcode;					// ���ʱ��� 
    struct TkWord *next;			// ָ���ϣ��ͻ����������
    char *spelling;					// �����ַ��� 
    struct Symbol *sym_struct;		// ָ�򵥴�����ʾ�Ľṹ����
    struct Symbol *sym_identifier;	// ָ�򵥴�����ʾ�ı�ʶ��
} TkWord;

/* �ض�λ�ṹ���� */
typedef struct CoffReloc 
{
    DWORD offset;	// ��Ҫ�����ض�λ�Ĵ�������ݵĵ�ַ
    DWORD cfsym;				// ���ű������(��0��ʼ)
	BYTE  section;  // �˴���һ��Ϊʲô��COFF�ض�λ�ṹ�����޸ļ�¼Section��Ϣ*/
    BYTE  type;    
} CoffReloc;

#endif