// coff_generator.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "coff_generator.h"
#include <vector>


std::vector<Section> vecSection;
Section *sec_text,			// �����
		*sec_data,			// ���ݽ�
		*sec_bss,			// δ��ʼ�����ݽ�
		*sec_idata,			// �������
		*sec_rdata,			// ֻ�����ݽ�
		*sec_rel,			// �ض�λ��Ϣ��
		*sec_symtab,		// ���ű���	
		*sec_dynsymtab;		// ���ӿ���Ž�

int nsec_image;				// ӳ���ļ��ڸ���

std::vector<TkWord> tktable;

void section_realloc(Section * sec, int new_size);
int coffsym_search(Section * symtab, char * name);
char * coffstr_add(Section * strtab, char * name);
void coffreloc_redirect_add(int offset, int cfsym, char section, char type);


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
 *  ����:            �½��洢COFF���ű��Ľ�
 *  symtab:          COFF���ű���
 *  Characteristics: ������
 *  strtab_name:     ����ű���ص��ַ�����
 *  ����ֵ:          �洢COFF���ű��Ľ�
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
 * MAXKEY:	��ϣ������
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
 *  symtab:          ����COFF���ű��Ľ�
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
		cfsym->name = csname - strtab->data;   // ????
		cfsym->value = val;
		cfsym->shortSection = sec_index;
		cfsym->type = type;
		cfsym->storageClass = StorageClass;
		cfsym->value = val;
		keyno = elf_hash(name);
		cfsym->next = hashtab[keyno];

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
		cfsym->value = val;
		cfsym->shortSection = sec_index;
	}
}

/***********************************************************
 *  ����:            ����COFF�������ַ���
 *  strtab:          ����COFF�ַ������Ľ�
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
 *  symtab:          ����COFF���ű��Ľ�
 *  name:            ��������
 *  ����ֵ:          ����COFF���ű������
 **********************************************************/
int coffsym_search(Section * symtab, char * name)
{
	CoffSym * cfsym;
	int cs, keyno ;
	char * csname ;
	Section * strtab;
	
	keyno  = elf_hash(name);
	strtab = symtab->link;
	cs     = symtab->hashtab[keyno];
	while(cs)
	{
		cfsym  = (CoffSym *)symtab->data + cs;
		csname = strtab->data + cfsym->name;
		if(!strcmp(name, csname))
			return cs;
		cs = cfsym->next;
	}
	return cs;
}

/***********************************************************
 *  ����:            �����ض�λ��Ŀ
 *  section:         �������ڽ�
 *  sym:             ����ָ��
 *  offset:          ��Ҫ�����ض�λ�Ĵ��������������Ӧ�ڵ�ƫ��λ��
 *  type:            �ض�λ����
 **********************************************************/
void coffreloc_add(Section * sec, Symbol * sym, int offset, char type)
{
	int cfsym;
	char * name;
	if(!sym->c)
		coffsym_add_update(sym, 0, 
			IMAGE_SYM_UNDEFINED, 0,  // CST_FUNC,
			IMAGE_SYM_CLASS_EXTERNAL);
	name = ((TkWord)tktable[sym->v]).spelling;
	cfsym = coffsym_search(sec_symtab, name);
	coffreloc_redirect_add(offset, cfsym, sec->index, type);
}

/***********************************************************
 * ����:            ����COFF�ض�λ��Ϣ
 * offset:          ��Ҫ�����ض�λ�Ĵ��������������Ӧ�ڵ�ƫ��λ��
 * cf sym:          ���ű�������
 * section:         �������ڽ�
 * type:            �ض�λ����
 **********************************************************/
void coffreloc_redirect_add(int offset, int cfsym, char section, char type)
{
	CoffReloc * rel;
	rel = (CoffReloc *)section_ptr_add(sec_rel, sizeof(CoffReloc));
	rel->offset  = offset;
	rel->cfsym   = cfsym;
	rel->section = section;
	rel->type    = type;
}

/***********************************************************
 * *����:            COFF��ʼ��
 * *�������õ�ȫ�ֱ���:            
 * Dyn Array sections��       //������
 * Section*sec_text��         //�����
 *         *sec_data��        //���ݽ�
 *         *sec_bss��         //δ��ʼ�����ݽ�
 *         *sec_idata��       //���˱���
 *         *sec_rdata��       //ֻ�����ݽ�
 *         *sec_rel��         //�ض�λ��Ϣ��
 *         *sec_symtab��      //���ű���
 *         *sec_dynsym tab��  //���ӿ���Ž�
 * in tn sec_image��          //ӳ���ļ��ڸ���
 **********************************************************/
