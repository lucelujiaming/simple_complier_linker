// coff_generator.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "coff_generator.h"
#include <vector>


std::vector<Section> vecSection;
Section *sec_text,			// 代码节
		*sec_data,			// 数据节
		*sec_bss,			// 未初始化数据节
		*sec_idata,			// 导入表节
		*sec_rdata,			// 只读数据节
		*sec_rel,			// 重定位信息节
		*sec_symtab,		// 符号表节	
		*sec_dynsymtab;		// 链接库符号节

int nsec_image;				// 映像文件节个数

extern std::vector<TkWord> tktable;
extern int sec_text_opcode_ind ;	 	// 指令在代码节位置

void section_realloc(Section * sec, int new_size);
int coffsym_search(Section * symtab, char * name);
char * coffstr_add(Section * strtab, char * name);
void coffreloc_redirect_add(int offset, int cfsym, char section, char type);
void *mallocz(int size);

/***********************************************************
 *  功能:             新建节
 *  name:             节名称
 *  characteristics:  节属性
 *  返回值:           新增加节
 *  nsec_image:       全局变量，映像文件中节个数
 **********************************************************/
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
	if(!(iCharacteristics & IMAGE_SCN_LNK_REMOVE))
		nsec_image++;
	vecSection.push_back(*sec);
	return sec;
}

/***********************************************************
 *  功能:           给节数据预留至少increment大小的内存空间
 *  sec:            预留内存空间的节
 *  increment:      预留的空间大小
 *  返回值:         预留内存空间的首地址
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
 *  功能:           给节数据重新分配内存，并将内容初始化为0
 *  sec:            重新分配内存的节
 *  new_size:       节数据新长度
 **********************************************************/
void section_realloc(Section * sec, int new_size)
{
	int size;
	char * data ;
	// 取得当前的空间大小。这个在section_new中初始化为8。
	size = sec->data_allocated;
	// 当前的空间大小乘以2，直到满足节数据新长度。显然这个值为8的幂。
	while(size < new_size)
		size = size * 2;
	// 使用realloc更改已经配置的内存空间。
	data = (char *)realloc(sec->data, size);
	if(data == NULL)
		printf("realloc error");
	// 因为是更改已经配置的内存空间。
	// 因此上，已经配置过空间被保留，我们只需要初始化新分配的空间。
	memset(data + sec->data_allocated, 0x00, size - sec->data_allocated);
	sec->data = data;
	sec->data_allocated = size;
}

/***********************************************************
 *  功能:            新建存储COFF符号表的节
 *  symtab:          COFF符号表名
 *  Characteristics: 节属性
 *  strtab_name:     与符号表相关的字符串表
 *  返回值:          存储COFF符号表的节
 **********************************************************/
Section * new_coffsym_section(char* symtable_name, int iCharacteristics,
		char * strtable_name)
{
	Section * sec;
	sec = section_new(symtable_name, iCharacteristics);
	sec->link = section_new(strtable_name, iCharacteristics);
	sec->hashtab = (int *)mallocz(sizeof(int) * MAXKEY);
	return sec;
}

/*********************************************************** 
 * 功能:	分配堆内存并将数据初始化为'0'
 * size:	分配内存大小
 **********************************************************/
void *mallocz(int size)
{
    void *ptr;
	ptr = malloc(size);
	if (!ptr && size)
    {
		printf("内存分配失败");
		exit(1);
	}
    memset(ptr, 0, size);
    return ptr;
}

/*********************************************************** 
 * 功能:	计算哈希地址
 * key:		哈希关键字
 * MAXKEY:	哈希表长度
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
 *  功能:            增加COFF符号
 *  symtab:          保存COFF符号表的节
 *  name:            符号名称
 *  val:             与符号相关的值
 *  sec_index:       定义此符号的节
 *  type:            Coff符号类型
 *  StorageClass:    Coff符号存储类别
 *  返回值:          符号COFF符号表中序号
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
		cfsym->name           = csname - strtab->data;   // ????
		cfsym->coff_sym_value = val;
		cfsym->shortSection   = sec_index;
		cfsym->type           = type;
		cfsym->storageClass   = StorageClass;
		// cfsym->coff_sym_value = val;
		keyno = elf_hash(name);
		cfsym->next = hashtab[keyno];

		cs = cfsym - (CoffSym *)symtab->data;
		hashtab[keyno] = cs;
	}
	return cs;
}

/***********************************************************
 *  功能:         增加或更新COFF符号，更新只适用于函数先声明后定义的情况
 *  S:            符号指针
 *  val:          符号值
 *  sec_index:    定义此符号的节
 *  type:         COFF符号类型
 *  StorageClass: COFF符号存储类别
 **********************************************************/
void coffsym_add_update(Symbol *s, int val, int sec_index,
		short type, char StorageClass)
{
	char * name;
	CoffSym *cfsym;
	if(!s->related_value)
	{
		name = ((TkWord)tktable[s->token_code]).spelling;
		s->related_value = coffsym_add(sec_symtab, name, val, sec_index, type, StorageClass);
	}
	else
	{
		cfsym = &((CoffSym *)sec_symtab->data)[s->related_value];
		cfsym->coff_sym_value = val;
		cfsym->shortSection = sec_index;
	}
}

/***********************************************************
 *  功能:            增加COFF符号名字符串
 *  strtab:          保存COFF字符串表的节
 *  name:            符号名称字符串
 *  返回值:          新增COFF字符串
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
 * ASCII的0x2E是.号。0x64是d。 
 * 00 2E 64 61 74 61 00 2E 62 73 73 00 2E 72 64 61 74 61 
 *     .  d  a  t  a     .  b  s  s     .  r  d  a  t  a 
 * 00 6D 61 69 6E 00 70 72 69 6E 74 66 00 5F 65 6E 74 72 
 *     m  a  i  n     p  r  i  n  t  f     _  e  n  t  r 
 * 79 00 65 78 69 74 00                                  
 *  y     e  x  i  t 
 * 字符串以00结尾。因此上有8个字符串条目。第一个是以00结尾的空字符串。 
 **********************************************************/

