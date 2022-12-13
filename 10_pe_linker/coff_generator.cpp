// coff_generator.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "coff_generator.h"
#include <vector>


std::vector<Section *> vecSection;
Section *sec_text,			// �����
		*sec_data,			// ���ݽ�
		*sec_bss,			// δ��ʼ�����ݽ�
		*sec_idata,			// ������
		*sec_rdata,			// ֻ�����ݽ�
		*sec_rel,			// �ض�λ��Ϣ��
		*sec_symtab,		// ���ű��	
		*sec_dynsymtab;		// ���ӿ���Ž�

int nsec_image;				// ӳ���ļ��ڸ���

extern std::vector<TkWord> tktable;
extern int sec_text_opcode_offset ;	 	// ָ���ڴ����λ��

/************************************************************************/
/*  ����:             �½���                                            */
/*  name:             ������                                            */
/*  characteristics:  ������                                            */
/*  ����ֵ:           �����ӽ�                                          */
/*  nsec_image:       ȫ�ֱ�����ӳ���ļ��нڸ���                        */
/************************************************************************/
Section * section_new(char * name, int iCharacteristics)
{
	Section * sec;
	int initSize = 8;
	sec = (Section *)mallocz(sizeof(Section));
	memcpy(sec->sh.Name, name, strlen(name));
	sec->sh.Characteristics = iCharacteristics;
	sec->index = vecSection.size() + 1;  // Start from 1
	sec->data = (char *)mallocz(sizeof(char) * initSize);
	sec->data_allocated = initSize;
	// IMAGE_SCN_LNK_REMOVE����˽ڲ����Ϊ�����γɵ�ӳ���ļ���һ���֡�
	// �˱�־����Ŀ���ļ��Ϸ���
	// ͨ�������־���жϸý��Ƿ���Ҫ����ӳ���ļ��У�
	// �����Ҫ����ӳ���ļ��� nsec_image�Զ���1�� 
	// �����н��½���ɺ� nsec_image������¼��ӳ���ļ��Ľڸ�����
	if(!(iCharacteristics & IMAGE_SCN_LNK_REMOVE))
		nsec_image++;
	vecSection.push_back(sec);
	return sec;
}

/************************************************************************/
/*  ����:           �����������·����ڴ棬�������ݳ�ʼ��Ϊ0             */
/*  sec:            ���·����ڴ�Ľ�                                    */
/*  new_size:       �������³���                                        */
/************************************************************************/
void section_realloc(Section * sec, int new_size)
{
	int size;
	char * data ;
	// ȡ�õ�ǰ�Ŀռ��С�������section_new�г�ʼ��Ϊ8��
	size = sec->data_allocated;
	// ��ǰ�Ŀռ��С����2��ֱ������������³��ȡ���Ȼ���ֵΪ8���ݡ�
	while(size < new_size)
		size = size * 2;
	// ʹ��realloc�����Ѿ����õ��ڴ�ռ䡣
	data = (char *)realloc(sec->data, size);
	if(data == NULL)
		printf("realloc error");
	// ��Ϊ�Ǹ����Ѿ����õ��ڴ�ռ䡣
	// ����ϣ��Ѿ����ù��ռ䱻����������ֻ��Ҫ��ʼ���·���Ŀռ䡣
	memset(data + sec->data_allocated, 0x00, size - sec->data_allocated);
	sec->data = data;
	sec->data_allocated = size;
}

/************************************************************************/
/*  ����:           ��������Ԥ������increment��С���ڴ�ռ�             */
/*  sec:            Ԥ���ڴ�ռ�Ľ�                                    */
/*  increment:      Ԥ���Ŀռ��С                                      */
/*  ����ֵ:         Ԥ���ڴ�ռ���׵�ַ                                */
/************************************************************************/
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