void init_coff()
{
	vecSection.resize(8);
	nsec_image = 0 ;
	
	sec_text = section_new(".text",
			IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE);
	sec_data = section_new(".data",
			IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | 
	        IMAGE_SCN_CNT_INITIALIZED_DATA);    
	sec_idata = section_new(".idata",
			IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);  
	sec_rdata = section_new(".rdata",
			IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | 
	        IMAGE_SCN_CNT_INITIALIZED_DATA);  
	sec_bss = section_new(".bss",
			IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | 
	        IMAGE_SCN_CNT_INITIALIZED_DATA);     
	sec_rel = section_new(".rel",
			IMAGE_SCN_LNK_REMOVE | IMAGE_SCN_MEM_READ);   
			
	sec_symtab = new_coffsym_section(".symtab",
			IMAGE_SCN_LNK_REMOVE | IMAGE_SCN_MEM_READ, ".strtab");  
	sec_dynsymtab = new_coffsym_section(".dynsym",
			IMAGE_SCN_LNK_REMOVE | IMAGE_SCN_MEM_READ, ".dynstr");

	coffsym_add(sec_symtab, "", 0, 0, 0, IMAGE_SYM_CLASS_NULL);
	coffsym_add(sec_symtab, ".data", 0, sec_data->index, 0, IMAGE_SYM_CLASS_STATIC);
	coffsym_add(sec_symtab, ".bss", 0, sec_bss->index, 0, IMAGE_SYM_CLASS_STATIC);
	coffsym_add(sec_symtab, ".rdata", 0, sec_rdata->index, 0, IMAGE_SYM_CLASS_STATIC);
	coffsym_add(sec_dynsymtab, "", 0, 0, 0, IMAGE_SYM_CLASS_NULL);
}

/***********************************************************
 *  ����:            �ͷ����н�����
 **********************************************************/
void free_section()
{
	int i;
	Section * sec;
	for(i = 0; i < vecSection.size(); i++)
	{
		sec = &((Section)vecSection[i]);
		if(sec->hashtab != NULL)
			free(sec->hashtab);
		free(sec->data);
	}
}

void fpad(FILE * fp, int new_pos)
{
	int curpos = ftell(fp);
	while (++curpos <= new_pos)
	{
		fputc(0, fp);
	}
}

/***********************************************************
 * ����:            ���Ŀ���ļ�
 * name:            Ŀ���ļ���
 **********************************************************/
void write_obj(char * name)
{
	int file_offset;
	FILE * fout = fopen(name, "wb");
	int i, sh_size, nsec_obj = 0;
	IMAGE_FILE_HEADER *fh;
	
	nsec_obj = vecSection.size() -2 ;
	sh_size  = sizeof(IMAGE_SECTION_HEADER);
	file_offset = sizeof(IMAGE_FILE_HEADER) + nsec_obj * sh_size;
	fpad(fout, file_offset);
	fh = (IMAGE_FILE_HEADER *)malloc(sizeof(IMAGE_FILE_HEADER));
	// Write File Sections
	for(i = 0; i < nsec_obj; i++)
	{
		Section * sec = &vecSection[i];
		if(sec->data == NULL)
			continue;
		fwrite(sec->data, 1, sec->data_offset, fout);
		sec->sh.PointerToRawData = file_offset;
		sec->sh.SizeOfRawData = sec->data_offset;
		file_offset += sec->data_offset;
	}
	fseek(fout, SEEK_SET, 0);
	// Write File Header
	fh->Machine = IMAGE_FILE_MACHINE_I386;
	fh->NumberOfSections = nsec_obj;
	fh->PointerToSymbolTable = sec_symtab->sh.PointerToRawData;
	fh->NumberOfSymbols = sec_symtab->sh.SizeOfRawData / sizeof(CoffSym);
	fwrite(fh, 1, sizeof(IMAGE_FILE_HEADER), fout);
	for (i =0; i < nsec_obj; i++)
	{
		Section * sec = &vecSection[i];
		fwrite(sec->sh.Name, 1, sh_size, fout);
	}
	free(fh);
	fclose(fout);
}

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
