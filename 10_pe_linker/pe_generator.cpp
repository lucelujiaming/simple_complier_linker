#include "pe_generator.h"

char *entry_symbol = "_entry";
extern std::vector<Section *> vecSection;

extern Section *sec_text,			// �����
				*sec_idata,			// �����
				*sec_rel,			// �ض�λ��Ϣ��
				*sec_symtab,		// ���ű��	
				*sec_dynsymtab;		// ���ӿ���Ž�

char *lib_path;
std::vector<std::string> vecDllName;
std::vector<std::string> vecLib;

short g_subsystem;
int   g_output_type;				// ����ļ�����

extern int nsec_image;				// ӳ���ļ�������ִ���ļ����ڸ���

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
		0x010B,       /* WORD OptionalHeader.Magic                     ӳ���ļ�������ִ���ļ�����״̬��
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

char *get_line(char *line, int size, FILE *fp);
/***********************************************************
 * ����:	���ؾ�̬�⡣
            ��slib���ļ�������ÿһ�У�����coffsym_add������
			�ѿ��ļ�����ķ��żӵ�sec_dynsymtab���档
 * name:    ��̬���ļ���,������·���ͺ�׺
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
			// ȡ����̬���е�һ�С���ȡ�������ַ���
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
			// ����COFF����
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
		char* vecLibLine = (char *)vecLib[i].c_str();
		pe_load_lib_file(vecLibLine);
	}
}

int pe_find_import(char * strSymbol);
struct ImportSym * pe_add_import(struct PEInfo * pe,
								 int sym_index, char * importsym_name);
/***********************************************************
 * ����:	�����������õ����ⲿ���š�
            ������߼��������ġ�����ȡ�����ű���е�ÿһ���š�
			�������Ǵ����к��еģ��ⲿ��������printf�ͻ���δ������š�
			�����ҵ������ķ��š���sec_dynsymtab�в��ҡ�
 * pe:		PE��Ϣ�洢�ṹָ��
   ����ֵ:  0 - �ɹ���1 - �����еķ����Ҳ�����
 **********************************************************/