/************************************************************************/
/*  ����:            �½��洢COFF���ű�Ľ�                             */
/*  symtab:          COFF���ű���                                       */
/*  Characteristics: ������                                             */
/*  strtab_name:     ����ű���ص��ַ�����                             */
/*  ����ֵ:          �洢COFF���ű�Ľ�                                 */
/************************************************************************/
Section * new_coffsym_section(char* symtable_name, int iCharacteristics,
		char * strtable_name)
{
	Section * sec;
	sec = section_new(symtable_name, iCharacteristics);
	sec->link = section_new(strtable_name, iCharacteristics);
	sec->hashtab = (int *)mallocz(sizeof(int) * MAXKEY);
	return sec;
}

/************************************************************************/
/* ����:	������ڴ沢�����ݳ�ʼ��Ϊ'0'                               */
/* size:	�����ڴ��С                                                */
/************************************************************************/
void *mallocz(int size)
{
    void *ptr;
	ptr = malloc(size);
	if (!ptr && size)
    {
		printf("�ڴ����ʧ��");
		exit(1);
	}
    memset(ptr, 0, size);
    return ptr;
}

/************************************************************************/
/* ����:	�����ϣ��ַ                                                */
/* key:		��ϣ�ؼ���                                                  */
/* MAXKEY:	��ϣ����                                                  */
/*   �������ǵ�hash�����һ��unsigned int���͵����ݣ�                   */
/*          0000 0000 0000 0000                                         */
/*   1. hash����4λ����key���롣��Ϊһ��char�а�λ��                    */
/*      ��ᵼ�µ�һ���ֽڵĸ���λ�������е�һ�����ۡ�                  */
/*   2. ������bites_mask & 0xf0000000��ȡ��hash�ĵ��ĸ��ֽڵĸ���λ��   */
/*      ���ø���λ��Ϊ�������ڶ������ۡ�                                */
/*	                                                                    */
/*      ������������������һ�£���Ϊ���ǵ�ELF hashǿ������ÿ���ַ�      */
/*      ��Ҫ�����Ľṹ��Ӱ�죬����˵�������Ƶ�һ���̶��ǻ��̵�        */
/*	    ��ߵ���λ�ģ�����˵����Ҫ����ߵ���λ�ȶԴ�����Ӱ�죬          */
/*	    ���������̵���֮������е�Ӱ�춼�ǵ��ӵģ�                      */
/*	    ����Ƕ�ε����۱�֤ɢ�о��ȣ���ֹ���ֳ�ͻ�Ĵ������֡�          */
/*   3. ��bites_mask����24λ�ƶ����ղŵ�5-8λ���                     */
/*      �ٶ�5-8λ���еڶ������ۡ�                                       */
/*   4. ���Ƕ�ʱ��ո���λ��ʵ�����ⲿ����������ȫû�б�Ҫ��            */
/*      �����㷨Ҫ����Ϊ������һ�ε����ƻ��Զ��̵�����λ��            */
/*   5. key������������һ���ַ��������ۡ�                               */
/*   6. ����һ��ȱʧ����߷���λ���޷�����                              */
/*      ��Ϊ�˷�ֹ�õ����з��ŵ�ʱ����ɵ��������Ϊ����hashֵ        */
/************************************************************************/
int elf_hash(char *key)
{
    unsigned int hash = 0, bites_mask;
    while (*key) 
	{
        hash = (hash << 4) + *key;          // 1 - Ӱ�����λ������һ��
        bites_mask = hash & 0xf0000000;     // 2 - �ø���λ��Ϊ�������ڶ�������
        if (bites_mask)
        {
			hash ^= bites_mask >> 24;       // 3 - Ӱ��4-5λ������һ��    
			hash &= ~bites_mask;            // 4 - ��ո���λ
		}
		key++;                              // 5 - ������һ���ַ���������
    }
    return hash % MAXKEY;                   // 6 
}

