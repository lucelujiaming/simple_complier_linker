#include "pe_generator.h"

char *entry_symbol = "_entry";
extern std::vector<Section *> vecSection;

extern Section *sec_text,			// 代码节
				*sec_data,			// 数据节
				*sec_bss,			// 未初始化数据节
				*sec_idata,			// 导入表节
				*sec_rdata,			// 只读数据节
				*sec_rel,			// 重定位信息节
				*sec_symtab,		// 符号表节	
				*sec_dynsymtab;		// 链接库符号节

char *lib_path;
std::vector<std::string> vecDllName;
std::vector<std::string> vecLib;

short g_subsystem;
int   g_output_type;				// 输出文件类型

extern int nsec_image;				// 映像文件节个数

// MS-DOS头
IMAGE_DOS_HEADER g_dos_header = {
	/* IMAGE_DOS_HEADER */
	0x5A4D,			/* WORD e_magic      DOS可执行文件标记, 为'Mz'  */
	0x0090,			/* WORD e_cblp       Bytes on last page of file */
	0x0003,			/* WORD e_cp         Pages in file */
	0x0000,			/* WORD e_crlc       Relocations */
				
	0x0004,			/* WORD e_cparhdr    Sizeof header in paragraphs */
	0x0000,			/* WORD e_minalloc   Minimum extra paragraphs needed */
	0xFFFF,			/* WORD e_maxalloc   Maximum extra paragraphs needed */
	0x0000,			/* WORD e_ss         DOS代码的初始化堆栈段 */
				
	0x00B8,			/* WORD e_sp         DOS代码的初始化堆栈指针 */ 
	0x0000, 		/* WORD e_csum       Checksum */
	0x0000, 		/* WORD e_ip         DOS代码的入口IP */
	0x0000,			/* WORD e_cs         DOS代码的入口cs */
		
	0x0040, 		/* WORD e_lfarlc     File address of relocation table */
	0x0000, 		/* WORD e_ovno       Overlay number */
	{0, 0, 0, 0},	/* WORD e_res[4]     Reserved words */
	0x0000, 		/* WORD e_oemid      OEM identifier(for e oem info) */
		
	0x0000, 		/* WORD e_oeminfo    OEM information; e_oem id specific */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
					/* WORD e_res2[10]   Reserved words */   
	0x00000080		/* WORD e_lfanew     指向PE文件头 */
};

