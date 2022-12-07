#include "pe_generator.h"

extern std::vector<Section *> vecSection;

extern Section *sec_text,			// �����
				*sec_data,			// ���ݽ�
				*sec_bss,			// δ��ʼ�����ݽ�
				*sec_idata,			// ������
				*sec_rdata,			// ֻ�����ݽ�
				*sec_rel,			// �ض�λ��Ϣ��
				*sec_symtab,		// ���ű��	
				*sec_dynsymtab;		// ���ӿ���Ž�

char *lib_path;
std::vector<std::string> vecDllName;
std::vector<std::string> vecLib;

extern int nsec_image;				// ӳ���ļ��ڸ���

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

	section_header_size = sizeof(IMAGE_SECTION_HEADER);
	file_obj = fopen(file_name, "rb");
	fread(&file_header, 1, sizeof(IMAGE_FILE_HEADER), file_obj);
	nsec_obj = file_header.NumberOfSections;
	// ?????
	// secs = (Section **)vecSection.data;
	for (i = 0; i < nsec_obj; i++)
	{
		fread(vecSection[i]->sh.Name, 1, section_header_size, file_obj);
	}
	// ��ȡ�����ƫ��
	cur_text_offset = sec_text->data_offset;
	// ��ȡ�ض�λ��Ϣ��ƫ��
	cur_rel_offset  = sec_rel->data_offset;
	for (i = 0; i < nsec_obj; i++)
	{
		if(!strcmp((char *)vecSection[i]->sh.Name, ".symtab"))
		{
			cfsyms = (CoffSym *)mallocz(vecSection[i]->sh.SizeOfRawData);
			fseek(file_obj, 1, vecSection[i]->sh.PointerToRawData);
			fread(cfsyms, 1, vecSection[i]->sh.SizeOfRawData, file_obj);
			nSym = vecSection[i]->sh.SizeOfRawData / sizeof(CoffSym);
			continue;
		}
		else if(!strcmp((char *)vecSection[i]->sh.Name, ".strtab"))
		{
			strs = (char *)mallocz(vecSection[i]->sh.SizeOfRawData);
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

char *get_line(char *line, int size, FILE *fp);
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
		// dll_name = get_dllname(lib_file);
		dll_name = strrchr(lib_file, '\\');
		vecDllName.push_back(std::string(dll_name));
		for (;;)
		{
			p = get_line(line, sizeof(line), fp);
			if (NULL == p)
			{
				break;
			}
			if (0 == *p || ';' == *p)
			{
				continue;
			}
			coffsym_add(sec_dynsymtab, p, vecDllName.size(),
				sec_text->index, CST_FUNC, IMAGE_SYM_CLASS_EXTERNAL);
		}
		ret = 0;
		if (fp)
		{
			fclose(fp);
		}
	}
	else
	{
		print_error("File open failed", name);
	}
	return ret;
}

/***********************************************************
 * ����:	�Ӿ�̬���ļ��ж�ȡһ��,���������Ч�ַ�
 * line:	���ݴ洢λ��
 * size:	��ȡ������ַ���
 * fp:		�Ѵ򿪵ľ�̬���ļ�ָ��
 **********************************************************/	
char *get_line(char *line, int size, FILE *fp)
{
	char *p,*p1;
    if (NULL == fgets(line, size, fp))
        return NULL;

	//ȥ����ո�
	p = line;
	while (*p && isspace(*p))
        p++;
    
	//ȥ���ҿո񼰻س���
	p1 = strchr(p,'\0');
	while(p1 > p && p1[-1] <= ' ' ) 
		p1--;
	*p1 = '\0';

    return p;
}

void add_runtime_libs()
{
	int i;
	int pe_type = 0;
	for (i = 0; i < vecLib.size(); i++)
	{
		pe_load_lib_file((char *)vecLib[i].c_str());
	}
}

int pe_find_import(char * strSymbol);
struct ImportSym * pe_add_import(struct PEInfo * pe,
								 int sym_index, char * name);
