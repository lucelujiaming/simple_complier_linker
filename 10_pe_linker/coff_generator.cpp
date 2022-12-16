// coff_generator.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "coff_generator.h"
#include <vector>


std::vector<Section *> vecSection;
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
extern int sec_text_opcode_offset ;	 	// 指令在代码节位置

/************************************************************************/
/*  功能:             新建节                                            */
/*  name:             节名称                                            */
/*  characteristics:  节属性                                            */
/*  返回值:           新增加节                                          */
/*  nsec_image:       全局变量，映像文件中节个数                        */
/************************************************************************/
Section * section_new(char * section_name, int iCharacteristics)
{
	Section * secObj;
	int initSize = 8;
	secObj = (Section *)mallocz(sizeof(Section));
	memcpy(secObj->sh.Name, section_name, strlen(section_name));
	secObj->sh.Characteristics = iCharacteristics;
	secObj->cSectionIndex      = vecSection.size() + 1;  // Start from 1
	secObj->bufferSectionData  = (char *)mallocz(sizeof(char) * initSize);
	secObj->data_allocated     = initSize;
	// IMAGE_SCN_LNK_REMOVE代表此节不会成为最终形成的映像文件的一部分。
	// 此标志仅对目标文件合法，
	// 通过这个标志可判断该节是否需要放人映像文件中，
	// 如果需要放入映像文件， nsec_image自动加1， 
	// 在所有节新建完成后， nsec_image变量记录了映像文件的节个数。
	if(!(iCharacteristics & IMAGE_SCN_LNK_REMOVE))
		nsec_image++;
	vecSection.push_back(secObj);
	return secObj;
}

/************************************************************************/
/*  功能:           给节数据重新分配内存，并将内容初始化为0             */
/*  sec:            重新分配内存的节                                    */
/*  new_size:       节数据新长度                                        */
/************************************************************************/
void section_realloc(Section * sec, int new_size)
{
	int sizeAllocate;
	char * dataBuffer ;
	// 取得当前的空间大小。这个在section_new中初始化为8。
	sizeAllocate = sec->data_allocated;
	// 当前的空间大小乘以2，直到满足节数据新长度。显然这个值为8的幂。
	while(sizeAllocate < new_size)
		sizeAllocate = sizeAllocate * 2;
	// 使用realloc更改已经配置的内存空间。发生以下情况：
	//   1）如果当前内存段后面有需要的内存空间，则直接扩展这段内存空间，realloc()将返回原指针。
	//   2）如果当前内存段后面的空闲字节不够，那么就使用堆中的第一个能够满足这一要求的内存块，
	//      将目前的数据复制到新的位置，并将原来的数据块释放掉，返回新的内存块位置。
	dataBuffer = (char *)realloc(sec->bufferSectionData, sizeAllocate);
	if(dataBuffer == NULL)
		printf("realloc error");
	// 因为是更改已经配置的内存空间。
	// 因此上，已经配置过空间被保留，我们只需要初始化新分配的空间。
	memset(dataBuffer + sec->data_allocated, 0x00, sizeAllocate - sec->data_allocated);
	sec->bufferSectionData = dataBuffer;
	sec->data_allocated    = sizeAllocate;
}

/************************************************************************/
/*  功能:           给节数据预留至少increment大小的内存空间             */
/*  sec:            预留内存空间的节                                    */
/*  increment:      预留的空间大小                                      */
/*  返回值:         预留内存空间的首地址                                */
/************************************************************************/
void * section_ptr_add(Section * sec, int increment)
{
	int offset, offsetOne;
	offset = sec->data_offset;
	offsetOne = offset + increment;
	if(offsetOne > sec->data_allocated)
		section_realloc(sec, offsetOne);
	sec->data_offset = offsetOne;
	return sec->bufferSectionData + offset;
}

/************************************************************************/
/*  功能:            新建存储COFF符号表的节                             */
/*  symtab:          COFF符号表名                                       */
/*  Characteristics: 节属性                                             */
/*  strtab_name:     与符号表相关的字符串表                             */
/*  返回值:          存储COFF符号表的节                                 */
/************************************************************************/
Section * coffsym_section_new(char* symtable_name, int iCharacteristics,
		char * strtable_name)
{
	Section * sec;
	sec = section_new(symtable_name, iCharacteristics);
	sec->linkSection = section_new(strtable_name, iCharacteristics);
	sec->sym_hashtab = (int *)mallocz(sizeof(int) * MAXKEY);
	return sec;
}