/************************************************************************/
/*  ����:            ����COFF����                                       */
/*  symtab:          ����COFF���ű�Ľ�                                 */
/*  name:            ��������                                           */
/*  val:             �������ص�ֵ                                     */
/*  sec_index:       ����˷��ŵĽ�                                     */
/*  type:            Coff��������                                       */
/*  StorageClass:    Coff���Ŵ洢���                                   */
/*  ����ֵ:          ����COFF���ű������                               */
/************************************************************************/
int coffsym_add(Section * symtab, char * name, int val, int sec_index,
		WORD typeFunc, char StorageClass)
{
	CoffSym * cfsym;
	int cs, keyno;
	char * csname;
	Section * strtab = symtab->link;
	int * hashtab;
	hashtab = symtab->hashtab;
	// �����Ƿ���ڡ�
	cs = coffsym_search(symtab, name);
	// ��������ڣ�����ӡ�
	if(cs == 0)  //
	{
		cfsym = (CoffSym *)section_ptr_add(symtab, sizeof(CoffSym));
		// ����COFF�������ַ�����
		csname = coffstr_add(strtab, name);
		cfsym->name           = csname - strtab->data;   // ????
		// ���ں�����ʾ��ǰָ���ڴ����λ�á�
		cfsym->coff_sym_value = val;
		// �ڱ������
		cfsym->shortSection   = sec_index;
		// ��ʾ���͵����֡�0x20 - CST_FUNC
		cfsym->typeFunc           = typeFunc;
		// �洢���ͨ����˵����IMAGE_SYM_CLASS_EXTERNAL��Ҳ����
		cfsym->storageClass   = StorageClass;
		// ���ɹ�ϣֵ��
		keyno = elf_hash(name);
		// 
		cfsym->next = hashtab[keyno];

		cs = cfsym - (CoffSym *)symtab->data;
		hashtab[keyno] = cs;
	}
	return cs;
}

/************************************************************************/
/*  ����:         ���ӻ����COFF���ţ�                                  */
/*                ����ֻ�����ں�����������������                    */
/*  S:            ����ָ��                                              */
/*  val:          ����ֵ                                                */
/*  sec_index:    ����˷��ŵĽ�                                        */
/*  typeFunc:     COFF��������                                          */
/*  StorageClass: COFF���Ŵ洢���                                      */
/************************************************************************/
void coffsym_add_update(Symbol *sym, int val, int sec_index,
		WORD typeFunc, char StorageClass)
{
	char * name;
	CoffSym *cfsym;
	if(!sym->related_value)
	{
		name = ((TkWord)tktable[sym->token_code]).spelling;
		sym->related_value = coffsym_add(sec_symtab, 
					name, val, sec_index, typeFunc, StorageClass);
	}
	else
	{
		cfsym = &((CoffSym *)sec_symtab->data)[sym->related_value];
		cfsym->coff_sym_value = val;
		cfsym->shortSection = sec_index;
	}
}

/************************************************************************/
/*  ����:            ����COFF�������ַ���                               */
/*  strtab:          ����COFF�ַ�����Ľ�                               */
/*  name:            ���������ַ���                                     */
/*  ����ֵ:          ����COFF�ַ���                                     */
/************************************************************************/
char * coffstr_add(Section * strtab, char * name)
{
	int len;
	char * pstr;
	len = strlen(name);
	pstr = (char *)section_ptr_add(strtab, len + 1);
	memcpy(pstr, name, len);
	return pstr;
}

/************************************************************************/
/* ASCII��0x2E��.�š�0x64��d��                                          */
/* 00 2E 64 61 74 61 00 2E 62 73 73 00 2E 72 64 61 74 61                */
/*     .  d  a  t  a     .  b  s  s     .  r  d  a  t  a                */
/* 00 6D 61 69 6E 00 70 72 69 6E 74 66 00 5F 65 6E 74 72                */
/*     m  a  i  n     p  r  i  n  t  f     _  e  n  t  r                */
/* 79 00 65 78 69 74 00                                                 */
/*  y     e  x  i  t                                                    */
/* �ַ�����00��β���������8���ַ�����Ŀ����һ������00��β�Ŀ��ַ�����  */
/************************************************************************/

