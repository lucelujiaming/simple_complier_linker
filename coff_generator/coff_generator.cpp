// coff_generator.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "coff_generator.h"
#include <vector>


std::vector<Section> vecSection;
Section *sec_text,			// �����
		*sec_data,			// ���ݽ�
		*sec_bss,			// δ��ʼ�����ݽ�
		*sec_idata,			// ������
		*sec_rdata,			// ֻ�����ݽ�
		*sec_rel,			// �ض�λ��Ϣ��
		*sec_symtab,		// ���ű��	
		*sec_dynsymtab;		// ���ӿ���Ž�

int nsec_image;				// ӳ���ļ��ڸ���

std::vector<TkWord> tktable;

void section_realloc(Section * sec, int new_size);
int coffsym_search(Section * symtab, char * name);
char * coffstr_add(Section * strtab, char * name);
/***********************************************************
 *  ����:             �½���
 *  name:             ������
 *  characteristics:  ������
 *  ����ֵ:           �����ӽ�
 *  nsec_image:       ȫ�ֱ�����ӳ���ļ��нڸ���
 **********************************************************/
Section * section_new(char * name, int iCharacteristics)
{
	Section * sec;
	int initSize = 8;
	sec = (Section *)malloc(sizeof(Section));
	memcpy(sec->sh.Name, name, strlen(name));
	sec->sh.Characteristics = iCharacteristics;
	// sec->index = sections.count + 1;  // Start from 1
	sec->data = (char *)malloc(sizeof(char) * initSize);
	sec->data_allocated = initSize;
	if(!(iCharacteristics & IMAGE_SCN_LNK_REMOVE))
		nsec_image++;
	vecSection.push_back(*sec);
	return sec;
}

/***********************************************************
 *  ����:           ��������Ԥ������increment��С���ڴ�ռ�
 *  sec:            Ԥ���ڴ�ռ�Ľ�
 *  increment:      Ԥ���Ŀռ��С
 *  ����ֵ:         Ԥ���ڴ�ռ���׵�ַ
 **********************************************************/
void * section_ptr_add(Section * sec, int increment)
{
	int offset, offsetOne;
	offset = sec->data_offset;
	offsetOne = offset + increment;
	if(offsetOne > sec->data_allocated)
		section_realloc(sec, offsetOne);
	sec->data_offset = offsetOne;
	return sec->data + offset;
}

/***********************************************************
 *  ����:           �����������·����ڴ棬�������ݳ�ʼ��Ϊ0
 *  sec:            ���·����ڴ�Ľ�
 *  new_size:       �������³���
 **********************************************************/
void section_realloc(Section * sec, int new_size)
{
	int size;
	char * data ;
	size = sec->data_allocated;
	while(size < new_size)
		size = size * 2;
	data = (char *)realloc(sec->data, size);
	if(data == NULL)
		printf("realloc error");

	memset(data + sec->data_allocated, 0x00, size - sec->data_allocated);
	sec->data = data;
	sec->data_allocated = size;
}

/***********************************************************
 *  ����:            �½��洢COFF���ű�Ľ�
 *  symtab:          COFF���ű���
 *  Characteristics: ������
 *  strtab_name:     ����ű���ص��ַ�����
 *  ����ֵ:          �洢COFF���ű�Ľ�
 **********************************************************/
Section * new_coffsym_section(char* symtable_name, int iCharacteristics,
		char * strtable_name)
{
	Section * sec;
	sec = section_new(symtable_name, iCharacteristics);
	sec->link = section_new(strtable_name, iCharacteristics);
	sec->hashtab = (int *)malloc(sizeof(int) * MAXKEY);
	return sec;
}

/*********************************************************** 
 * ����:	�����ϣ��ַ
 * key:		��ϣ�ؼ���
 * MAXKEY:	��ϣ����
 **********************************************************/
int elf_hash(char *key)
{
    int h = 0, g;
    while (*key) 
	{
        h = (h << 4) + *key++;
        g = h & 0xf0000000;
        if (g)
            h ^= g >> 24;
        h &= ~g;
    }
    return h % MAXKEY;
}

/***********************************************************
 *  ����:            ����COFF����
 *  symtab:          ����COFF���ű�Ľ�
 *  name:            ��������
 *  val:             �������ص�ֵ
 *  sec_index:       ����˷��ŵĽ�
 *  type:            Coff��������
 *  StorageClass:    Coff���Ŵ洢���
 *  ����ֵ:          ����COFF���ű������
 **********************************************************/