int resolve_coffsyms(struct PEInfo * pe)
{
	CoffSym * sym;
	int sym_index, sym_end ;
	int ret = 0;
	int found = 1;

	sym_end = sec_symtab->data_offset / sizeof(CoffSym);
	for (sym_index = 1; sym_index < sym_end; sym_index++)
	{
		sym = (CoffSym *)sec_symtab->data + sym_index;
		if(sym->shortSection == IMAGE_SYM_UNDEFINED)
		{
			char * name = 
				sec_symtab->link->data + sym->name;
			unsigned short type = sym->type;
			int imp_sym_index = pe_find_import(name);
			struct ImportSym * import_sym;
			if (0 == imp_sym_index)
				found = 0;
			import_sym = pe_add_import(pe, imp_sym_index, name);
			if(!import_sym)
				found = 0;
			if (found && type == CST_FUNC)
			{
				int offset = import_sym->thk_offset;
				char buffer[128];
				offset = sec_text->data_offset;
				WORD * retTmp = (WORD *)section_ptr_add(sec_text, 6);
				*retTmp = 0x25FF;
				sprintf(buffer, "IAT.%s", name);
				import_sym->iat_index = coffsym_add(sec_symtab, 
					buffer, 0, sec_idata->index,
					CST_FUNC, IMAGE_SYM_CLASS_EXTERNAL);
				coffreloc_redirect_add(offset + 2,
					import_sym->iat_index, sec_text->index,
					IMAGE_REL_I386_DIR32);
				import_sym->thk_offset = offset;
			}
			else
			{
				print_error("Undefined", name);
				ret = 1;
			}
		}
	}
	return ret;
}

int pe_find_import(char * strSymbol)
{
	int sym_index ;
	sym_index = coffsym_search(sec_dynsymtab, strSymbol);
	return sym_index;
}

struct ImportSym * pe_add_import(struct PEInfo * pe,
								 int sym_index, char * name)
{
	// int i ;
	int dll_index;
	struct ImportInfo *p;
	struct ImportSym  *s;
	dll_index = ((CoffSym *)sec_dynsymtab->data + sym_index)->coff_sym_value;
	if (0 == dll_index)
		return NULL;
	if (pe->imps.size() > dll_index)
	{
		p = pe->imps[dll_index];
	}
	else
	{
		p = (struct ImportInfo *)mallocz(sizeof(*p));
		p->imp_syms.reserve(8);
		p->dll_index = dll_index;
		pe->imps.push_back(p);
	}
	if (p->imp_syms.size() > dll_index)
	{
		return (struct ImportSym *)&(p->imp_syms[dll_index]);
	}
	else
	{
		s = (struct ImportSym *)mallocz(sizeof(struct ImportSym) + strlen(name));
		strcpy((char *)(&s->imp_sym.Name), name);
		p->dll_index = dll_index;
		p->imp_syms.push_back(s);
		return s;
	}
}
	
DWORD pe_virtual_align(DWORD n);
void pe_build_imports(struct PEInfo * pe);
int pe_assign_addresses(struct PEInfo * pe)
{
	int i ;
	DWORD addr ;
	Section *sec, **ps;
	pe->thunk = sec_idata;
	pe->secs = (Section **)mallocz(nsec_image * sizeof(Section *));
	addr = nt_header.OptionalHeader.ImageBase+1;
	for (i = 0; i < nsec_image; ++i)
	{
		sec = vecSection[i];
		ps  = &pe->secs[pe->sec_size];
		*ps = sec;
		sec->sh.VirtualAddress = addr = pe_virtual_align(addr);

		if (sec == pe->thunk)
		{
			pe_build_imports(pe);
		}
		if (sec->data_offset)
		{
			addr += sec->data_offset;
			++pe->sec_size;
		}
	}
	return 0;
}


/***********************************************************
 * ����:	�����ڴ����λ��
 * n:		δ����ǰλ��
 **********************************************************/	
DWORD pe_virtual_align(DWORD n)
{ 
	DWORD SectionAlignment = nt_header.OptionalHeader.SectionAlignment; //�ڴ��жεĶ�������
	return calc_align(n,SectionAlignment);
} 