/************************************************************************/
/*  ����:            ����COFF����                                       */
/*  symtab:          ����COFF���ű�Ľ�                                 */
/*  name:            ��������                                           */
/*  ����ֵ:          ����COFF���ű������                               */
/************************************************************************/
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

/************************************************************************/
/*  ����:            �����ض�λ��Ŀ                                     */
/*  section:         �������ڽ�                                         */
/*  sym:             ����ָ��                                           */
/*  offset:          ��Ҫ�����ض�λ�Ĵ��������������Ӧ�ڵ�ƫ��λ��     */
/*  type:            �ض�λ����                                         */
/************************************************************************/
void coffreloc_add(Section * sec, Symbol * sym, int offset, char type)
{
	int cfsym;
	char * name;
	// ������ű�û�ж�Ӧ�Ĺ���ֵ����Ϊ����ӡ�
	if(!sym->related_value)
		coffsym_add_update(sym, 0, 
			IMAGE_SYM_UNDEFINED, CST_FUNC ,
			IMAGE_SYM_CLASS_EXTERNAL);
	// ���ݷ��ű����õ����ַ���.
	name = ((TkWord)tktable[sym->token_code]).spelling;
	// ���ݵ����ַ�������COFF���š�
	cfsym = coffsym_search(sec_symtab, name);
	coffreloc_redirect_add(offset, cfsym, sec->index, type);
}

/************************************************************************/
/* ����:            ����COFF�ض�λ��Ϣ                                  */
/* offset:          ��Ҫ�����ض�λ�Ĵ��������������Ӧ�ڵ�ƫ��λ��      */
/* cf sym:          ���ű������                                        */
/* section:         �������ڽ�                                          */
/* type:            �ض�λ����                                          */
/************************************************************************/
void coffreloc_redirect_add(int offset, int cfsym, char section, char type)
{
	CoffReloc * rel;
	rel = (CoffReloc *)section_ptr_add(sec_rel, sizeof(CoffReloc));
	rel->offset  = offset;
	rel->cfsym   = cfsym;
	rel->section = section;
	rel->type    = type;
}

/************************************************************************/
/* ���ܣ�    ��¼������ת��ַ��ָ����                                   */
/* jmp_addr��ǰһ��תָ���ַ                                           */
/*           0x00 - ��������תָ���ַ��                              */
/*		     0xXX - break��תλ�û���continue��תλ�á�                 */
/************************************************************************/
int make_jmpaddr_list(int jmp_addr)
{
	int indNew;
	// ����ָ���ڴ����λ�ü�4��������һ��ָ���λ�á�
	indNew = sec_text_opcode_offset + 4;
	// ������������ʹ���´�С���·������ڡ�
	if (indNew > sec_text->data_allocated)
	{
		section_realloc(sec_text, indNew);
	}
	// ��ǰһ��תָ���ַд������λ�á�
	// ���ֵ��if��for��ʱ��Ϊ�㡣����break, continue, return��ʱ����㡣
	*(int *)(sec_text->data + sec_text_opcode_offset) = jmp_addr;
	// ����ֵΪ�����λ�á���仰��ʵûɶ���á�
	jmp_addr = sec_text_opcode_offset;
	// ���´����λ�á�ָ����һ��ָ�
	sec_text_opcode_offset = indNew;
	// ����ֵΪ�����λ�á�
	return jmp_addr;
}

/************************************************************************/
/* ���ܣ�       ���������tΪ���׵ĸ���������ת��ַ������Ե�ַ       */
/* fill_offset�����ס�����Ϊ����Ҫ���Ĵ����ƫ��λ��                  */
/* jmp_addr��   ָ����תλ�á�                                          */
/************************************************************************/
void jmpaddr_backstuff(int fill_offset, int jmp_addr)
{
	int next_addr, *ptrSecText;
	// �����Ҫ���Ĵ����ƫ��λ�ò�Ϊ�㡣
	while (fill_offset)
	{
		// ptrָ������ƫ��λ��
		ptrSecText = (int *)(sec_text->data + fill_offset);
		// ��ס��һ����Ҫ����λ��
		next_addr = *ptrSecText;
		// ������Ե�ַ
		*ptrSecText = jmp_addr - fill_offset - 4;
		// ׼��������һ����Ҫ����λ��
		fill_offset = next_addr ;
	}
}