/************************************************************************/
/* 功能:	分配堆内存并将数据初始化为'0'                               */
/* size:	分配内存大小                                                */
/************************************************************************/
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

/************************************************************************/
/* 功能:	计算哈希地址                                                */
/* key:		哈希关键字                                                  */
/* MAXKEY:	哈希表长度                                                  */
/*   首先我们的hash结果是一个unsigned int类型的数据：                   */
/*          0000 0000 0000 0000                                         */
/*   1. hash左移4位，将key插入。因为一个char有八位。                    */
/*      这会导致第一个字节的高四位发生进行第一次杂糅。                  */
/*   2. 这里用bites_mask & 0xf0000000获取了hash的第四个字节的高四位，   */
/*      并用高四位作为掩码做第二次杂糅。                                */
/*	                                                                    */
/*      在这里我们首先声明一下，因为我们的ELF hash强调的是每个字符      */
/*      都要对最后的结构有影响，所以说我们左移到一定程度是会吞掉        */
/*	    最高的四位的，所以说我们要将最高的四位先对串产生影响，          */
/*	    再让他被吞掉，之后的所有的影响都是叠加的，                      */
/*	    这就是多次的杂糅保证散列均匀，防止出现冲突的大量出现。          */
/*   3. 将bites_mask右移24位移动到刚才的5-8位那里，                     */
/*      再对5-8位进行第二次杂糅。                                       */
/*   4. 我们定时清空高四位，实际上这部操作我们完全没有必要，            */
/*      但是算法要求，因为我们下一次的左移会自动吞掉这四位。            */
/*   5. key递增，引入下一个字符进行杂糅。                               */
/*   6. 返回一个缺失了最高符号位的无符号数                              */
/*      （为了防止用到了有符号的时候造成的溢出）作为最后的hash值        */
/************************************************************************/
int elf_hash(char *key)
{
    unsigned int hash = 0, bites_mask;
    while (*key) 
	{
        hash = (hash << 4) + *key;          // 1 - 影响高四位，杂糅一次
        bites_mask = hash & 0xf0000000;     // 2 - 用高四位作为掩码做第二次杂糅
        if (bites_mask)
        {
			hash ^= bites_mask >> 24;       // 3 - 影响4-5位，杂糅一次    
			hash &= ~bites_mask;            // 4 - 清空高四位
		}
		key++;                              // 5 - 引入下一个字符进行杂糅
    }
    return hash % MAXKEY;                   // 6 
}