// MS-DOS占位程序
BYTE dos_stub[0x40] = {
    /* BYTE dosstub[0x40] */
    /* 14 code bytes + "This program cannot be run in DOS mode.\r\r\n$" + 6 * 0x00 */
    0x0e,0x1f,0xba,0x0e,0x00,0xb4,0x09,0xcd,0x21,0xb8,0x01,0x4c,0xcd,0x21,0x54,0x68,
    0x69,0x73,0x20,0x70,0x72,0x6f,0x67,0x72,0x61,0x6d,0x20,0x63,0x61,0x6e,0x6e,0x6f,
    0x74,0x20,0x62,0x65,0x20,0x72,0x75,0x6e,0x20,0x69,0x6e,0x20,0x44,0x4f,0x53,0x20,
    0x6d,0x6f,0x64,0x65,0x2e,0x0d,0x0d,0x0a,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

/* NT头(又称PE头) 包括3部分内容：PE文件签名、COFF文件头、COFF可选文件头。 */ 
IMAGE_NT_HEADERS32 nt_header = {
	0x00004550,		/* DWORD Signature; PE文件签名 */
	{
		/* IMAGE_FILE_HEADER COFF文件头 */
		0x014c,       /* WORD FileHeader.Machine                运行平台 */
		0x0003,       /* WORD FileHeader.NumberOfSections       文件的节数目 */
		0x00000000,   /* WORD FileHeader.TimeDateStamp          文件的创建日期和时间 */
		0x00000000,   /* WORD FileHeader.PointerToSymbolTable   指向符号表(用于调试) */
		0x00000000,   /* WORD FileHeader.NumberOfSymbols        符号表中的符号数量(用于调试) */
		0x00E0,       /* WORD FileHeader.SizeOfOptionalHeader   COFF可选文件头长度 */
		0x030F        /* WORD FileHeader.Characteristics        文件属性 */
	},
	{
		/* IMAGE_OPTIONAL_HEADER32 - COFF可选文件头 - 包括三部分： */
		/* 1. 可选文件头中的标准域 */
		0x010B,       /* WORD OptionalHeader.Magic                     映像文件的状态。
		                                                               0x10B，表明这是一个正常的可执行文件。*/
		0x06,         /* BYTE OptionalHeader.MajorLinkerVersion        链接器主版本号 */
		0x00,         /* BYTE OptionalHeader.MinorLinkerVersion        链接器次版本号 */
		0x00000000,   /* DWORD OptionalHeader.SizeOfCode               所有含代码段的总大小 */
		0x00000000,   /* DWORD OptionalHeader.SizeOfInitializedData    已初始化数据段的总大小 */
		0x00000000,   /* DWORD OptionalHeader.SizeOfUninitializedData  未初始化数据段的大小 */
		0x00000000,   /* DWORD OptionalHeader.AddressOfEntryPoint      程序执行人口的相对虚地址RVA */
		0x00000000,   /* DWORD OptionalHeader.BaseOfCode               代码段的起始相对虚地址RVA */
		0x00000000,   /* DWORD OptionalHeader.BaseOfData               数据段的起始相对虚地址RVA */

		/* 2. 可选文件头中的Windows特定域 */
		0x00400000,   /* DWORD OptionalHeader.ImageBase                    程序的建议装载地址 */ 
		0x00001000,   /* DWORD OptionalHeader.SectionAlignment             内存中段的对齐粒度 */
		0x00000200,   /* DWORD OptionalHeader.FileAlignment                文件中段的对齐粒度 */
		0x0004,       /* WORD  OptionalHeader.MajorOperatingSystemVersion  操作系统的主版本号 */
		0x0000,       /* WORD  OptionalHeader.MinorOperatingSystemVersion  操作系统的次版本号 */
		0x0000,       /* WORD  OptionalHeader.MajorImageVersion            程序的主版本号 */
		0x0000,       /* WORD  OptionalHeader.MinorImageVersion            程序的次版本号 */
		0x0004,       /* WORD  OptionalHeader.MajorSubsystemVersion        子系统的主版本号 */
		0x0000,       /* WORD  OptionalHeader.MinorSubsystemVersion        子系统的次版本号 */
		0x00000000,   /* DWORD OptionalHeader.Win32VersionValue            保留， 设为0 */
		0x00000000,   /* DWORD OptionalHeader.SizeOfImage                  内存中整个PE映像尺寸 */
		0x00000200,   /* DWORD OptionalHeader.SizeOfHeaders                所有头+节表的大小 */
		0x00000000,   /* DWORD OptionalHeader.CheckSum                     校验和 */
		0x0003,       /* WORD  OptionalHeader.Subsystem                    文件的子系统 */
		0x0000,       /* WORD  OptionalHeader.DllCharacteristics           DLL特征 */
		0x00100000,   /* DWORD OptionalHeader.SizeOfStackReserve           初始化时堆栈大小 */
		0x00001000,   /* DWORD OptionalHeader.SizeOfStackCommit            初始化时实际提交的堆栈大小 */
		0x00100000,   /* DWORD OptionalHeader.SizeOfHeapReserve            初始化时保留的堆大小 */
		0x00001000,   /* DWORD OptionalHeader.SizeOfHeapCommit             初始化时实际提交的堆大小 */
		0x00000000,   /* DWORD OptionalHeader.LoaderFlags                  保留， 设为0 */
		0x00000010,   /* DWORD OptionalHeader.NumberOfRvaAndSizes          下面的数据目录结构的数量 */
		/* 3. 可选文件头中的数据目录 - IMAGE_DATA_DIRECTORY */ 
		{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
		 {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}
	}                   
}; 

char *get_line(char *line, int size, FILE *fp);
/***********************************************************
 * 功能:	加载静态库
 * name:    静态库文件名,不包括路径和后缀
 **********************************************************/
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
			// 取出静态库中的一行。
			p = get_line(line, sizeof(line), fp);
			if (NULL == p)
			{
				break;
			}
			if (0 == *p || ';' == *p)
			{
				continue;
			}
			int vecDllNameSize = vecDllName.size();
			// 增加COFF符号
			coffsym_add(sec_dynsymtab, p, vecDllNameSize,
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
 * 功能:	从静态库文件中读取一行,并处理掉无效字符
 * line:	数据存储位置
 * size:	读取的最大字符数
 * fp:		已打开的静态库文件指针
 **********************************************************/	
char *get_line(char *line, int size, FILE *fp)
{
	char *p,*p1;
    if (NULL == fgets(line, size, fp))
        return NULL;

	//去掉左空格
	p = line;
	while (*p && isspace(*p))
        p++;
    
	//去掉右空格及回车符
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
		char* vecLibLine = (char *)vecLib[i].c_str();
		pe_load_lib_file(vecLibLine);
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
			unsigned short typeFunc = sym->typeFunc;
			int imp_sym_index = pe_find_import(name);
			struct ImportSym * import_sym;
			if (0 == imp_sym_index)
				found = 0;
			import_sym = pe_add_import(pe, imp_sym_index, name);
			if(!import_sym)
				found = 0;
			if (found && typeFunc == CST_FUNC)
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
 * 功能:	计算内存对齐位置
 * n:		未对齐前位置
 **********************************************************/	
DWORD pe_virtual_align(DWORD n)
{ 
	DWORD SectionAlignment = nt_header.OptionalHeader.SectionAlignment; //内存中段的对齐粒度
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
	CoffReloc * rel, *rel_end, * qrel;
	CoffSym * sym;
	int type, sym_index;
	char * ptr;
	unsigned long val, addr;
	char * name;

	sr = sec_rel;
	rel_end = (CoffReloc *)(sr->data + sr->data_offset);
	qrel = (CoffReloc *)sr->data;
	for (rel = qrel; rel < rel_end; rel++)
	{
		sec = (Section *)vecSection[rel->section - 1];

		sym_index = rel->cfsym;
		sym = &((CoffSym *)sec_symtab->data)[sym_index];
		name = sec_symtab->link->data + sym->name;
		val  = sym->coff_sym_value;
		type = rel->type;
		addr = sec->sh.VirtualAddress + rel->offset;
		ptr = sec->data + rel->offset;
		switch(type) {
		case IMAGE_REL_I386_DIR32:
			*(int *)ptr += val;
			break;
		case IMAGE_REL_I386_REL32:
			*(int *)ptr += val - addr;
			break;
		default:
			print_error("Error type ", "");
			break;
		}
	}
}
DWORD pe_file_align(DWORD n);
void pe_set_datadir(int dir, DWORD addr, DWORD size);
int pe_write(struct PEInfo * pe)
{
	int i ;
	FILE * op;
	DWORD file_offset, r;
	int sizeofHeaders;

	op = fopen(pe->filename, "wb");
	if (NULL == op)
	{
		print_error(" generate failed", (char *)pe->filename);
		return 1;
	}

	sizeofHeaders = pe_file_align(
		sizeof(g_dos_header) + 
		sizeof(dos_stub) + 
		sizeof(nt_header) +
		pe->sec_size * sizeof(IMAGE_SECTION_HEADER));
	for (i = 0; i < pe->sec_size; ++i)
	{
		Section * sec = pe->secs[i];
		char * sh_name = (char *)sec->sh.Name;
		unsigned long addr = sec->sh.VirtualAddress - 
					nt_header.OptionalHeader.ImageBase;
		unsigned long size = sec->data_offset;
		IMAGE_SECTION_HEADER * psh = &sec->sh;

		if (!strcmp((char *)sec->sh.Name, ".text"))
		{
			nt_header.OptionalHeader.BaseOfCode = addr;
			nt_header.OptionalHeader.AddressOfEntryPoint
				= addr + pe->entry_addr;
		}
		else if (!strcmp((char *)sec->sh.Name, ".data"))
		{
			nt_header.OptionalHeader.BaseOfData = addr;
		}
		else if (!strcmp((char *)sec->sh.Name, ".ldata"))
		{
			if (pe->imp_size)
			{
				pe_set_datadir(IMAGE_DIRECTORY_ENTRY_IMPORT,
					pe->imp_offs + addr, pe->imp_size);
				pe_set_datadir(IMAGE_DIRECTORY_ENTRY_IAT,
					pe->iat_offs + addr, pe->iat_size);
			}
		}
		strcpy((char *)psh->Name, sh_name);
		psh->VirtualAddress   = addr;
		psh->Misc.VirtualSize = size;
		nt_header.OptionalHeader.SizeOfImage = 
			pe_virtual_align(size + addr);
		if (sec->data_offset)
		{
			psh->PointerToRawData = r = file_offset;
			if (!strcmp((char *)sec->sh.Name, ".bss"))
			{
				sec->sh.SizeOfRawData = 0;
				continue;
			}
			fwrite(sec->data,1 ,sec->data_offset, op);
			file_offset = pe_file_align(file_offset + sec->data_offset);
			psh->SizeOfRawData = file_offset = 1;
			fpad(op, file_offset);
		}
	}

	nt_header.FileHeader.NumberOfSections = pe->sec_size;
	nt_header.OptionalHeader.SizeOfHeaders = sizeofHeaders;

	nt_header.OptionalHeader.Subsystem = g_subsystem;

	fseek(op, SEEK_SET, 0);
	fwrite(&g_dos_header, 1, sizeof(g_dos_header), op);
	fwrite(&dos_stub, 1, sizeof(dos_stub), op);
	fwrite(&nt_header, 1, sizeof(nt_header), op);
	for (i = 0; i < pe->sec_size; ++i)
	{
		fwrite(&pe->secs[i]->sh, 1, sizeof(IMAGE_SECTION_HEADER), op);
	}
	fclose(op);
	return 0;
}
void get_entry_addr(struct PEInfo * pe);
int pe_output_file(char * file_name)
{
	int ret;
	struct PEInfo pe;

	memset(&pe, 0, sizeof(pe) - sizeof(std::vector<struct ImportInfo *>));
	pe.imps.reserve(8);
	pe.filename = file_name;
	// 添加运行库。
	add_runtime_libs();

	get_entry_addr(&pe);
	ret = resolve_coffsyms(&pe);
	if (0 == ret)
	{
		pe_assign_addresses(&pe);
		relocate_syms();
		coff_relocs_fixup();
		ret = pe_write(&pe);
		free(pe.secs);
	}
	return ret ;
}

void get_entry_addr(struct PEInfo * pe)
{
	unsigned long addr = 0;
	int cs;
	CoffSym * cfsym_entry;
	cs = coffsym_search(sec_symtab, entry_symbol);
	cfsym_entry = (CoffSym *)sec_symtab->data + cs;
	addr = cfsym_entry->coff_sym_value;
	pe->entry_addr = addr;
}

/***********************************************************
 * 功能:	计算文件对齐位置
 * n:		未对齐前位置
 **********************************************************/	
DWORD pe_file_align(DWORD n)
{
	DWORD FileAlignment = nt_header.OptionalHeader.FileAlignment; //文件中段的对齐粒度
	return calc_align(n,FileAlignment);
}

/***********************************************************
 * 功能:	设置数据目录
 * dir:		目录类型
 * addr:	表的RVA
 * size:	表的大小(以字节计)
 **********************************************************/	
void pe_set_datadir(int dir, DWORD addr, DWORD size)
{
    nt_header.OptionalHeader.DataDirectory[dir].VirtualAddress = addr;
    nt_header.OptionalHeader.DataDirectory[dir].Size = size;
}

/*********************************************************** 
 * 功能:	得到放置静态库的目录
 **********************************************************/
char *get_lib_path()
{
    /* 我们假定编译需要链接的静态库放在与编译器同级目录的lib文件夹下
	   此处需要讲一下静态库
	*/
    char path[MAX_PATH];
	char *p;
    GetModuleFileNameA(NULL, path, sizeof(path));
	p = strrchr(path,'\\');
	*p = '\0';
	strcat(path,"\\lib\\");
    return strdup(path);
}