/***********************************************************
 *  功能:            查找COFF符号
 *  symtab:          保存COFF符号表的节
 *  name:            符号名称
 *  返回值:          符号COFF符号表中序号
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
 *  功能:            增加重定位条目
 *  section:         符号所在节
 *  sym:             符号指针
 *  offset:          需要进行重定位的代码或数据在其相应节的偏移位置
 *  type:            重定位类型
 **********************************************************/
void coffreloc_add(Section * sec, Symbol * sym, int offset, char type)
{
	int cfsym;
	char * name;
	// 如果符号表没有对应的关联值，就为其添加。
	if(!sym->related_value)
		coffsym_add_update(sym, 0, 
			IMAGE_SYM_UNDEFINED, 0,  // CST_FUNC,
			IMAGE_SYM_CLASS_EXTERNAL);
	// 根据符号编码获得单词字符串.
	name = ((TkWord)tktable[sym->token_code]).spelling;
	// 根据单词字符串查找COFF符号。
	cfsym = coffsym_search(sec_symtab, name);
	coffreloc_redirect_add(offset, cfsym, sec->index, type);
}

/***********************************************************
 * 功能:            增加COFF重定位信息
 * offset:          需要进行重定位的代码或数据在其相应节的偏移位置
 * cf sym:          符号表的索引
 * section:         符号所在节
 * type:            重定位类型
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

/************************************************************************/
/* 功能：    记录待定跳转地址的指令链                                   */
/* jmp_addr：前一跳转指令地址                                           */
/*           0x00 - 待填充的跳转指令地址。                              */
/*		     0xXX - break跳转位置或者continue跳转位置。                 */
/************************************************************************/
int makelist(int jmp_addr)
{
	int indNew;
	// 根据指令在代码节位置加4来计算下一个指令的位置。
	indNew = sec_text_opcode_ind + 4;
	// 如果发现溢出，使用新大小从新分配代码节。
	if (indNew > sec_text->data_allocated)
	{
		section_realloc(sec_text, indNew);
	}
	// 把前一跳转指令地址写入代码节位置。
	// 这个值在if和for的时候为零。而在break, continue, return的时候非零。
	*(int *)(sec_text->data + sec_text_opcode_ind) = jmp_addr;
	// 返回值为代码节位置。这句话其实没啥作用。
	jmp_addr = sec_text_opcode_ind;
	// 更新代码节位置。指向下一条指令。
	sec_text_opcode_ind = indNew;
	// 返回值为代码节位置。
	return jmp_addr;
}

/************************************************************************/
/* 功能：       回填函数，把t为链首的各个待定跳转地址填入相对地址       */
/* fill_offset：链首。我认为是需要填充的代码节偏移位置                  */
/* jmp_addr：   指令跳转位置。                                          */
/************************************************************************/
void backpatch(int fill_offset, int jmp_addr)
{
	int next_addr, *ptrSecText;
	// 如果需要填充的代码节偏移位置不为零。
	while (fill_offset)
	{
		// ptr指向代码节偏移位置
		ptrSecText = (int *)(sec_text->data + fill_offset);
		// 记住下一个需要回填位置
		next_addr = *ptrSecText;
		// 填入相对地址
		*ptrSecText = jmp_addr - fill_offset - 4;
		// 准备处理下一个需要回填位置
		fill_offset = next_addr ;
	}
}

/***********************************************************
 * *功能:            COFF初始化
 * *本函数用到全局变量:            
 * Dyn Array sections；       //节数组
 * Section*sec_text，         //代码节
 *         *sec_data，        //数据节
 *         *sec_bss，         //未初始化数据节
 *         *sec_idata，       //导人表节
 *         *sec_rdata，       //只读数据节
 *         *sec_rel，         //重定位信息节
 *         *sec_symtab，      //符号表节
 *         *sec_dynsym tab；  //链接库符号节
 * in tn sec_image；          //映像文件节个数
 **********************************************************/
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

	coffsym_add(sec_symtab, "", 0, 0, 0, IMAGE_SYM_CLASS_NULL);
	coffsym_add(sec_symtab, ".data", 0, sec_data->index, 0, IMAGE_SYM_CLASS_STATIC);
	coffsym_add(sec_symtab, ".bss", 0, sec_bss->index, 0, IMAGE_SYM_CLASS_STATIC);
	coffsym_add(sec_symtab, ".rdata", 0, sec_rdata->index, 0, IMAGE_SYM_CLASS_STATIC);
	coffsym_add(sec_dynsymtab, "", 0, 0, 0, IMAGE_SYM_CLASS_NULL);
}

/***********************************************************
 *  功能:            释放所有节数据
 **********************************************************/
void free_sections()
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

/***********************************************************
 * 功能:            从当前读写位置到new_pos位置用0填补文件内容
 * fp:            文件指针
 * new_pos:            填补终点位置
 **********************************************************/
void fpad(FILE * fp, int new_pos)
{
	int cur_pos = ftell(fp);
	while (++cur_pos <= new_pos)
	{
		fputc(0, fp);
	}
}

/***********************************************************
 * 功能:            输出目标文件
 * name:            目标文件名
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
	fh = (IMAGE_FILE_HEADER *)mallocz(sizeof(IMAGE_FILE_HEADER));
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

// int main(int argc, char* argv[])
// {
//	init_coff();
//	write_obj("write_obj.obj"); 
//	free_sections();
//	return 0;
//}