int resolve_coffsyms(struct PEInfo * pe)
{
	CoffSym * coffSymElement;
	int sym_index, sym_end ;
	int ret = 0;
	int found = 1;
	// �õ����Ž��У����ŵĸ�����
	sym_end = sec_symtab->data_offset / sizeof(CoffSym);
	// ���Ŵ�1��ʼ����Ϊ��0������Ϊ�ա�
	for (sym_index = 1; sym_index < sym_end; sym_index++)
	{
		// ȡ��һ������
		coffSymElement = (CoffSym *)sec_symtab->bufferSectionData + sym_index;
		// ȡ��������ŵ����ֵ��Թ۲��á�
		char * dbg_name = 
				sec_symtab->linkSection->bufferSectionData + coffSymElement->coffSymName;		
		// �����һ��δ������š�
		if(coffSymElement->shortSectionNumber == IMAGE_SYM_UNDEFINED)
		{
			// �õ�������Ŷ�Ӧ�������ַ�����
			char * sym_name = 
				sec_symtab->linkSection->bufferSectionData + coffSymElement->coffSymName;
			// COFF��������
			unsigned short typeFunc = coffSymElement->typeFunc;
			// ����coffsym_search������sec_dynsymtab�в��ҷ��š�
			// ����sec_dynsymtab���ű��е���š�
			int imp_sym_index = pe_find_import(sym_name);
			struct ImportSym * import_sym;
			if (0 == imp_sym_index)
				found = 0;
			// ���ӵ��뺯��
			import_sym = pe_add_import(pe, imp_sym_index, sym_name);
			if(!import_sym)
				found = 0;
			// ����ҵ��˷��ţ������Ǻ�����
			if (found && typeFunc == CST_FUNC)
			{
				int offset = import_sym->thk_offset;
				char buffer[128];
				// �õ�����ڵĻ�����д��λ�á�
				offset = sec_text->data_offset;
				// ����ڻ�����Ԥ��6���ֽڵĿռ䡣
				// ���������ֽڵ�JMP�����4���ֽڵĵ�ַ��
				WORD * retTmp = (WORD *)section_ptr_add(sec_text, 6);
				// ��Ԥ���ռ�д��0x25FF�����Opcode������FF25�͵�IAT���á�
				// Ҳ����JMP DWORD PTR���������Ҫ��ת����VA��ַ��
				// �ο�JMP�������ʽ��Intel��Ƥ��1064ҳ���Է��֣�
				//     FF /4��ʾ��"Jump near, absolute indirect, address given in 
				//                 r/m32. Not supported in 64-bit mode."��
				*retTmp = 0x25FF;
				// IAT��Ϊ�����ַ���飨Import Address Table��IAT����
				// IAT��GOT�ǳ����ƣ�IAT�б����Ӧ��ģ�����õ����ⲿ���ŵ���ʵ��ַ��
				// ��ʼΪ�գ�Ҳ����Ϊ�գ�����װ�غ��ɶ�̬����������Ϊ��ʵ��ַ��
				sprintf(buffer, "IAT.%s", sym_name);
				// ��������Ϊ"IAT.%s"��COFF���š�����˷��ŵĽ��ǵ���ڡ��μ�10.2.7��
				import_sym->iat_index = coffsym_add(sec_symtab, 
					buffer, 0, 
					sec_idata->cSectionIndex, 
					CST_FUNC, IMAGE_SYM_CLASS_EXTERNAL);
				// Ϊ������COFF��������COFF�ض�λ��Ϣ��
				coffreloc_redirect_add(offset + 2,		// �ڵ�ƫ��λ��ΪJMP��������λ�á�
					import_sym->iat_index,				// ʹ�øո���ӵ�����Ϊ"IAT.%s"�ķ�����Ϊ���ű��������
					sec_text->cSectionIndex,			// �ض�λ���ݵĽڱ��Ϊ����ڵĽڱ�š�
					IMAGE_REL_I386_DIR32);				// 32λ���Զ�λ��
				// ����ڵĻ�����д��λ�á�
				import_sym->thk_offset = offset;
				// ��������Ĳ�����
				// �����ڴ������Ԥ����6���ֽڡ�
				// ���������ֽڵ�JMPָ���4���ֽڵ���ת��ַ�������ַ����coff_relocs_fixup����䡣
				// ��������̲μ�CoffReloc�Ķ��塣
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
 * ����:	���ҵ��뺯��
 * symbol:  ������
 **********************************************************/
int pe_find_import(char * strSymbol)
{
	int sym_index ;
	sym_index = coffsym_search(sec_dynsymtab, strSymbol);
	return sym_index;
}

/***********************************************************
 * ����:		���ӵ��뺯��
 * pe:			PE��Ϣ�洢�ṹָ��
 * sym_index:	���ű������ (sec_dynsymtab���ű�)
 * name:		������
 **********************************************************/
struct ImportSym * pe_add_import(struct PEInfo * pe,
								 int sym_index, char * importsym_name)
{
	// int i ;
	int dll_index;
	struct ImportInfo *pImportInfoElement;
	struct ImportSym  *pImportSymElement;
	// ���ӿ������Ϣ�������ӿ���Ž��У�����COFF���ű��е���ŵõ�������ŵ�CoffSym��Ϣ��
	CoffSym * externalDbgSym = ((CoffSym *)sec_dynsymtab->bufferSectionData + sym_index);
	// ֮��ȡ��������ŵ����ֵ��Թ۲��á�
	char * dbg_name = 
				sec_dynsymtab->linkSection->bufferSectionData + externalDbgSym->coffSymName;
	// ȡ�õ���ģ��������
	dll_index = ((CoffSym *)sec_dynsymtab->bufferSectionData + sym_index)->coff_sym_value;
	if (0 == dll_index)
		return NULL;
	// ���pe->imps�Ѿ���ʼ����˵��������뺯���Ѿ����ӡ�
	if (pe->imps.size() > dll_index)
	{
		// ȡ������ģ��������Ӧ�ĵ���ģ�顣
		pImportInfoElement = pe->imps[dll_index];
	}
	// ���pe->impsû�г�ʼ������
	else
	{
		// ����һ���µ�ImportInfo����
		pImportInfoElement = (struct ImportInfo *)mallocz(sizeof(struct ImportInfo));
		pImportInfoElement->imp_syms.reserve(8);
		// ���µ���ģ��������
		pImportInfoElement->dll_index = dll_index;
		// ��ӵ�����ģ����С�
		pe->imps.push_back(pImportInfoElement);
	}
	// ������뺯���Ѿ����ӡ�
	if (pImportInfoElement->imp_syms.size() > sym_index) // �ҵ�ֱ�ӷ��أ��Ҳ�������ӡ�
	{
		return (struct ImportSym *)&(pImportInfoElement->imp_syms[sym_index]);
	}
	else
	{
		// ����ռ䣬����ImportSym�ṹ��ͽ����ź���ķ������ռ䡣
		pImportSymElement = (struct ImportSym *)mallocz(
			sizeof(struct ImportSym) + strlen(importsym_name));
		// ���Ʒ�������IMAGE_IMPORT_BY_NAME��Name���档
		strcpy((char *)(&pImportSymElement->imp_sym.Name), importsym_name);
		// ��
		pImportInfoElement->imp_syms.push_back(pImportSymElement);
		return pImportSymElement;
	}
}
	
DWORD pe_virtual_align(DWORD n);
void pe_build_imports(struct PEInfo * pe);
/**************************************************************************/
/* ����:	���������RVA��ַ                                             */
/*          RVA��ַ������������ַ��                                     */
/*          ����ӳ���ļ���˵������ĳ�����ݱ����ؽ��ڴ��ĵ�ַ��ȥ        */
/*          ӳ���ļ��Ļ���ַ��Ҳ���ǻ���IMAGE BASE���ڴ��ַ��            */
/*          ĳ�����ݵ�RVA�������������ڴ����ϵ��ļ�λ��(�ļ�ָ��)��ͬ��   */
/*          ����Ŀ���ļ���˵�� RVA��û��ʲô���壬 ��Ϊ�ڴ�λ����δ���䡣 */
/*          ����������£�RVA��һ����(���潫Ҫ����)�еĵ�ַ��             */
/*          �����ַ���Ժ�����ʱҪ���ض�λ��                              */
/*          Ϊ�˼������������Ӧ�ý�ÿ���ڵ��׸�RVA����Ϊ0              */
/* pe:		PE��Ϣ�洢�ṹָ��                                            */
/**************************************************************************/
int pe_assign_addresses(struct PEInfo * pe)
{
	int i ;
	DWORD addr ;
	Section *sec, **ps;
	// ָ����ڡ��μ�10.2.7��
	pe->thunk = sec_idata;
	// ����ӳ���ļ�������ִ���ļ����ڸ�������ռ䡣
	pe->secs = (Section **)mallocz(nsec_image * sizeof(Section *));
	// RVA��ַ������������ַ��Ҳ���ǻ���IMAGE BASE���ڴ��ַ��
	// ����Ϊ 0x00400001
	addr = nt_header.OptionalHeader.ImageBase + 1;
	for (i = 0; i < nsec_image; ++i)
	{
		// ȡ��һ���ڡ�
		sec = vecSection[i];
		// ��pe->secs[pe->sec_size]ָ��vecSection[i]��
		ps  = &pe->secs[pe->sec_size];
		*ps = sec;
		// ���ڿ�ִ��ӳ����˵��������ֵ������ڱ����ؽ��ڴ�֮��
		// ���ĵ�һ���ֽ������ӳ���ַ��ƫ�Ƶ�ַ��
		// ʹ��pe_virtual_align����ʵ���ڴ���1000H�ֽڶ��롣
		sec->sh.VirtualAddress = addr = pe_virtual_align(addr);
		// ���ȡ�������ǵ���ڡ�
		if (sec == pe->thunk)
		{
			// ����������Ϣ
			pe_build_imports(pe);
		}
		// ��������ݲ�Ϊ�ա�
		if (sec->data_offset)
		{
			// �ѽ������ۼӵ�addr���档
			addr += sec->data_offset;
			// ָ��PE����һ��Section��
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
/***********************************************************
 * ����:	����������Ϣ������Ŀ¼��������ű�
 * pe:		pe:		PE��Ϣ�洢�ṹָ��
 **********************************************************/
void pe_build_imports(struct PEInfo * pe)
{
	int thk_ptr, ent_ptr, dll_ptr, sym_cnt, i;
	// ����ڱ����ؽ��ڴ�֮�����ĵ�һ���ֽ������ӳ���ַ��ƫ�Ƶ�ַ��
	// ����Ŀ���ļ���˵��������ֵ��û���ض�λ֮ǰ���һ���ֽڵĵ�ַ��
	// Ϊ�˼������������Ӧ�ðѴ�ֵ����Ϊ0���������ض�λʱӦ�ô�ƫ�Ƶ�ַ�м�ȥ���ֵ��
	// ����pe->thunkָ��sec_idata��
	DWORD rva_base = pe->thunk->sh.VirtualAddress - 
		nt_header.OptionalHeader.ImageBase;
	// �õ����뺯���ĸ�����
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
	// ���ó�sec_idata�ĵ�ǰ����ƫ��λ�á�sec_idata�ǵ���ڡ��μ�10.2.7��
	pe->imp_offs = dll_ptr = pe->thunk->data_offset;
	// һ�����뺯��ռ��һ��IMAGE_IMPORT_DESCRIPTOR�ṹ�塣��һ���ձ��
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
		// �趨�����ַ���RVA�������������뵼����ұ��������ȫһ����ֱ��ӳ�񱻰󶨡�
		p->imphdr.FirstThunk         = thk_ptr + rva_base;
		// �趨������ұ��RVA������������ÿһ��������ŵ����ƻ���š�
		p->imphdr.OriginalFirstThunk = ent_ptr + rva_base;
		// ����DLL���Ƶ�ASCII���ַ��������ӳ���ַ��ƫ�Ƶ�ַ��
		p->imphdr.Name               = v       + rva_base;
		// �����д�����ݡ�
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
	// �ڱ������(��1��ʼ)
	for (sym = (CoffSym *)sec_symtab->bufferSectionData + 1;
		sym < sym_end; sym++)
	{
		// �ڱ������(��1��ʼ)
		sec = (Section *)vecSection[sym->shortSectionNumber - 1];
		sym->coff_sym_value += sec->sh.VirtualAddress;
	}
}

/*ֱ���ض�λ--ȫ�ֱ��� ����ض�λ--��������*/
/*wndclass.lpfnWndProc   = WndProc ;����ֱ���ض�λ*/
/************************************************************************/
/* ����:	������Ҫ�����ض�λ�Ĵ�������ݵĵ�ַ                        */

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
		// ����COFF�ض�λ��Ϣ����Ҫ�ض�λ���ݵĽڱ�ţ��ҵ���Ӧ�Ľڡ�
		sec = (Section *)vecSection[rel->sectionIndex - 1];
		// �õ�COFF�ض�λ��Ϣ�ķ��ű�������
		sym_index = rel->cfsymIndex;
		// ���ݷ��ű������ҵ����ű�ڶ�Ӧ�ķ��š�
		sym      = &((CoffSym *)sec_symtab->bufferSectionData)[sym_index];
		// �õ�������Ŷ�Ӧ�������ַ�����
		sym_name = sec_symtab->linkSection->bufferSectionData + sym->coffSymName;
		// �õ���������ڽڵ�λ�á�
		val      = sym->coff_sym_value;
		// �õ�������ŵ����͡�ֻ���������ֵ��0x0 - �Ǻ�����0x20 - ������
		type     = rel->typeReloc;
		// �õ�����ڱ����ؽ��ڴ�֮�����ĵ�һ���ֽ������ӳ���ַ��ƫ�Ƶ�ַ��
		// �����ض�λ�Ĵ�������ݵĵ�ַ���õ�����
		addr     = sec->sh.VirtualAddress + rel->offsetAddr;
		// �õ�����ڵĻ�����д��λ�á�
		ptr      = sec->bufferSectionData  + rel->offsetAddr;
		// д�����ݡ�
		switch(type) {
		case IMAGE_REL_I386_DIR32:
			// ����32λ���Զ�λ�������ַ���Ƿ����ڽ��е�λ�á�
			*(int *)ptr += val;
			break;
		case IMAGE_REL_I386_REL32:
			// ����32λ��Զ�λ�������ַ���Ƿ����ڽ��е�λ�ü�ȥӳ���ַƫ�ƺ��ض�λƫ�Ƶ�ַ��
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
	// ����PE�ļ���MS-DOSռλ����PE�ļ�ͷ�ͽ�ͷ���ܴ�С��
	// ��������ΪFile Alignment�ı���
	sizeofHeaders = pe_file_align(
		sizeof(g_dos_header) + 
		sizeof(dos_stub) + 
		sizeof(nt_header) +
		pe->sec_size * sizeof(IMAGE_SECTION_HEADER));
	// ѭ������ÿһ���ڡ��ý���Ϣ���NTͷ��IMAGE_SECTION_HEADER�ṹ�塣
	for (i = 0; i < pe->sec_size; ++i)
	{
		Section * sec = pe->secs[i];
		// ȡ���ڵ����֡�
		char * sh_name = (char *)sec->sh.Name;
		// ����ڱ����ؽ��ڴ�֮�����ĵ�һ���ֽ������ӳ���ַ��ƫ�Ƶ�ַ��
		// ����Ŀ���ļ���˵��������ֵ��û���ض�λ֮ǰ���һ���ֽڵĵ�ַ��
		// Ϊ�˼������������Ӧ�ðѴ�ֵ����Ϊ0���������ض�λʱӦ�ô�ƫ�Ƶ�ַ�м�ȥ���ֵ��
		unsigned long addr = sec->sh.VirtualAddress -     // 
					nt_header.OptionalHeader.ImageBase;   // �ڴ�ӳ���׵�ַ��
		// ȡ����ǰ����ƫ��λ�á�
		unsigned long size = sec->data_offset;
		// ȡ����ͷ
		IMAGE_SECTION_HEADER * psh = &sec->sh;
		// ���ڴ����
		if (!strcmp((char *)sec->sh.Name, ".text"))
		{
			// ǰ����������addr���ǵ�ӳ�񱻼��ؽ��ڴ�ʱ����ڵĿ�ͷ�����ӳ���ַ��ƫ�Ƶ�ַ��
			nt_header.OptionalHeader.BaseOfCode = addr;
			// �����ַ����ǰ���������ĳ�����ڵ����������ַ��
			nt_header.OptionalHeader.AddressOfEntryPoint
				= addr + pe->entry_addr;
		}
		// �������ݽ�
		else if (!strcmp((char *)sec->sh.Name, ".data"))
		{
			// ǰ����������addr���ǵ�ӳ�񱻼��ؽ��ڴ�ʱ���ݽڵĿ�ͷ�����ӳ���ַ��ƫ�Ƶ�ַ��
			nt_header.OptionalHeader.BaseOfData = addr;
		}
		// �������ݽ�
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
	// �ڵ���Ŀ
	nt_header.FileHeader.NumberOfSections = pe->sec_size;
	// ����MS-DOSռλ����PE�ļ�ͷ�ͽ�ͷ���ܴ�С
	nt_header.OptionalHeader.SizeOfHeaders = sizeofHeaders;
	// �������д�ӳ���������ϵͳ��
	nt_header.OptionalHeader.Subsystem = g_subsystem;
	// �ļ�ָ��ص���ͷ��
	fseek(pe_fp, SEEK_SET, 0);
	// д��PE�ļ���DOS����
	fwrite(&g_dos_header, 1, sizeof(g_dos_header), pe_fp);
	// д��PE�ļ���MS-DOSռλ����
	fwrite(&dos_stub, 1, sizeof(dos_stub), pe_fp);
	// д��PE�ļ���NTͷ�Ĵ�С��
	fwrite(&nt_header, 1, sizeof(nt_header), pe_fp);
	// д�����еĽ�
	for (i = 0; i < pe->sec_size; ++i)
	{
		fwrite(&pe->secs[i]->sh, 1, sizeof(IMAGE_SECTION_HEADER), pe_fp);
	}
	fclose(pe_fp);
	return 0;
}
void get_entry_addr(struct PEInfo * pe);
/***********************************************************
 * ����:		���PE�ļ�
 * filename:	EXE�ļ���
 **********************************************************/
int pe_output_file(char * file_name)
{
	int ret;
	struct PEInfo pe;

	memset(&pe, 0, sizeof(pe) - sizeof(std::vector<struct ImportInfo *>));
	pe.imps.reserve(8);
	pe.filename = file_name;
	// ������п⡣
	add_runtime_libs();
	// ���������ڵ�
	get_entry_addr(&pe);
	// �����������õ����ⲿ����
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
 * ����:	���������ڵ�
 * pe:		PE��Ϣ�洢�ṹָ��
 **********************************************************/
void get_entry_addr(struct PEInfo * pe)
{
	unsigned long addr = 0;
	int cs;
	CoffSym * cfsym_entry;
	// ������Ϊ_entry�ķ��š����ط���COFF���ű��е���š�
	cs = coffsym_search(sec_symtab, entry_symbol);
	// ʹ������ڷ���COFF���ű����ҵ���Ӧ��CoffSym�ṹ�塣
	cfsym_entry = (CoffSym *)sec_symtab->bufferSectionData + cs;
	// ���ں�����coff_sym_value��ʾ��ǰָ���ڴ����λ�á�
	addr = cfsym_entry->coff_sym_value;
	// �ѵ�ǰָ���ڴ����λ�÷���entry_addr�С�
	pe->entry_addr = addr;
}

/***********************************************************
 * ����:	�����ļ�����λ��
 * n:		δ����ǰλ��
 **********************************************************/	
DWORD pe_file_align(DWORD n)
{
	DWORD FileAlignment = nt_header.OptionalHeader.FileAlignment; //�ļ��жεĶ�������
	return calc_align(n,FileAlignment);
}

/***********************************************************
 * ����:	��������Ŀ¼
 * dir:		Ŀ¼����
 * addr:	���RVA
 * size:	��Ĵ�С(���ֽڼ�)
 **********************************************************/	
void pe_set_datadir(int dir, DWORD addr, DWORD size)
{
    nt_header.OptionalHeader.DataDirectory[dir].VirtualAddress = addr;
    nt_header.OptionalHeader.DataDirectory[dir].Size = size;
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