/************************************************************************/
/*  功能:            增加COFF符号                                       */
/*  symtab:          保存COFF符号表的符号节。                           */
/*                   只能是sec_symtab或sec_dynsymtab                    */
/*  name:            符号名称                                           */
/*  val:             与符号相关的值                                     */
/*  sec_index:       定义此符号的节                                     */
/*  type:            Coff符号类型                                       */
/*  StorageClass:    Coff符号存储类别                                   */
/*  返回值:          符号COFF符号表中序号                               */
/************************************************************************/
int coffsym_add(Section * symtab, char * coffsym_name, int val, int sec_index,
		WORD typeFunc, char StorageClass)
{
	CoffSym * cfsym;
	int cs, keyno;
	char * csname;
	Section * strtab = symtab->linkSection;
	int * symtab_hashtable;
	// 得到符号表的哈希表
	symtab_hashtable = symtab->sym_hashtab;
	// 根据符号名称查找是否存在。
	cs = coffsym_search(symtab, coffsym_name);
	// 如果不存在，就添加。
	if(cs == 0)  //
	{
		// 扩充CoffSym来容纳数据。返回的cfsym指向节数据缓冲区的写入位置。
		cfsym = (CoffSym *)section_ptr_add(symtab, sizeof(CoffSym));
		// 增加COFF符号名字符串。
		csname = coffstr_add(strtab, coffsym_name);
		// name字段表示符号名称在字符串表中的偏移位置，因为符号表名称都放在字符串表中。
		cfsym->coffSymName         = csname - strtab->bufferSectionData;
		// 对于函数表示当前指令在代码节位置。
		cfsym->coff_sym_value = val;
		// 节表的索引
		cfsym->shortSectionNumber   = sec_index;
		// 表示类型的数字。0x20 - CST_FUNC, 0x00 - CST_NOTFUNC
		cfsym->typeFunc             = typeFunc;
		// 存储类别通常来说都是IMAGE_SYM_CLASS_EXTERNAL。
		cfsym->storageClass         = StorageClass;
		// 根据符号名称生成哈希值。
		keyno = elf_hash(coffsym_name);
		// 冲突链表指向keyno对应的哈希表项。
		// 当没有哈希冲突的时候，symtab_hashtable[keyno]不存在为零。
		// 这就是为什么COFF符号从1开始的原因。
		// 当存在哈希冲突的时候，symtab_hashtable[keyno]为和keyno一致的CoffSym类型数组下标。
		// 我们让nextCoffSymName指向它，记住这个值。并且更新symtab_hashtable[keyno]的值。
		cfsym->nextCoffSymName = symtab_hashtable[keyno];
		// 这里有一个技巧，就是我们把节数据缓冲区强转成为CoffSym类型指针。
		// 也就是强转成为一个CoffSym类型的数组。而cfsym则是指向这个数组一个元素的指针。
		// 因此上，这里的结算结果就会等于cfsym指向的这个数组元素在整个数组中的下标。
		cs = cfsym - (CoffSym *)symtab->bufferSectionData;
		// 把计算出来的CoffSym类型数组的下标放入哈希表。
		// symtab_hashtable[keyno]里面原来的值已经被保存在cfsym->nextCoffSymName里面.
		// 这样，在coffsym_search中，我们就可以找到cs对应的COFF符号。
		// 并利用nextCoffSymName找到后续冲突元素的下标。
		symtab_hashtable[keyno] = cs;
	}
	return cs;
}

/************************************************************************/
/*  功能:         增加或更新COFF符号，                                  */
/*                更新只适用于函数先声明后定义的情况                    */
/*  sym:          符号指针                                              */
/*  val:          符号值                                                */
/*  sec_index:    定义此符号的节                                        */
/*  typeFunc:     COFF符号类型                                          */
/*  StorageClass: COFF符号存储类别                                      */
/************************************************************************/
void coffsym_add_update(Symbol *sym, int val, int sec_index,
		WORD typeFunc, char StorageClass)
{
	char * coffsym_name;
	CoffSym *cfsym;
	if(!sym->related_value)
	{
		// 根据符号的单词编码取出的单词字符串。
		coffsym_name = ((TkWord)tktable[sym->token_code]).spelling;
		// 在符号表节sec_symtab中，添加一个符号。
		// 返回值是符号COFF符号表中序号，放在related_value中。
		sym->related_value = coffsym_add(sec_symtab, 
					coffsym_name, val, sec_index, typeFunc, StorageClass);
	}
	else
	{
		cfsym = &((CoffSym *)sec_symtab->bufferSectionData)[sym->related_value];
		cfsym->coff_sym_value     = val;
		cfsym->shortSectionNumber = sec_index;
	}
}

/************************************************************************/
/*  功能:            增加COFF符号名字符串                               */
/*  strtab:          保存COFF字符串表的字符串节                         */
/*                   只能是.strtab或.dynstr                             */
/*  name:            符号名称字符串                                     */
/*  返回值:          新增COFF字符串                                     */
/************************************************************************/
char * coffstr_add(Section * strtab, char * coffstr_name)
{
	int len;
	char * pstr;
	len = strlen(coffstr_name);
	pstr = (char *)section_ptr_add(strtab, len + 1);
	memcpy(pstr, coffstr_name, len);
	return pstr;
}