int coffsym_add(Section * symtab, char * name, int val, int sec_index,
		short type, char StorageClass)
{
	CoffSym * cfsym;
	int cs, keyno;
	char * csname;
	Section * strtab = symtab->link;
	int * hashtab;
	hashtab = symtab->hashtab;
	cs = coffsym_search(symtab, name);
	if(cs = 0)  //
	{
		cfsym = (CoffSym *)section_ptr_add(symtab, sizeof(CoffSym));
		csname = coffstr_add(strtab, name);
		cfsym->Name = csname - strtab->data;   // ????
		cfsym->Value = val;
		cfsym->shortSection = sec_index;
		cfsym->Type = type;
		cfsym->StorageClass = StorageClass;
		cfsym->Value = val;
		keyno = elf_hash(name);
		cfsym->Next = hashtab[keyno];

		cs = cfsym - (CoffSym *)symtab->data;
		hashtab[keyno] = cs;
	}
	return cs;
}

/***********************************************************
 *  ����:         ���ӻ����COFF���ţ�����ֻ�����ں�����������������
 *  S:            ����ָ��
 *  val:          ����ֵ
 *  sec_index:    ����˷��ŵĽ�
 *  type:         COFF��������
 *  StorageClass: COFF���Ŵ洢���
 **********************************************************/
void coffsym_add_update(Symbol *s, int val, int sec_index,
		short type, char StorageClass)
{
	char * name;
	CoffSym *cfsym;
	if(!s->c)
	{
		name = ((TkWord)tktable[s->v]).spelling;
		s->c = coffsym_add(sec_symtab, name, val, sec_index, type, StorageClass);
	}
	else
	{
		cfsym = &((CoffSym *)sec_symtab->data)[s->c];
		cfsym->Value = val;
		cfsym->shortSection = sec_index;
	}
}

/***********************************************************
 *  ����:            ����COFF�������ַ���
 *  strtab:          ����COFF�ַ�����Ľ�
 *  name:            ���������ַ���
 *  ����ֵ:          ����COFF�ַ���
 **********************************************************/
char * coffstr_add(Section * strtab, char * name)
{
	int len;
	char * pstr;
	len = strlen(name);
	pstr = (char *)section_ptr_add(strtab, len + 1);
	memcpy(pstr, name, len);
	return pstr;
}

/***********************************************************
 * ASCII��0x2E��.�š�0x64��d�� 
 * 00 2E 64 61 74 61 00 2E 62 73 73 00 2E 72 64 61 74 61 
 *     .  d  a  t  a     .  b  s  s     .  r  d  a  t  a 
 * 00 6D 61 69 6E 00 70 72 69 6E 74 66 00 5F 65 6E 74 72 
 *     m  a  i  n     p  r  i  n  t  f     _  e  n  t  r 
 * 79 00 65 78 69 74 00                                  
 *  y     e  x  i  t 
 * �ַ�����00��β���������8���ַ�����Ŀ����һ������00��β�Ŀ��ַ����� 
 **********************************************************/

/***********************************************************
 *  ����:            ����COFF����
 *  symtab:          ����COFF���ű�Ľ�
 *  name:            ��������
 *  ����ֵ:          ����COFF���ű������
 **********************************************************/
int coffsym_search(Section * symtab, char * name)
{
	

	return 1;
}



/***********************************************************
 *  ����:            �����ض�λ��Ŀ
 *  section:         �������ڽ�
 *  sym:             ����ָ��
 *  offset:          ��Ҫ�����ض�λ�Ĵ��������������Ӧ�ڵ�ƫ��λ��
 *  type:            �ض�λ����
 **********************************************************/








/***********************************************************
 * ����:            ����COFF�ض�λ��Ϣ
 * offset:          ��Ҫ�����ض�λ�Ĵ��������������Ӧ�ڵ�ƫ��λ��
 * cf sym:          ���ű������
 * section:         �������ڽ�
 * type:            �ض�λ����
 **********************************************************/







/***********************************************************
 * *����:            COFF��ʼ��
 * *�������õ�ȫ�ֱ���:            
 * Dyn Array sections��   //������
 * Section*sec_text��     //�����
 * *sec_data��            //���ݽ�
 * *sec_bss��             //δ��ʼ�����ݽ�
 * *sec_i data��          //���˱��
 * *sec_r data��          //ֻ�����ݽ�
 * *sec_rel��             //�ض�λ��Ϣ��
 * *sec_symtab��          //���ű��
 * *sec_dynsym tab��      //���ӿ���Ž�
 * in tn sec_image��      //ӳ���ļ��ڸ���
 **********************************************************/


/***********************************************************
 *  ����:            �ͷ����н�����
 **********************************************************/



/***********************************************************
 * ����:            ���Ŀ���ļ�
 * name:            Ŀ���ļ���
 **********************************************************/




/***********************************************************
 * ����:            �ӵ�ǰ��дλ�õ�new_posλ����0��ļ�����
 * fp:            �ļ�ָ��
 * new_pos:            ��յ�λ��
 **********************************************************/





int main(int argc, char* argv[])
{
	printf("Hello World!\n");
	return 0;
}

