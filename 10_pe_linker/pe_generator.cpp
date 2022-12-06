#include "pe_generator.h"

#include "get_token.h"
#include "translation_unit.h"

extern std::vector<Section> vecSection;

extern Section *sec_text,			// �����
				*sec_data,			// ���ݽ�
				*sec_bss,			// δ��ʼ�����ݽ�
				*sec_idata,			// ������
				*sec_rdata,			// ֻ�����ݽ�
				*sec_rel,			// �ض�λ��Ϣ��
				*sec_symtab,		// ���ű��	
				*sec_dynsymtab;		// ���ӿ���Ž�

char *lib_path;

// MS-DOSͷ
IMAGE_DOS_HEADER g_dos_header = {
	/* IMAGE_DOS_HEADER */
	0x5A4D,			/* WORD e_magic      DOS��ִ���ļ����, Ϊ'Mz'  */
	0x0090,			/* WORD e_cblp       Bytes on last page of file */
	0x0003,			/* WORD e_cp         Pages in file */
	0x0000,			/* WORD e_crlc       Relocations */
				
	0x0004,			/* WORD e_cparhdr    Sizeof header in paragraphs */
	0x0000,			/* WORD e_minalloc   Minimum extra paragraphs needed */
	0xFFFF,			/* WORD e_maxalloc   Maximum extra paragraphs needed */
	0x0000,			/* WORD e_ss         DOS����ĳ�ʼ����ջ�� */
				
	0x00B8,			/* WORD e_sp         DOS����ĳ�ʼ����ջָ�� */ 
	0x0000, 		/* WORD e_csum       Checksum */
	0x0000, 		/* WORD e_ip         DOS��������IP */
	0x0000,			/* WORD e_cs         DOS��������cs */
		
	0x0040, 		/* WORD e_lfarlc     File address of relocation table */
	0x0000, 		/* WORD e_ovno       Overlay number */
	{0, 0, 0, 0},	/* WORD e_res[4]     Reserved words */
	0x0000, 		/* WORD e_oemid      OEM identifier(for e oem info) */
		
	0x0000, 		/* WORD e_oeminfo    OEM information; e_oem id specific */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
					/* WORD e_res2[10]   Reserved words */   
	0x00000080		/* WORD e_lfanew     ָ��PE�ļ�ͷ */
};