/************************************************************************/
/* ASCII的0x2E是.号。0x64是d。                                          */
/* 00 2E 64 61 74 61 00 2E 62 73 73 00 2E 72 64 61 74 61                */
/*     .  d  a  t  a     .  b  s  s     .  r  d  a  t  a                */
/* 00 6D 61 69 6E 00 70 72 69 6E 74 66 00 5F 65 6E 74 72                */
/*     m  a  i  n     p  r  i  n  t  f     _  e  n  t  r                */
/* 79 00 65 78 69 74 00                                                 */
/*  y     e  x  i  t                                                    */
/* 字符串以00结尾。因此上有8个字符串条目。第一个是以00结尾的空字符串。  */
/************************************************************************/

/************************************************************************/
/*  功能:            查找COFF符号                                       */
/*  symtab:          保存COFF符号表的节                                 */
/*                   只能是sec_symtab或sec_dynsymtab                    */
/*  name:            符号名称                                           */
/*  返回值:          符号COFF符号表中的序号                             */
/************************************************************************/
int coffsym_search(Section * symtab, char * coffsym_name)
{
	CoffSym * cfsym;
	int coffsym_index, keyno ;
	char * csname ;
	Section * strtab;
	// 生成符号名称对应的哈希值。
	keyno  = elf_hash(coffsym_name);
	// 得到符号表对应的字符串表。
	strtab = symtab->linkSection;
	// 得到符号表的哈希表
	coffsym_index     = symtab->sym_hashtab[keyno];
	while(coffsym_index)
	{
		// 得到节中的CoffSym元素。
		cfsym  = (CoffSym *)symtab->bufferSectionData + coffsym_index;
		// 得到字符串表中的字符串。 
		csname = strtab->bufferSectionData + cfsym->coffSymName;
		// 如果内容一致，就返回。
		if(!strcmp(coffsym_name, csname))
			return coffsym_index;
		// 哈希冲突的下一个名字在字符串表中的偏移地址。
		coffsym_index = cfsym->nextCoffSymName;
	}
	return coffsym_index;
}

/************************************************************************/
/*  功能:            增加重定位条目                                     */
/*  section:         符号所在节                                         */
/*  sym:             符号指针                                           */
/*  offset:          需要进行重定位的代码或数据在其相应节的偏移位置     */
/*  type:            重定位类型                                         */
/************************************************************************/
void coffreloc_add(Section * sec, Symbol * sym, int offset, char type_reloc)
{
	int cfsym_index;
	char * token_name;
	// 如果符号表没有对应的关联值，就为其添加。
	//     定义此符号的节为   - IMAGE_SYM_UNDEFINED
	//     添加类型为         - CST_FUNC
	//     COFF符号存储类别为 - IMAGE_SYM_CLASS_EXTERNAL
	// 
	// 这个重定位条目，会在resolve_coffsyms中被处理。
	if(!sym->related_value)
	{
		coffsym_add_update(sym, 0, IMAGE_SYM_UNDEFINED, CST_FUNC, 
			IMAGE_SYM_CLASS_EXTERNAL);
	}
	// 根据符号编码获得单词字符串.
	token_name = ((TkWord)tktable[sym->token_code]).spelling;
	// 根据单词字符串查找COFF符号。
	cfsym_index = coffsym_search(sec_symtab, token_name);
	coffreloc_redirect_add(offset, cfsym_index, sec->cSectionIndex, type_reloc);
}

/************************************************************************/
/* 功能:            增加COFF重定位信息                                  */
/* offset:          需要进行重定位的代码或数据在其相应节的偏移位置      */
/* cfsym:           符号表的索引                                        */
/* section:         符号所在节的索引                                    */
/* type:            重定位类型                                          */
/************************************************************************/
void coffreloc_redirect_add(int offset, int cfsym_index, 
							char section_index, char type_reloc)
{
	CoffReloc * rel;
	rel = (CoffReloc *)section_ptr_add(sec_rel, sizeof(CoffReloc));
	rel->offsetAddr   = offset;
	rel->cfsymIndex   = cfsym_index;
	rel->sectionIndex = section_index;
	rel->typeReloc    = type_reloc;
}