int put_import_str(Section * sec, char *sym);
void pe_build_imports(struct PEInfo * pe)
{
	int thk_ptr, ent_ptr, dll_ptr, sym_cnt, i;
	DWORD rva_base = pe->thunk->sh.VirtualAddress - 
		nt_header.OptionalHeader.ImageBase;
	int ndlls = pe->imps.size();

	for (sym_cnt = i = 0; i < ndlls; ++i)
	{
		sym_cnt += ((struct ImportInfo *)pe->imps[i])->imp_syms.size();
	}
	if (sym_cnt == 0)
	{
		return ;
	}
	pe->imp_offs = dll_ptr = pe->thunk->data_offset;
	pe->imp_size = (ndlls + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR);
	pe->iat_offs = dll_ptr + pe->imp_size;
	pe->iat_size = (sym_cnt + ndlls) * sizeof(DWORD);
	section_ptr_add(pe->thunk, pe->imp_size + 2 * pe->iat_size);
	
	thk_ptr = pe->iat_offs;
	ent_ptr = pe->iat_offs + pe->iat_size;
	for (i = 0 ; i < pe->imps.size(); ++i)
	{
		int k, n, v;
		struct ImportInfo * p = pe->imps[i];
		char * name = (char *)vecDllName[p->dll_index - 1].c_str();
		//
		v = put_import_str(pe->thunk, name);
		p->imphdr.FirstThunk         = thk_ptr + rva_base;
		p->imphdr.OriginalFirstThunk = ent_ptr + rva_base;
		p->imphdr.Name               = v       + rva_base;
		memcpy(pe->thunk->data + dll_ptr, 
			&p->imphdr, sizeof(IMAGE_IMPORT_DESCRIPTOR));

		for (k = 0, n = p->imp_syms.size(); k <= n; ++k)
		{
			if(k < n)
			{
				struct ImportSym * import_sym = (struct ImportSym *)
					p->imp_syms[k];
				DWORD iat_index = import_sym->iat_index;
				CoffSym * org_sym = (CoffSym *)sec_symtab->data + iat_index;

				org_sym->coff_sym_value = thk_ptr;
				org_sym->shortSection   = pe->thunk->index;
				v = pe->thunk->data_offset + rva_base;

				section_ptr_add(pe->thunk, sizeof(import_sym->imp_sym.Hint));
				put_import_str(pe->thunk, (char *)import_sym->imp_sym.Name);
			}
			else
			{
				v = 0 ;
			}
			*(DWORD *)(pe->thunk->data + thk_ptr) = 
				*(DWORD *)(pe->thunk->data + ent_ptr) = v;
			thk_ptr += sizeof(DWORD);
			ent_ptr += sizeof(DWORD);
		}
		dll_ptr += sizeof(IMAGE_IMPORT_DESCRIPTOR);
		for (std::vector<struct ImportSym *>::iterator 
			iterImportSym = p->imp_syms.begin(); 
			iterImportSym != p->imp_syms.end(); iterImportSym++)
		{
			free(*iterImportSym);
		}
		 p->imp_syms.clear();
	}
	for (std::vector<struct ImportInfo *>::iterator iter = pe->imps.begin(); 
		iter != pe->imps.end(); iter++)
	{
		free(*iter);
	}
	pe->imps.clear();
}

int put_import_str(Section * sec, char *sym)
{
	int offset, len;
	char * ptr;
	len =strlen(sym) + 1;
	offset = sec->data_offset ;
	ptr    = (char *)section_ptr_add(sec, len);
	memcpy(ptr, sym, len);
	return offset;
}

void relocate_syms()
{
	CoffSym * sym, * sym_end;
	Section * sec;
	sym_end = (CoffSym *)(sec_symtab->data + sec_symtab->data_offset);
	for (sym = (CoffSym *)sec_symtab->data + 1;
		sym < sym_end; sym++)
	{
		sec = (Section *)vecSection[sym->shortSection - 1];
		sym->coff_sym_value += sec->sh.VirtualAddress;
	}
}

void coff_relocs_fixup()
{
	Section * sec, * sr;
	CoffReloc * rel, rel_end, * qrel;
	CoffSym * sym;
	int type, sym_index;
	char * ptr;
	unsigned long val, addr;
	char * name;

	sr = sec_rel;
	rel_end = (CoffReloc *)(sr->data + sr->data_offset);
	
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

