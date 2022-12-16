#ifndef COFF_GENERATOR_H
#define COFF_GENERATOR_H
#include <stdio.h>
#include <windows.h>

#include "get_token.h"

#define MAXKEY	1024				// 哈希表容量

enum e_Sec_Storage{
	SEC_BSS_STORAGE   = 0,
	SEC_DATA_STORAGE  = 1,
	SEC_RDATA_STORAGE = 2
};

// 写在前面。
// 这个结构体的设计是基于这样的一个前提。
// 就是对于不同的节来说，data中保存的内容也不同。例如：
//     对于代码节来说，  就是一个字节一个字节的机器码。在编译生成代码的时候，通过gen_byte写入。
//     对于数据节来说，  就是一个字节一个字节的数据。在initializer和init_variable中写入。
//     对于符号表节来说，就是一个一个的CoffSym结构体。我们在编译过程中，
//                       每当遇到一个函数或者是一个全局变量的时候，我们就需要增加或更新COFF符号。
//     对于链接库符号节来说，也是一个一个的CoffSym结构体。
//     对于字符串节来说，就是一个一个的字符串。
//     对于重定位信息节来说，就是一个一个的CoffReloc结构体。
// 因此上，我们需要记录缓冲区写入位置，缓冲区，缓冲区大小。
typedef struct Section_t{
    int data_offset;                  // 当前数据偏移位置。也就是缓冲区写入位置。
    char * bufferSectionData;         // 节数据。也就是缓冲区
    int data_allocated;               // 分配内存空间。也就是缓冲区大小。
    char cSectionIndex;               // 节序号。这个有几个用处。
	                                  // 一个是在符号表节中，表示这个符号来自于哪个节。
	                                  // 还有是在重定位信息节中，标识需要重定位数据的节编号。
    struct Section_t * linkSection;   // 关联的其他节，符号节关联字符串节
    int * sym_hashtab;                // 哈希表，只用于存储符号表。
		  // 因为对于符号表节来说，保存的是一个一个的CoffSym结构体，也就是一个COFF符号。
		  // 当符号很多的时候，为了快速找到对应的CoffSym结构体，我们设计了这个哈希表。
		  // 以便于在coffsym_search中根据名字的哈希值快速找到对应的CoffSym结构体。
		  // 这个哈希表里每一个元素保存的是CoffSym类型数组下标。
		  // 例如：sym_hashtab[422] = 5 表明符号表中的第5个COFF符号(CoffSym结构体)
		  // 的名字"printf"的哈希值为422。
		  // 这样，当我们查找"printf"这个符号的时候，我们就可以先对"printf"进行哈希，
		  // 得到这个字符串的哈希值为422，之后直接访问sym_hashtab[422]，得到5。
		  // 我们就可以知道"printf"这个符号是符号表节中的第5个。
		  // 但是我们会遇到哈希冲突的情况。这就是CoffSym结构体中nextCoffSymName成员的作用。
    IMAGE_SECTION_HEADER sh;          // 节头
} Section;

// COFF符号结构定义。含义参见表7.9 符号表记录成员变量说明
// 对于符号表节来说，里面存放的就是一个个的CoffSym结构体。
// 这个的定义参考了IMAGE_SYMBOL的定义。
typedef struct CoffSym
{
    DWORD coffSymName;          // 符号名称：字符串表中的一个偏移地址。
								//           这里对符号表记录格式做一个小修改。按照COFF文件规范，
								//           当符号名小于长度8时，将符号名直接存在ShortName中，
								//           当大于8时，Short字段为0，Long字段表示了符号名称在字符串表中的偏移位置。
								//           现在改为无论符号名称长度是否大于8，符号表名称都放在字符串表中，
								//           name字段表示符号名称在字符串表中的偏移位置，
								//           新增加的Next字段用来保存哈希冲突链表。
	DWORD nextCoffSymName;		// 用于保存冲突链表
    /*
    struct {
		DWORD   Short;			// if 0, use LongName
        DWORD   Long;			// offset into string table
    } name;
	*/
    DWORD   coff_sym_value;		// 与符号相关的值：这个值使用coffsym_add_update设置。
                                //                 对于全局变量表示符号地址、
                                //                 对于函数表示当前指令在代码节位置
								//                 其意义依赖于Section Number和Storage Class这两个域，
								//                 它通常表示可重定位的地址。
    short   shortSectionNumber;	// 定义此符号的节索引：这个带符号整数是节表的索引(从1开始)，用以标识定义此符号的节
								//                     小于1的值有特殊的含义，详细信息请参考表7.10“Section Number域的值”
    WORD    typeFunc;			// COFF符号类型：一个表示类型的数字。
	                            //               这个字的低字节定义了这个符号是一个指针，函数返回还是数组基地址。
	                            //               Microsoft工具仅使用此字段来指示符号是否为函数，
								//               所以，只有两个结果值：0x0和0x20。
								//               但是，其他工具可以使用此字段来传达更多信息。
	                            //               例如，Visual C++ 调试信息用于指示类型。
								//               正确指定函数属性非常重要。 增量链接需要此信息才能正常工作。
	                            //               对于某些体系结构，可能需要这些信息才能用于其他目的。
								// 参见：https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
    BYTE    storageClass;		// COFF符号存储类别：这是一个表示存储类别的枚举类型值。
	                            //                    要获取更多信息，请参考表7.11“存储类别取值”
    BYTE    numberOfAuxSymbols;	// 跟在本记录后面的辅助符号表项的个数
} CoffSym;