// MS-DOSռλ����
BYTE dos_stub[0x40] = {
    /* BYTE dosstub[0x40] */
    /* 14 code bytes + "This program cannot be run in DOS mode.\r\r\n$" + 6 * 0x00 */
    0x0e,0x1f,0xba,0x0e,0x00,0xb4,0x09,0xcd,0x21,0xb8,0x01,0x4c,0xcd,0x21,0x54,0x68,
    0x69,0x73,0x20,0x70,0x72,0x6f,0x67,0x72,0x61,0x6d,0x20,0x63,0x61,0x6e,0x6e,0x6f,
    0x74,0x20,0x62,0x65,0x20,0x72,0x75,0x6e,0x20,0x69,0x6e,0x20,0x44,0x4f,0x53,0x20,
    0x6d,0x6f,0x64,0x65,0x2e,0x0d,0x0d,0x0a,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

/* NTͷ(�ֳ�PEͷ) ����3�������ݣ�PE�ļ�ǩ����COFF�ļ�ͷ��COFF��ѡ�ļ�ͷ�� */ 
IMAGE_NT_HEADERS32 nt_header = {
	0x00004550,		/* DWORD Signature; PE�ļ�ǩ�� */
	{
		/* IMAGE_FILE_HEADER COFF�ļ�ͷ */
		0x014c,       /* WORD FileHeader.Machine                ����ƽ̨ */
		0x0003,       /* WORD FileHeader.NumberOfSections       �ļ��Ľ���Ŀ */
		0x00000000,   /* WORD FileHeader.TimeDateStamp          �ļ��Ĵ������ں�ʱ�� */
		0x00000000,   /* WORD FileHeader.PointerToSymbolTable   ָ����ű�(���ڵ���) */
		0x00000000,   /* WORD FileHeader.NumberOfSymbols        ���ű��еķ�������(���ڵ���) */
		0x00E0,       /* WORD FileHeader.SizeOfOptionalHeader   COFF��ѡ�ļ�ͷ���� */
		0x030F        /* WORD FileHeader.Characteristics        �ļ����� */
	},
	{
		/* IMAGE_OPTIONAL_HEADER32 - COFF��ѡ�ļ�ͷ - ���������֣� */
		/* 1. ��ѡ�ļ�ͷ�еı�׼�� */
		0x010B,       /* WORD OptionalHeader.Magic                     ӳ���ļ���״̬��
		                                                               0x10B����������һ�������Ŀ�ִ���ļ���*/
		0x06,         /* BYTE OptionalHeader.MajorLinkerVersion        ���������汾�� */
		0x00,         /* BYTE OptionalHeader.MinorLinkerVersion        �������ΰ汾�� */
		0x00000000,   /* DWORD OptionalHeader.SizeOfCode               ���к�����ε��ܴ�С */
		0x00000000,   /* DWORD OptionalHeader.SizeOfInitializedData    �ѳ�ʼ�����ݶε��ܴ�С */
		0x00000000,   /* DWORD OptionalHeader.SizeOfUninitializedData  δ��ʼ�����ݶεĴ�С */
		0x00000000,   /* DWORD OptionalHeader.AddressOfEntryPoint      ����ִ���˿ڵ�������ַRVA */
		0x00000000,   /* DWORD OptionalHeader.BaseOfCode               ����ε���ʼ������ַRVA */
		0x00000000,   /* DWORD OptionalHeader.BaseOfData               ���ݶε���ʼ������ַRVA */

		/* 2. ��ѡ�ļ�ͷ�е�Windows�ض��� */
		0x00400000,   /* DWORD OptionalHeader.ImageBase                    ����Ľ���װ�ص�ַ */ 
		0x00001000,   /* DWORD OptionalHeader.SectionAlignment             �ڴ��жεĶ������� */
		0x00000200,   /* DWORD OptionalHeader.FileAlignment                �ļ��жεĶ������� */
		0x0004,       /* WORD  OptionalHeader.MajorOperatingSystemVersion  ����ϵͳ�����汾�� */
		0x0000,       /* WORD  OptionalHeader.MinorOperatingSystemVersion  ����ϵͳ�Ĵΰ汾�� */
		0x0000,       /* WORD  OptionalHeader.MajorImageVersion            ��������汾�� */
		0x0000,       /* WORD  OptionalHeader.MinorImageVersion            ����Ĵΰ汾�� */
		0x0004,       /* WORD  OptionalHeader.MajorSubsystemVersion        ��ϵͳ�����汾�� */
		0x0000,       /* WORD  OptionalHeader.MinorSubsystemVersion        ��ϵͳ�Ĵΰ汾�� */
		0x00000000,   /* DWORD OptionalHeader.Win32VersionValue            ������ ��Ϊ0 */
		0x00000000,   /* DWORD OptionalHeader.SizeOfImage                  �ڴ�������PEӳ��ߴ� */
		0x00000200,   /* DWORD OptionalHeader.SizeOfHeaders                ����ͷ+�ڱ�Ĵ�С */
		0x00000000,   /* DWORD OptionalHeader.CheckSum                     У��� */
		0x0003,       /* WORD  OptionalHeader.Subsystem                    �ļ�����ϵͳ */
		0x0000,       /* WORD  OptionalHeader.DllCharacteristics           DLL���� */
		0x00100000,   /* DWORD OptionalHeader.SizeOfStackReserve           ��ʼ��ʱ��ջ��С */
		0x00001000,   /* DWORD OptionalHeader.SizeOfStackCommit            ��ʼ��ʱʵ���ύ�Ķ�ջ��С */
		0x00100000,   /* DWORD OptionalHeader.SizeOfHeapReserve            ��ʼ��ʱ�����ĶѴ�С */
		0x00001000,   /* DWORD OptionalHeader.SizeOfHeapCommit             ��ʼ��ʱʵ���ύ�ĶѴ�С */
		0x00000000,   /* DWORD OptionalHeader.LoaderFlags                  ������ ��Ϊ0 */
		0x00000010,   /* DWORD OptionalHeader.NumberOfRvaAndSizes          ���������Ŀ¼�ṹ������ */
		/* 3. ��ѡ�ļ�ͷ�е�����Ŀ¼ - IMAGE_DATA_DIRECTORY */ 
		{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
		 {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}
	}                   
}; 

int load_obj_file(char * file_name)
{
	IMAGE_FILE_HEADER file_header;
	Section ** secs;
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

	section_header_size = sizeof(IMAGE_SECTION_HEADER);
	file_obj = fopen(file_name, "rb");
	fread(&file_header, 1, sizeof(IMAGE_FILE_HEADER), file_obj);
	nsec_obj = file_header.NumberOfSections;
	// ?????
	// secs = (Section **)vecSection.data;
	for (i = 0; i < nsec_obj; i++)
	{
		fread(vecSection[i].sh.Name, 1, section_header_size, file_obj);
	}
	// ��ȡ�����ƫ��
	cur_text_offset = sec_text->data_offset;
	// ��ȡ�ض�λ��Ϣ��ƫ��
	cur_rel_offset  = sec_rel->data_offset;
	for (i = 0; i < nsec_obj; i++)
	{
		if(!strcmp((char *)vecSection[i].sh.Name, ".symtab"))
		{
			cfsym = (CoffSym *)mallocz(vecSection[i].sh.SizeOfRawData);
			fseek(file_obj, 1, vecSection[i].sh.PointerToRawData);
			fread(cfsym, 1, vecSection[i].sh.SizeOfRawData, file_obj);
			nSym = vecSection[i].sh.SizeOfRawData / sizeof(CoffSym);
			continue;
		}
		else if(!strcmp((char *)vecSection[i].sh.Name, ".strtab"))
		{
			strs = (char *)mallocz(vecSection[i].sh.SizeOfRawData);
			fseek(file_obj, 1, vecSection[i].sh.PointerToRawData);
			fread(strs, 1, vecSection[i].sh.SizeOfRawData, file_obj);
			continue;
		}
		else if(!strcmp((char *)vecSection[i].sh.Name, ".dynsym") ||
			    !strcmp((char *)vecSection[i].sh.Name, ".dynstr"))
		{
			continue;
		}
		fseek(file_obj, SEEK_SET, vecSection[i].sh.PointerToRawData);
		ptr = section_ptr_add(&vecSection[i], vecSection[i].sh.SizeOfRawData);
		fread(ptr, 1, vecSection[i].sh.SizeOfRawData, file_obj);
	}
	old_to_new_syms = (int *)mallocz(sizeof(int) * nSym);
	for (i = 1; i < nSym; i++)
	{
		cfsym = &cfsyms[i];
		cs_name = strs + cfsym->name;
		cfsym_index = coffsym_add(sec_symtab, cs_name,
			cfsym->coff_sym_value, cfsym->shortSection, 
			cfsym->type,           cfsym->storageClass);
		old_to_new_syms[i] = cfsym_index;
	}
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

char * get_dllname(char * lib_file);
int pe_load_lib_file(char * name)
{
	char lib_file[MAX_PATH];
	int ret = -1;
	char line[512], *dll_name, *p;
	FILE * fp;
	sprintf(lib_file, "%s%s.slib", lib_path, name);
	fp = fopen(lib_file, "rb");
	if (fp)
	{
		dll_name = get_dllname(lib_file);
		// 
	}
	return 1;
}

char * get_dllname(char * lib_file)
{
	int n1, n2;
	char * lib_name, *dll_name, *p;
	n1 = strlen(lib_file);
	lib_name = lib_file;
	return dll_name;
}


/*********************************************************** 
 * ����:	�õ����þ�̬���Ŀ¼
 **********************************************************/
char *get_lib_path()
{
    /* ���Ǽٶ�������Ҫ���ӵľ�̬������������ͬ��Ŀ¼��lib�ļ�����
	   �˴���Ҫ��һ�¾�̬��
	*/
    char path[MAX_PATH];
	char *p;
    GetModuleFileNameA(NULL, path, sizeof(path));
	p = strrchr(path,'\\');
	*p = '\0';
	strcat(path,"\\lib\\");
    return strdup(path);
}