/************************************************************************/
/* *����:            COFF��ʼ��                                         */
/* *�������õ�ȫ�ֱ���:                                                 */
/* DynArray sections��        //������                                  */
/* Section*sec_text��         //�����                                  */
/*         *sec_data��        //���ݽ�                                  */
/*         *sec_bss��         //δ��ʼ�����ݽ�                          */
/*         *sec_idata��       //���˱��                                */
/*         *sec_rdata��       //ֻ�����ݽ�                              */
/*         *sec_rel��         //�ض�λ��Ϣ��                            */
/*         *sec_symtab��      //���ű��                                */
/*         *sec_dynsym tab��  //���ӿ���Ž�                            */
/* in tn sec_image��          //ӳ���ļ��ڸ���                          */
/************************************************************************/
void init_coff()
{
	vecSection.reserve(1024);
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

	coffsym_add(sec_symtab,    "",       0,                0, CST_NOTFUNC, IMAGE_SYM_CLASS_NULL);
	coffsym_add(sec_symtab,    ".data",  0,  sec_data->index, CST_NOTFUNC, IMAGE_SYM_CLASS_STATIC);
	coffsym_add(sec_symtab,    ".bss",   0,   sec_bss->index, CST_NOTFUNC, IMAGE_SYM_CLASS_STATIC);
	coffsym_add(sec_symtab,    ".rdata", 0, sec_rdata->index, CST_NOTFUNC, IMAGE_SYM_CLASS_STATIC);
	coffsym_add(sec_dynsymtab, "",       0,                0, CST_NOTFUNC, IMAGE_SYM_CLASS_NULL);
}

/************************************************************************/
/*  ����:            �ͷ����н�����                                     */
/************************************************************************/
void free_sections()
{
	int idx;
	Section * sec;
	for(idx = 0; idx < vecSection.size(); idx++)
	{
		sec = (Section *)vecSection[idx];
		if(sec->hashtab != NULL)
			free(sec->hashtab);
		free(sec->data);
		free(sec);
	}
}

void print_all_section()
{
	int idx;
	Section * sec;
	printf("We have ");
	for(idx = 0; idx < vecSection.size(); idx++)
	{
		sec = (Section *)vecSection[idx];
		printf(" <%s>", sec->sh.Name);
	}
	printf(". \r\n");
}


/************************************************************************/
/* ����:          �ӵ�ǰ��дλ�õ�new_posλ����0��ļ�����            */
/* fp:            �ļ�ָ��                                              */
/* new_pos:       ��յ�λ��                                          */
/************************************************************************/
void fpad(FILE * fp, int new_pos)
{
	int cur_pos = ftell(fp);
	while (++cur_pos <= new_pos)
	{
		fputc(0, fp);
	}
}

