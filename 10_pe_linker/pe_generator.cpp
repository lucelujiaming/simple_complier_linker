#include "pe_generator.h"

#include "get_token.h"
#include "translation_unit.h"

extern std::vector<Section> vecSection;

extern Section *sec_text,			// 代码节
				*sec_data,			// 数据节
				*sec_bss,			// 未初始化数据节
				*sec_idata,			// 导入表节
				*sec_rdata,			// 只读数据节
				*sec_rel,			// 重定位信息节
				*sec_symtab,		// 符号表节	
				*sec_dynsymtab;		// 链接库符号节

char *lib_path;

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
	// 读取代码节偏移
	cur_text_offset = sec_text->data_offset;
	// 读取重定位信息节偏移
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