/************************************************************************/
/* 功能：    记录待定跳转地址的指令链                                   */
/* jmp_addr：前一跳转指令地址                                           */
/*           0x00 - 待填充的跳转指令地址。                              */
/*		     0xXX - break跳转位置或者continue跳转位置。                 */
/************************************************************************/
int make_jmpaddr_list(int jmp_addr)
{
	int indNew;
	// 根据指令在代码节位置加4来计算下一个指令的位置。
	indNew = sec_text_opcode_offset + 4;
	// 如果发现溢出，使用新大小从新分配代码节。
	if (indNew > sec_text->data_allocated)
	{
		section_realloc(sec_text, indNew);
	}
	// 把前一跳转指令地址写入代码节位置。
	// 这个值在if和for的时候为零。而在break, continue, return的时候非零。
	*(int *)(sec_text->bufferSectionData + sec_text_opcode_offset) = jmp_addr;
	// 返回值为代码节位置。这句话其实没啥作用。
	jmp_addr = sec_text_opcode_offset;
	// 更新代码节位置。指向下一条指令。
	sec_text_opcode_offset = indNew;
	// 返回值为代码节位置。
	return jmp_addr;
}

/************************************************************************/
/* 功能：       回填函数，把t为链首的各个待定跳转地址填入相对地址       */
/* fill_offset：链首。我认为是需要填充的代码节偏移位置                  */
/* jmp_addr：   指令跳转位置。                                          */
/************************************************************************/
void jmpaddr_backstuff(int fill_offset, int jmp_addr)
{
	int next_addr, *ptrSecText;
	// 如果需要填充的代码节偏移位置不为零。
	while (fill_offset)
	{
		// ptr指向代码节偏移位置
		ptrSecText = (int *)(sec_text->bufferSectionData + fill_offset);
		// 记住下一个需要回填位置
		next_addr = *ptrSecText;
		// 填入相对地址
		*ptrSecText = jmp_addr - fill_offset - 4;
		// 准备处理下一个需要回填位置
		fill_offset = next_addr ;
	}
}

/************************************************************************/
/* *功能:            COFF初始化                                         */
/* *本函数用到全局变量:                                                 */
/* DynArray sections；        //节数组                                  */
/* Section*sec_text，         //代码节                                  */
/*         *sec_data，        //数据节                                  */
/*         *sec_bss，         //未初始化数据节                          */
/*         *sec_idata，       //导人表节                                */
/*         *sec_rdata，       //只读数据节                              */
/*         *sec_rel，         //重定位信息节                            */
/*         *sec_symtab，      //符号表节                                */
/*         *sec_dynsymtab；   //链接库符号节                            */
/* in tn sec_image；          //映像文件节个数                          */
/************************************************************************/
void init_all_sections_and_coffsyms()
{
	vecSection.reserve(1024);
	nsec_image = 0 ;
	// 创建代码节
	sec_text = section_new(".text",
			IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE);
	// 创建数据节
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
			
	sec_symtab = coffsym_section_new(".symtab",
			IMAGE_SCN_LNK_REMOVE | IMAGE_SCN_MEM_READ, ".strtab");  
	sec_dynsymtab = coffsym_section_new(".dynsym",
			IMAGE_SCN_LNK_REMOVE | IMAGE_SCN_MEM_READ, ".dynstr");

	coffsym_add(sec_symtab,    "",       0,                0, CST_NOTFUNC, IMAGE_SYM_CLASS_NULL);
	coffsym_add(sec_symtab,    ".data",  0,  sec_data->cSectionIndex, CST_NOTFUNC, IMAGE_SYM_CLASS_STATIC);
	coffsym_add(sec_symtab,    ".bss",   0,   sec_bss->cSectionIndex, CST_NOTFUNC, IMAGE_SYM_CLASS_STATIC);
	coffsym_add(sec_symtab,    ".rdata", 0, sec_rdata->cSectionIndex, CST_NOTFUNC, IMAGE_SYM_CLASS_STATIC);
	coffsym_add(sec_dynsymtab, "",       0,                0, CST_NOTFUNC, IMAGE_SYM_CLASS_NULL);
}