/************************************************************************/
/* ����:            ����Ŀ���ļ�                                        */
/* file_name:       Ŀ���ļ���                                          */
/* ��������Ϊ-lmsvcrt -o HelloWorld.exe HelloWorld.obj����������������*/
/************************************************************************/
int load_objfile(char * file_name)
{
	IMAGE_FILE_HEADER file_header;
	// Section ** secs;
	int i, section_header_size, nsec_obj = 0, nSym;

	FILE * file_obj ;
	CoffSym * cfsyms ;
	CoffSym * cfsym ;

	char * strs, * cs_name;
	void * ptr ;
	int cur_text_offset ;
	int cur_rel_offset ;
	int * old_to_new_syms;
	int cfsym_index;

	CoffReloc * rel, *rel_end;
	// �����ͷ��С
	section_header_size = sizeof(IMAGE_SECTION_HEADER);
	memset(&file_header, 0x00, sizeof(IMAGE_FILE_HEADER));
	// ���ļ���
	file_obj = fopen(file_name, "rb");
	// ��COFF�ļ�ͷ��
	fread(&file_header, 1, sizeof(IMAGE_FILE_HEADER), file_obj);
	// �õ���ͷ������
	nsec_obj = file_header.NumberOfSections;
	// ������ÿһ���ڡ�������һ�����ɣ�����Name��sh�ĵ�һ����Ա��
	// Name�ĵ�ַ��vecSection[i]->sh�ĵ�ַ��ȡ�Ҳ���ǣ�
	//     &(vecSection[i]->sh) == vecSection[i]->sh.Name
	for (i = 0; i < nsec_obj; i++)
	{
	//	IMAGE_SECTION_HEADER * testOne = &(vecSection[i]->sh);
	//	BYTE * testTwo = vecSection[i]->sh.Name;
		fread(vecSection[i]->sh.Name, 1, section_header_size, file_obj);
	}
	// ��ȡ�����ƫ��
	cur_text_offset = sec_text->data_offset;
	// ��ȡ�ض�λ��Ϣ��ƫ��
	cur_rel_offset  = sec_rel->data_offset;
	for (i = 0; i < nsec_obj; i++)
	{
		// COFF���ű�
		if(!strcmp((char *)vecSection[i]->sh.Name, ".symtab"))
		{
			// ����һ��COFF����
			cfsyms = (CoffSym *)mallocz(vecSection[i]->sh.SizeOfRawData);
			// �������COFF����
			fseek(file_obj, 1, vecSection[i]->sh.PointerToRawData);
			fread(cfsyms, 1, vecSection[i]->sh.SizeOfRawData, file_obj);
			// �õ�COFF���Ÿ�����
			nSym = vecSection[i]->sh.SizeOfRawData / sizeof(CoffSym);
			continue;
		}
		// COFF�ַ�����
		else if(!strcmp((char *)vecSection[i]->sh.Name, ".strtab"))
		{
			// ����һ���ַ���
			strs = (char *)mallocz(vecSection[i]->sh.SizeOfRawData);
			// �������е��ַ����������ǣ�
			//     ..data..bss..rdata.main._entry.
			fseek(file_obj, 1, vecSection[i]->sh.PointerToRawData);
			fread(strs, 1, vecSection[i]->sh.SizeOfRawData, file_obj);
			continue;
		}
		else if(!strcmp((char *)vecSection[i]->sh.Name, ".dynsym") ||
			    !strcmp((char *)vecSection[i]->sh.Name, ".dynstr"))
		{
			continue;
		}
		fseek(file_obj, SEEK_SET, vecSection[i]->sh.PointerToRawData);
		ptr = section_ptr_add(vecSection[i], vecSection[i]->sh.SizeOfRawData);
		fread(ptr, 1, vecSection[i]->sh.SizeOfRawData, file_obj);
	}
	// ʹ��COFF���Ÿ�������ռ䡣
	old_to_new_syms = (int *)mallocz(sizeof(int) * nSym);
	// ���Ŵ�һ��ʼ��
	for (i = 1; i < nSym; i++)
	{
		cfsym = &cfsyms[i];
		cs_name = strs + cfsym->name;
		cfsym_index = coffsym_add(sec_symtab, cs_name,
			cfsym->coff_sym_value, cfsym->shortSection, 
			cfsym->typeFunc,       cfsym->storageClass);
		old_to_new_syms[i] = cfsym_index;
	}
	// �ض�λ��Ϣ��
	rel     = (CoffReloc *)(sec_rel->data +cur_rel_offset);
	rel_end = (CoffReloc *)(sec_rel->data + sec_rel->data_offset);
	for (; rel < rel_end; rel++)
	{
		cfsym_index  = rel->cfsym;
		rel->cfsym   = old_to_new_syms[cfsym_index];
		rel->offset += cur_rel_offset;
	}
	free(cfsyms);
	free(strs);
	free(old_to_new_syms);
	fclose(file_obj);
	return 1;
}