#define CST_FUNC    0x20  //Coff符号类型，函数
#define CST_NOTFUNC 0     //Coff符号类型，非函数

/* 重定位结构定义 */
/************************************************************************/
/* COFF重定位信息：                                                     */
/* 目标文件中包含COFF重定位信息，它用来指出当节中的数据被放进映像文件   */
/* 以及将来被加载进内存时应如何修改这些数据。                           */
/* 映像文件中没有COFF重定位信息，这是因为所有被引用的符号都已经被赋予了 */
/* 一个在平坦内存空间中的地址。映像文件中包含的重定位信息是位于.reloc节 */
/* 中的基址重定位信息(除非映像有IMAGE_FILE_RELOCS_STRIPPED属性)。       */
/* 对于目标文件中的每个节，都有一个由长度固定的记录组成的数组来保存     */
/* 此节的COFF重定位信息。此数组的位置和长度在节头中指定。               */
/* 数组的每个元素定义在IMAGE_RELOCATION中。简单可以理解为：             */
/*		typedef struct _IMAGE_RELOCATION {                              */
/*			unsigned long ulAddr;    // 定位偏移                        */
/*			unsigned long ulSymbol;  // 符号                            */
/*			unsigned shortusType;    // 定位类型                        */
/*		} IMAGE_RELOCATION;                                             */
/*                                                                      */
/* 一共三个成员！                                                       */
/*     ulAddr是要定位的内容在段内偏移。比如：                           */
/*        一个正文段，起始位置为0x010，ulAddr的值为0x05，那你的         */
/*        定位信息就要写在0x15处。而且信息的长度要看你的代码的类型，    */
/*        32位的代码要写4个字节，16位的就只要字2个字节。                */
/*     ulSymbol是符号索引，它指向符号表中的一个记录。是符号表中的       */
/*        一个记录的记录号。这个成员指明了重定位信息所对映的符号。      */
/*     usType是重定位类型的标识。32位代码中，                           */
/*        通常只用两种定位方式。一是绝对定位，二是相对定位。            */
/************************************************************************/
typedef struct CoffReloc
{
    DWORD offsetAddr;	// 需要进行重定位的代码或数据的地址。
	                    // 这是从节开头算起的偏移，加上节的RVA/Offset域的值，
    DWORD cfsymIndex;	// 符号表的索引(从0开始)。这个符号给出了用于重定位的地址。
                        // 如果这个指定符号的存储类别为节，
					    // 那么它的地址就是第一个与它同名的节的地址。
	// 下面对重定位记录格式做一个小修改，
	// 将Type这个成员变量的两个字节拆分为两个变量：
	// 一个字节保存需要重定位数据的节编号；另一个字节存储重定位类型。
	BYTE  sectionIndex; // 需要重定位数据的节编号。
    BYTE  typeReloc;    // 重定位类型。合法的重定位类型依赖于机器类型，
	                    // 请参考表7.13“重定位类型指示符取值”
					    // 基本上只有两种情况。
					    //     IMAGE_REL_I386_DIR32 - 32位绝对定位。
					    //     IMAGE_REL_I386_REL32 - 32位相对定位。
} CoffReloc;
// 参见7.1.4节。例如， 如果节的第一个字节的地址是0xl0，那么第三个字节的地址就是0x12


void *mallocz(int size);

// 节空间分配和初始化函数。
void init_all_sections_and_coffsyms();
// 节空间释放函数。
void free_sections();
// 节数据空间处理函数
void * section_ptr_add(Section * sec, int increment);
void section_realloc(Section * sec, int new_size);

// 符号表节和动态符号表节操作函数。
int coffsym_add(Section * symtab, char * coffsym_name, int val, int sec_index,
		WORD typeFunc, char StorageClass);
		// short typeFunc, char StorageClass);
void coffsym_add_update(Symbol *sym, int val, int sec_index,
		WORD typeFunc, char StorageClass);
		// short typeFunc, char StorageClass);
int coffsym_search(Section * symtab, char * coffsym_name);

// 字符串节操作函数。
char * coffstr_add(Section * strtab, char * coffstr_name);
// 重定位节操作函数。
void coffreloc_add(Section * sec, Symbol * sym, int offset, char type_reloc);
// 重定位信息节操作函数。
void coffreloc_redirect_add(int offset, int cfsym_index, char section_index, char type_reloc);


// 代码节地址空间处理函数。
int  make_jmpaddr_list(int jmp_addr);
void jmpaddr_backstuff(int fill_offset, int jmp_addr);

// COFF持久化函数。
int load_obj_file(char * file_name);
void output_obj_file(char * file_name);
void fpad(FILE * fp, int new_pos);

#endif