/************************************************************************/
/*  功能:            释放所有节数据                                     */
/************************************************************************/
void free_sections()
{
	int idx;
	Section * sec;
	for(idx = 0; idx < vecSection.size(); idx++)
	{
		sec = (Section *)vecSection[idx];
		if(sec->sym_hashtab != NULL)
			free(sec->sym_hashtab);
		free(sec->bufferSectionData);
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
/* 功能:          从当前读写位置到new_pos位置用0填补文件内容            */
/* fp:            文件指针                                              */
/* new_pos:       填补终点位置                                          */
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
/* 功能:            加载目标文件                                        */
/* file_name:       目标文件名                                          */
/* 启动参数为-lmsvcrt -o HelloWorld.exe HelloWorld.obj，会进入这个函数。*/
/************************************************************************/
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
	// 计算节头大小
	section_header_size = sizeof(IMAGE_SECTION_HEADER);
	memset(&file_header, 0x00, sizeof(IMAGE_FILE_HEADER));
	// 打开文件。
	file_obj = fopen(file_name, "rb");
	// 读COFF文件头。
	fread(&file_header, 1, sizeof(IMAGE_FILE_HEADER), file_obj);
	// 得到节头个数。
	nsec_obj = file_header.NumberOfSections;
	// 读出来每一个节。这里有一个技巧，就是Name是sh的第一个成员。
	// Name的地址和vecSection[i]->sh的地址相等。也就是：
	//     &(vecSection[i]->sh) == vecSection[i]->sh.Name
	for (i = 0; i < nsec_obj; i++)
	{
	//	IMAGE_SECTION_HEADER * testOne = &(vecSection[i]->sh);
	//	BYTE * testTwo = vecSection[i]->sh.Name;
		fread(vecSection[i]->sh.Name, 1, section_header_size, file_obj);
	}
	// 读取代码节偏移
	cur_text_offset = sec_text->data_offset;
	// 读取重定位信息节偏移
	cur_rel_offset  = sec_rel->data_offset;
	for (i = 0; i < nsec_obj; i++)
	{
		// COFF符号表存储在.symtab节中。
		//     例如在.symtab节头中，PointerToRawData=0x1D0，SizeOfRawData=0x90，
		//     即符号表内容是位于文件偏移位置0x1D0，连续的0x 0=144字节，
		//     符号个数 = 144 / sizeof(CoffSym) = 144/18 = 8，
		//     这8个符号是什么需要通过字符串表才能知道。
		if(!strcmp((char *)vecSection[i]->sh.Name, ".symtab"))
		{
			// 为COFF符号表分配空间。大小由SizeOfRawData指定。
			cfsyms = (CoffSym *)mallocz(vecSection[i]->sh.SizeOfRawData);
			// 根据PointerToRawData给出的文件偏移位置移动文件指针。
			fseek(file_obj, 1, vecSection[i]->sh.PointerToRawData);
			// 读出COFF符号表
			fread(cfsyms, 1, vecSection[i]->sh.SizeOfRawData, file_obj);
			// 得到COFF符号个数。这8个符号是什么需要通过字符串表才能知道。
			nSym = vecSection[i]->sh.SizeOfRawData / sizeof(CoffSym);
			continue;
		}
		// COFF字符串表
		else if(!strcmp((char *)vecSection[i]->sh.Name, ".strtab"))
		{
			// 为字符串表分配空间。
			strs = (char *)mallocz(vecSection[i]->sh.SizeOfRawData);
			// 读出所有的字符串、这里是：
			//     ..data..bss..rdata.main._entry.
			fseek(file_obj, 1, vecSection[i]->sh.PointerToRawData);
			fread(strs, 1, vecSection[i]->sh.SizeOfRawData, file_obj);
			continue;
		}
		// 不处理动态符号表和动态字符串表。
		else if(!strcmp((char *)vecSection[i]->sh.Name, ".dynsym") ||
			    !strcmp((char *)vecSection[i]->sh.Name, ".dynstr"))
		{
			continue;
		}
		// 代码执行到这里，说明不是符号表和字符串表。
		// 而是<.text> <.data> <.idata> <.rdata> <.bss> <.rel> 
		// 根据PointerToRawData给出的文件偏移位置移动文件指针。
		fseek(file_obj, SEEK_SET, vecSection[i]->sh.PointerToRawData);
		// 给节数据预留至少increment大小的内存空间
		ptr = section_ptr_add(vecSection[i], vecSection[i]->sh.SizeOfRawData);
		// 读出一个节。
		fread(ptr, 1, vecSection[i]->sh.SizeOfRawData, file_obj);
	}
	// 使用COFF符号个数分配指针空间。
	old_to_new_syms = (int *)mallocz(sizeof(int) * nSym);
	// 符号从一开始。因为第一个是符号对应的是空字符串。
	for (i = 1; i < nSym; i++)
	{
		// 取得一个符号。
		cfsym = &cfsyms[i];
		// Name字段表示符号名称在字符串表中的偏移位置。
		// cs_name指向字符串表中当前的字符串。
		cs_name = strs + cfsym->coffSymName;
		// 
		cfsym_index = coffsym_add(sec_symtab, cs_name,
			cfsym->coff_sym_value, cfsym->shortSectionNumber, 
			cfsym->typeFunc,       cfsym->storageClass);
		old_to_new_syms[i] = cfsym_index;
	}
	// 重定位信息节。
	rel     = (CoffReloc *)(sec_rel->bufferSectionData + cur_rel_offset);
	rel_end = (CoffReloc *)(sec_rel->bufferSectionData + sec_rel->data_offset);
	for (; rel < rel_end; rel++)
	{
		cfsym_index  = rel->cfsymIndex;
		rel->cfsymIndex   = old_to_new_syms[cfsym_index];
		rel->offsetAddr  += cur_rel_offset;
	}
	free(cfsyms);
	free(strs);
	free(old_to_new_syms);
	fclose(file_obj);
	return 1;
}

/************************************************************************/
/* 功能:            输出目标文件                                        */
/* file_name:       目标文件名                                          */
/* 启动参数为-o HelloWorld.obj -c HelloWorld.c，会进入这个函数。        */
/************************************************************************/
void output_obj_file(char * file_name)
{
	int idx, sh_size, nsec_obj = 0;
	IMAGE_FILE_HEADER * coff_file_header;
	int file_offset;
	// 打开文件。
	FILE * coff_obj_file = fopen(file_name, "wb");
	// We have  <.text> <.data> <.idata> <.rdata> <.bss> <.rel> 
	//    <.symtab> <.strtab> <.dynsym> <.dynstr>.
	// 计算节头个数。这里减去<.dynsym> <.dynstr>两个节。
	// 因为，映像文件中不包含COFF重定位信息。
	nsec_obj = vecSection.size() -2 ;
	sh_size  = sizeof(IMAGE_SECTION_HEADER);
	// 计算文件大小。为COFF文件头 + 节头大小 * 节头数。
	file_offset = sizeof(IMAGE_FILE_HEADER) + nsec_obj * sh_size;
	// 填补文件内容、给COFF文件头和节头表预留空间。
	fpad(coff_obj_file, file_offset);
	coff_file_header = (IMAGE_FILE_HEADER *)mallocz(sizeof(IMAGE_FILE_HEADER));
	// Write File Sections
	// 下面开始写入节数据
	for(idx = 0; idx < nsec_obj; idx++)
	{
		Section * sec = vecSection[idx];
		// 如果没有数据，就不处理。
		if(sec->bufferSectionData == NULL)
			continue;
		// 写入一个节的数据
		fwrite(sec->bufferSectionData, 1, sec->data_offset, coff_obj_file);
		// 计算指向COFF文件中节的第一个页面的文件指针。
		sec->sh.PointerToRawData = file_offset;
		// 计算磁盘文件中已初始化数据的大小。
		sec->sh.SizeOfRawData = sec->data_offset;
		file_offset += sec->data_offset;
	}
	// 文件指针回到开头，准备写COFF文件头和节头表。
	fseek(coff_obj_file, SEEK_SET, 0);
	// 机器类型为I386
	coff_file_header->Machine = IMAGE_FILE_MACHINE_I386;
	// 节的数目。
	coff_file_header->NumberOfSections = nsec_obj;
	// 符号表的文件偏移指向sec_symtab节的第一个页面。
	coff_file_header->PointerToSymbolTable = sec_symtab->sh.PointerToRawData;
	// 符号表中的元素数目
	coff_file_header->NumberOfSymbols = sec_symtab->sh.SizeOfRawData / sizeof(CoffSym);
	// 写COFF文件头
	fwrite(coff_file_header, 1, sizeof(IMAGE_FILE_HEADER), coff_obj_file);
	// 写节头表
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