/************************************************************************/
/* ����:            ���Ŀ���ļ�                                        */
/* file_name:       Ŀ���ļ���                                          */
/* ��������Ϊ-o HelloWorld.obj -c HelloWorld.c����������������        */
/************************************************************************/
void output_objfile(char * file_name)
{
	int idx, sh_size, nsec_obj = 0;
	IMAGE_FILE_HEADER * coff_file_header;
	int file_offset;
	// ���ļ���
	FILE * coff_obj_file = fopen(file_name, "wb");
	// We have  <.text> <.data> <.idata> <.rdata> <.bss> <.rel> 
	//    <.symtab> <.strtab> <.dynsym> <.dynstr>.
	// �����ͷ�����������ȥ<.dynsym> <.dynstr>�����ڡ�
	// ��Ϊ��ӳ���ļ��в�����COFF�ض�λ��Ϣ��
	nsec_obj = vecSection.size() -2 ;
	sh_size  = sizeof(IMAGE_SECTION_HEADER);
	// �����ļ���С��ΪCOFF�ļ�ͷ + ��ͷ��С * ��ͷ����
	file_offset = sizeof(IMAGE_FILE_HEADER) + nsec_obj * sh_size;
	// ��ļ����ݡ���COFF�ļ�ͷ�ͽ�ͷ��Ԥ���ռ䡣
	fpad(coff_obj_file, file_offset);
	coff_file_header = (IMAGE_FILE_HEADER *)mallocz(sizeof(IMAGE_FILE_HEADER));
	// Write File Sections
	// ���濪ʼд�������
	for(idx = 0; idx < nsec_obj; idx++)
	{
		Section * sec = vecSection[idx];
		// ���û�����ݣ��Ͳ�����
		if(sec->data == NULL)
			continue;
		// д��һ���ڵ�����
		fwrite(sec->data, 1, sec->data_offset, coff_obj_file);
		// ����ָ��COFF�ļ��нڵĵ�һ��ҳ����ļ�ָ�롣
		sec->sh.PointerToRawData = file_offset;
		// ��������ļ����ѳ�ʼ�����ݵĴ�С��
		sec->sh.SizeOfRawData = sec->data_offset;
		file_offset += sec->data_offset;
	}
	// �ļ�ָ��ص���ͷ��׼��дCOFF�ļ�ͷ�ͽ�ͷ��
	fseek(coff_obj_file, SEEK_SET, 0);
	// ��������ΪI386
	coff_file_header->Machine = IMAGE_FILE_MACHINE_I386;
	// �ڵ���Ŀ��
	coff_file_header->NumberOfSections = nsec_obj;
	// ���ű���ļ�ƫ��ָ��sec_symtab�ڵĵ�һ��ҳ�档
	coff_file_header->PointerToSymbolTable = sec_symtab->sh.PointerToRawData;
	// ���ű��е�Ԫ����Ŀ
	coff_file_header->NumberOfSymbols = sec_symtab->sh.SizeOfRawData / sizeof(CoffSym);
	// дCOFF�ļ�ͷ
	fwrite(coff_file_header, 1, sizeof(IMAGE_FILE_HEADER), coff_obj_file);
	// д��ͷ��
	for (idx =0; idx < nsec_obj; idx++)
	{
		Section * sec = vecSection[idx];
		fwrite(sec->sh.Name, 1, sh_size, coff_obj_file);
	}
	free(coff_file_header);
	fclose(coff_obj_file);
}

// int main(int argc, char* argv[])
// {
//	init_coff();
//	output_objfile("write_obj.obj"); 
//	free_sections();
//	return 0;
//}

