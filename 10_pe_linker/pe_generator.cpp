#include "pe_generator.h"

char *entry_symbol = "_entry";
extern std::vector<Section *> vecSection;

extern Section *sec_text,			// 代码节
				*sec_idata,			// 导入节
				*sec_rel,			// 重定位信息节
				*sec_symtab,		// 符号表节	
				*sec_dynsymtab;		// 链接库符号节

char *lib_path;
std::vector<std::string> vecDllName;
std::vector<std::string> vecLib;

short g_subsystem;
int   g_output_type;				// 输出文件类型

extern int nsec_image;				// 映像文件（即可执行文件）节个数

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
		0x010B,       /* WORD OptionalHeader.Magic                     映像文件（即可执行文件）的状态。
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
 * 功能:	加载静态库。
            打开slib库文件，读出每一行，调用coffsym_add函数，
			把库文件里面的符号加到sec_dynsymtab里面。
 * name:    静态库文件名,不包括路径和后缀
 **********************************************************/
int pe_load_lib_file(char * file_name)
{
	char lib_file[MAX_PATH];
	int ret = -1;
	char lib_line[512], *dll_name, *pTrimedLibLinePtr;
	FILE * slib_fp;
	sprintf(lib_file, "%s%s.slib", lib_path, file_name);
	slib_fp = fopen(lib_file, "rb");
	if (slib_fp)
	{
		// dll_name = get_dllname(lib_file);
		dll_name = strrchr(lib_file, '\\');
		vecDllName.push_back(std::string(dll_name));
		for (;;)
		{
			// 取出静态库中的一行。并取出无用字符。
			pTrimedLibLinePtr = get_line(lib_line, sizeof(lib_line), slib_fp);
			if (NULL == pTrimedLibLinePtr)
			{
				break;
			}
			if (0 == *pTrimedLibLinePtr || ';' == *pTrimedLibLinePtr)
			{
				continue;
			}
			int vecDllNameSize = vecDllName.size();
			// 增加COFF符号
			coffsym_add(sec_dynsymtab, pTrimedLibLinePtr, vecDllNameSize,
				sec_text->cSectionIndex, CST_FUNC, IMAGE_SYM_CLASS_EXTERNAL);
		}
		ret = 0;
		if (slib_fp)
		{
			fclose(slib_fp);
		}
	}
	else
	{
		print_error("File open failed", file_name);
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
								 int sym_index, char * importsym_name);
/***********************************************************
 * 功能:	解析程序中用到的外部符号。
            这个的逻辑是这样的。首先取出符号表节中的每一符号。
			除了我们代码中含有的，外部函数例如printf就会是未定义符号。
			我们找到这样的符号。在sec_dynsymtab中查找。
 * pe:		PE信息存储结构指针
   返回值:  0 - 成功，1 - 发现有的符号找不到。
 **********************************************************/
int resolve_coffsyms(struct PEInfo * pe)
{
	CoffSym * coffSymElement;
	int sym_index, sym_end ;
	int ret = 0;
	int found = 1;
	// 得到符号节中，符号的个数。
	sym_end = sec_symtab->data_offset / sizeof(CoffSym);
	// 符号从1开始，因为第0个符号为空。
	for (sym_index = 1; sym_index < sym_end; sym_index++)
	{
		// 取得一个符号
		coffSymElement = (CoffSym *)sec_symtab->bufferSectionData + sym_index;
		// 取得这个符号的名字调试观察用。
		char * dbg_name = 
				sec_symtab->linkSection->bufferSectionData + coffSymElement->coffSymName;		
		// 如果是一个未定义符号。
		if(coffSymElement->shortSectionNumber == IMAGE_SYM_UNDEFINED)
		{
			// 得到这个符号对应的名字字符串。
			char * sym_name = 
				sec_symtab->linkSection->bufferSectionData + coffSymElement->coffSymName;
			// COFF符号类型
			unsigned short typeFunc = coffSymElement->typeFunc;
			// 调用coffsym_search函数在sec_dynsymtab中查找符号。
			// 返回sec_dynsymtab符号表中的序号。
			int imp_sym_index = pe_find_import(sym_name);
			struct ImportSym * import_sym;
			if (0 == imp_sym_index)
				found = 0;
			// 增加导入函数
			import_sym = pe_add_import(pe, imp_sym_index, sym_name);
			if(!import_sym)
				found = 0;
			// 如果找到了符号，并且是函数。
			if (found && typeFunc == CST_FUNC)
			{
				int offset = import_sym->thk_offset;
				char buffer[128];
				// 得到代码节的缓冲区写入位置。
				offset = sec_text->data_offset;
				// 代码节缓冲区预留6个字节的空间。
				// 包括两个字节的JMP命令和4个字节的地址。
				WORD * retTmp = (WORD *)section_ptr_add(sec_text, 6);
				// 在预留空间写入0x25FF。这句Opcode表明是FF25型的IAT调用。
				// 也就是JMP DWORD PTR，后面跟着要跳转到的VA地址。
				// 参考JMP的命令格式在Intel白皮书1064页可以发现：
				//     FF /4表示是"Jump near, absolute indirect, address given in 
				//                 r/m32. Not supported in 64-bit mode."。
				*retTmp = 0x25FF;
				// IAT称为导入地址数组（Import Address Table，IAT）。
				// IAT和GOT非常类似，IAT中表项对应本模块中用到的外部符号的真实地址，
				// 初始为空（也不算为空），在装载后由动态链接器更新为真实地址。
				sprintf(buffer, "IAT.%s", sym_name);
				// 增加名字为"IAT.%s"的COFF符号。定义此符号的节是导入节。参见10.2.7。
				import_sym->iat_index = coffsym_add(sec_symtab, 
					buffer, 0, 
					sec_idata->cSectionIndex, 
					CST_FUNC, IMAGE_SYM_CLASS_EXTERNAL);
				// 为新增的COFF符号增加COFF重定位信息。
				coffreloc_redirect_add(offset + 2,		// 节的偏移位置为JMP命令后面的位置。
					import_sym->iat_index,				// 使用刚刚添加的名字为"IAT.%s"的符号作为符号表的索引。
					sec_text->cSectionIndex,			// 重定位数据的节编号为代码节的节编号。
					IMAGE_REL_I386_DIR32);				// 32位绝对定位。
				// 代码节的缓冲区写入位置。
				import_sym->thk_offset = offset;
				// 经过上面的操作。
				// 我们在代码节中预留了6个字节。
				// 包括两个字节的JMP指令和4个字节的跳转地址。这个地址会在coff_relocs_fixup中填充。
				// 具体的流程参见CoffReloc的定义。
			}
			else
			{
				print_error("Undefined", sym_name);
				ret = 1;
			}
		}
	}
	return ret;
}

/***********************************************************
 * 功能:	查找导入函数
 * symbol:  符号名
 **********************************************************/
int pe_find_import(char * strSymbol)
{
	int sym_index ;
	sym_index = coffsym_search(sec_dynsymtab, strSymbol);
	return sym_index;
}

/***********************************************************
 * 功能:		增加导入函数
 * pe:			PE信息存储结构指针
 * sym_index:	符号表的索引 (sec_dynsymtab符号表)
 * name:		符号名
 **********************************************************/
struct ImportSym * pe_add_import(struct PEInfo * pe,
								 int sym_index, char * importsym_name)
{
	// int i ;
	int dll_index;
	struct ImportInfo *pImportInfoElement;
	struct ImportSym  *pImportSymElement;
	// 链接库符号信息放在链接库符号节中，根据COFF符号表中的序号得到这个符号的CoffSym信息。
	CoffSym * externalDbgSym = ((CoffSym *)sec_dynsymtab->bufferSectionData + sym_index);
	// 之后取得这个符号的名字调试观察用。
	char * dbg_name = 
				sec_dynsymtab->linkSection->bufferSectionData + externalDbgSym->coffSymName;
	// 取得导入模块索引。
	dll_index = ((CoffSym *)sec_dynsymtab->bufferSectionData + sym_index)->coff_sym_value;
	if (0 == dll_index)
		return NULL;
	// 如果pe->imps已经初始化。说明这个导入函数已经增加。
	if (pe->imps.size() > dll_index)
	{
		// 取出导入模块索引对应的导入模块。
		pImportInfoElement = pe->imps[dll_index];
	}
	// 如果pe->imps没有初始化过。
	else
	{
		// 创建一个新的ImportInfo对象。
		pImportInfoElement = (struct ImportInfo *)mallocz(sizeof(struct ImportInfo));
		pImportInfoElement->imp_syms.reserve(8);
		// 记下导入模块索引。
		pImportInfoElement->dll_index = dll_index;
		// 添加到导入模块表中。
		pe->imps.push_back(pImportInfoElement);
	}
	// 如果导入函数已经增加。
	if (pImportInfoElement->imp_syms.size() > sym_index) // 找到直接返回，找不到则填加。
	{
		return (struct ImportSym *)&(pImportInfoElement->imp_syms[sym_index]);
	}
	else
	{
		// 分配空间，包括ImportSym结构体和紧跟着后面的符号名空间。
		pImportSymElement = (struct ImportSym *)mallocz(
			sizeof(struct ImportSym) + strlen(importsym_name));
		// 复制符号名到IMAGE_IMPORT_BY_NAME的Name里面。
		strcpy((char *)(&pImportSymElement->imp_sym.Name), importsym_name);
		// 把
		pImportInfoElement->imp_syms.push_back(pImportSymElement);
		return pImportSymElement;
	}
}
	
DWORD pe_virtual_align(DWORD n);
void pe_build_imports(struct PEInfo * pe);
/**************************************************************************/
/* 功能:	计算节区的RVA地址                                             */
/*          RVA地址，即相对虚拟地址。                                     */
/*          对于映像文件来说，它是某项内容被加载进内存后的地址减去        */
/*          映像文件的基地址。也就是基于IMAGE BASE的内存地址。            */
/*          某项内容的RVA几乎总是与它在磁盘上的文件位置(文件指针)不同。   */
/*          对于目标文件来说， RVA并没有什么意义， 因为内存位置尚未分配。 */
/*          在这种情况下，RVA是一个节(后面将要描述)中的地址，             */
/*          这个地址在以后链接时要被重定位。                              */
/*          为了简单起见，编译器应该将每个节的首个RVA设置为0              */
/* pe:		PE信息存储结构指针                                            */
/**************************************************************************/
int pe_assign_addresses(struct PEInfo * pe)
{
	int i ;
	DWORD addr ;
	Section *sec, **ps;
	// 指向导入节。参见10.2.7。
	pe->thunk = sec_idata;
	// 根据映像文件（即可执行文件）节个数分配空间。
	pe->secs = (Section **)mallocz(nsec_image * sizeof(Section *));
	// RVA地址，即相对虚拟地址。也就是基于IMAGE BASE的内存地址。
	// 这里为 0x00400001
	addr = nt_header.OptionalHeader.ImageBase + 1;
	for (i = 0; i < nsec_image; ++i)
	{
		// 取出一个节。
		sec = vecSection[i];
		// 让pe->secs[pe->sec_size]指向vecSection[i]。
		ps  = &pe->secs[pe->sec_size];
		*ps = sec;
		// 对于可执行映像来说，这个域的值是这个节被加载进内存之后
		// 它的第一个字节相对于映像基址的偏移地址。
		// 使用pe_virtual_align函数实现内存中1000H字节对齐。
		sec->sh.VirtualAddress = addr = pe_virtual_align(addr);
		// 如果取出来的是导入节。
		if (sec == pe->thunk)
		{
			// 创建导入信息
			pe_build_imports(pe);
		}
		// 如果节数据不为空。
		if (sec->data_offset)
		{
			// 把节数据累加到addr上面。
			addr += sec->data_offset;
			// 指向PE的下一个Section。
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
/***********************************************************
 * 功能:	创建导入信息（导入目录表及导入符号表）
 * pe:		pe:		PE信息存储结构指针
 **********************************************************/
void pe_build_imports(struct PEInfo * pe)
{
	int thk_ptr, ent_ptr, dll_ptr, sym_cnt, i;
	// 计算节被加载进内存之后它的第一个字节相对于映像基址的偏移地址。
	// 对于目标文件来说，这个域的值是没有重定位之前其第一个字节的地址；
	// 为了简单起见，编译器应该把此值设置为0；并且在重定位时应该从偏移地址中减去这个值。
	// 这里pe->thunk指向sec_idata。
	DWORD rva_base = pe->thunk->sh.VirtualAddress - 
		nt_header.OptionalHeader.ImageBase;
	// 得到导入函数的个数。
	int ndlls = pe->imps.size();
	sym_cnt = 0;
	for (i = 0; i < ndlls; ++i)
	{
#ifdef _DEBUG
		
#endif
		sym_cnt += ((struct ImportInfo *)pe->imps[i])->imp_syms.size();
	}
	if (sym_cnt == 0)
	{
		return ;
	}
	// 设置成sec_idata的当前数据偏移位置。sec_idata是导入节。参见10.2.7。
	pe->imp_offs = dll_ptr = pe->thunk->data_offset;
	// 一个导入函数占用一个IMAGE_IMPORT_DESCRIPTOR结构体。加一个空表项。
	// 
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
		char * dll_name = (char *)vecDllName[p->dll_index - 1].c_str();
		//
		v = put_import_str(pe->thunk, dll_name);
		// 设定导入地址表的RVA，这个表的内容与导入查找表的内容完全一样，直到映像被绑定。
		p->imphdr.FirstThunk         = thk_ptr + rva_base;
		// 设定导入查找表的RVA，这个表包含了每一个导入符号的名称或序号。
		p->imphdr.OriginalFirstThunk = ent_ptr + rva_base;
		// 包含DLL名称的ASCII码字符串相对于映像基址的偏移地址。
		p->imphdr.Name               = v       + rva_base;
		// 向导入节写入数据。
		memcpy(pe->thunk->bufferSectionData + dll_ptr, 
			&p->imphdr, sizeof(IMAGE_IMPORT_DESCRIPTOR));

		for (k = 0, n = p->imp_syms.size(); k <= n; ++k)
		{
			if(k < n)
			{
				struct ImportSym * import_sym = (struct ImportSym *)
					p->imp_syms[k];
				DWORD iat_index = import_sym->iat_index;
				CoffSym * org_sym = (CoffSym *)sec_symtab->bufferSectionData + iat_index;

				org_sym->coff_sym_value       = thk_ptr;
				org_sym->shortSectionNumber   = pe->thunk->cSectionIndex;
				v = pe->thunk->data_offset + rva_base;

				section_ptr_add(pe->thunk, sizeof(import_sym->imp_sym.Hint));
				put_import_str(pe->thunk, (char *)import_sym->imp_sym.Name);
			}
			else
			{
				v = 0 ;
			}
			*(DWORD *)(pe->thunk->bufferSectionData + thk_ptr) = 
				*(DWORD *)(pe->thunk->bufferSectionData + ent_ptr) = v;
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
	sym_end = (CoffSym *)(sec_symtab->bufferSectionData + sec_symtab->data_offset);
	// 节表的索引(从1开始)
	for (sym = (CoffSym *)sec_symtab->bufferSectionData + 1;
		sym < sym_end; sym++)
	{
		// 节表的索引(从1开始)
		sec = (Section *)vecSection[sym->shortSectionNumber - 1];
		sym->coff_sym_value += sec->sh.VirtualAddress;
	}
}

/*直接重定位--全局变量 间接重定位--函数调用*/
/*wndclass.lpfnWndProc   = WndProc ;属于直接重定位*/
/************************************************************************/
/* 功能:	修正需要进行重定位的代码或数据的地址                        */

/************************************************************************/
void coff_relocs_fixup()
{
	Section * sec, * sr;
	CoffReloc * rel, *rel_end, * qrel;
	CoffSym * sym;
	int type, sym_index;
	char * ptr;
	unsigned long val, addr;
	char * sym_name;

	sr = sec_rel;
	rel_end = (CoffReloc *)(sr->bufferSectionData + sr->data_offset);
	qrel = (CoffReloc *)sr->bufferSectionData;
	for (rel = qrel; rel < rel_end; rel++)
	{
		// 根据COFF重定位信息中需要重定位数据的节编号，找到对应的节。
		sec = (Section *)vecSection[rel->sectionIndex - 1];
		// 得到COFF重定位信息的符号表索引。
		sym_index = rel->cfsymIndex;
		// 根据符号表索引找到符号表节对应的符号。
		sym      = &((CoffSym *)sec_symtab->bufferSectionData)[sym_index];
		// 得到这个符号对应的名字字符串。
		sym_name = sec_symtab->linkSection->bufferSectionData + sym->coffSymName;
		// 得到这个符号在节的位置。
		val      = sym->coff_sym_value;
		// 得到这个符号的类型。只有两个结果值：0x0 - 非函数和0x20 - 函数。
		type     = rel->typeReloc;
		// 得到这个节被加载进内存之后它的第一个字节相对于映像基址的偏移地址。
		// 加上重定位的代码或数据的地址。得到的是
		addr     = sec->sh.VirtualAddress + rel->offsetAddr;
		// 得到这个节的缓冲区写入位置。
		ptr      = sec->bufferSectionData  + rel->offsetAddr;
		// 写入数据。
		switch(type) {
		case IMAGE_REL_I386_DIR32:
			// 对于32位绝对定位。这个地址就是符号在节中的位置。
			*(int *)ptr += val;
			break;
		case IMAGE_REL_I386_REL32:
			// 对于32位相对定位。这个地址就是符号在节中的位置减去映像基址偏移和重定位偏移地址。
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
	FILE * pe_fp;
	DWORD file_offset, r;
	int sizeofHeaders;

	pe_fp = fopen(pe->filename, "wb");
	if (NULL == pe_fp)
	{
		print_error(" generate failed", (char *)pe->filename);
		return 1;
	}
	// 计算PE文件的MS-DOS占位程序、PE文件头和节头的总大小，
	// 向上舍入为File Alignment的倍数
	sizeofHeaders = pe_file_align(
		sizeof(g_dos_header) + 
		sizeof(dos_stub) + 
		sizeof(nt_header) +
		pe->sec_size * sizeof(IMAGE_SECTION_HEADER));
	// 循环处理每一个节。用节信息填充NT头和IMAGE_SECTION_HEADER结构体。
	for (i = 0; i < pe->sec_size; ++i)
	{
		Section * sec = pe->secs[i];
		// 取出节的名字。
		char * sh_name = (char *)sec->sh.Name;
		// 计算节被加载进内存之后它的第一个字节相对于映像基址的偏移地址。
		// 对于目标文件来说，这个域的值是没有重定位之前其第一个字节的地址；
		// 为了简单起见，编译器应该把此值设置为0；并且在重定位时应该从偏移地址中减去这个值。
		unsigned long addr = sec->sh.VirtualAddress -     // 
					nt_header.OptionalHeader.ImageBase;   // 内存映像首地址。
		// 取出当前数据偏移位置。
		unsigned long size = sec->data_offset;
		// 取出节头
		IMAGE_SECTION_HEADER * psh = &sec->sh;
		// 对于代码节
		if (!strcmp((char *)sec->sh.Name, ".text"))
		{
			// 前面计算出来的addr就是当映像被加载进内存时代码节的开头相对于映像基址的偏移地址。
			nt_header.OptionalHeader.BaseOfCode = addr;
			// 这个地址加上前面计算出来的程序入口点就是启动地址。
			nt_header.OptionalHeader.AddressOfEntryPoint
				= addr + pe->entry_addr;
		}
		// 对于数据节
		else if (!strcmp((char *)sec->sh.Name, ".data"))
		{
			// 前面计算出来的addr就是当映像被加载进内存时数据节的开头相对于映像基址的偏移地址。
			nt_header.OptionalHeader.BaseOfData = addr;
		}
		// 对于数据节
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
			fwrite(sec->bufferSectionData, 1, sec->data_offset, pe_fp);
			file_offset = pe_file_align(file_offset + sec->data_offset);
			psh->SizeOfRawData = file_offset = 1;
			fpad(pe_fp, file_offset);
		}
	}
	// 节的数目
	nt_header.FileHeader.NumberOfSections = pe->sec_size;
	// 设置MS-DOS占位程序、PE文件头和节头的总大小
	nt_header.OptionalHeader.SizeOfHeaders = sizeofHeaders;
	// 设置运行此映像所需的子系统。
	nt_header.OptionalHeader.Subsystem = g_subsystem;
	// 文件指针回到开头。
	fseek(pe_fp, SEEK_SET, 0);
	// 写入PE文件的DOS部分
	fwrite(&g_dos_header, 1, sizeof(g_dos_header), pe_fp);
	// 写入PE文件的MS-DOS占位程序。
	fwrite(&dos_stub, 1, sizeof(dos_stub), pe_fp);
	// 写入PE文件的NT头的大小。
	fwrite(&nt_header, 1, sizeof(nt_header), pe_fp);
	// 写入所有的节
	for (i = 0; i < pe->sec_size; ++i)
	{
		fwrite(&pe->secs[i]->sh, 1, sizeof(IMAGE_SECTION_HEADER), pe_fp);
	}
	fclose(pe_fp);
	return 0;
}
void get_entry_addr(struct PEInfo * pe);
/***********************************************************
 * 功能:		输出PE文件
 * filename:	EXE文件名
 **********************************************************/
int pe_output_file(char * file_name)
{
	int ret;
	struct PEInfo pe;

	memset(&pe, 0, sizeof(pe) - sizeof(std::vector<struct ImportInfo *>));
	pe.imps.reserve(8);
	pe.filename = file_name;
	// 添加运行库。
	add_runtime_libs();
	// 计算程序入口点
	get_entry_addr(&pe);
	// 解析程序中用到的外部符号
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

/***********************************************************
 * 功能:	计算程序入口点
 * pe:		PE信息存储结构指针
 **********************************************************/
void get_entry_addr(struct PEInfo * pe)
{
	unsigned long addr = 0;
	int cs;
	CoffSym * cfsym_entry;
	// 查找名为_entry的符号。返回符号COFF符号表中的序号。
	cs = coffsym_search(sec_symtab, entry_symbol);
	// 使用序号在符号COFF符号表中找到对应的CoffSym结构体。
	cfsym_entry = (CoffSym *)sec_symtab->bufferSectionData + cs;
	// 对于函数，coff_sym_value表示当前指令在代码节位置。
	addr = cfsym_entry->coff_sym_value;
	// 把当前指令在代码节位置放入entry_addr中。
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

