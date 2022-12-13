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

typedef struct Section_t{
	int data_offset;           //当前数据偏移位置     
	char * data;               //节数据
	int data_allocated;        //分配内存空间
	char index;                //节序号
	struct Section_t * link;   //关联的其他节，符号节关联字符串节
	int * hashtab;             //哈希表，只用于存储符号表
	IMAGE_SECTION_HEADER sh;   //节头
} Section;

/* COFF符号结构定义。含义参见表7.9 符号表记录成员变量说明 */
typedef struct CoffSym 
{
    DWORD name;					// 符号名称：字符串表中的一个偏移地址。
								//           这里对符号表记录格式做一个小修改。按照COFF文件规范，
								//           当符号名小于长度8时，将符号名直接存在ShortName中，
								//           当大于8时，Short字段为0，Long字段表示了符号名称在字符串表中的偏移位置。
								//           现在改为无论符号名称长度是否大于8，符号表名称都放在字符串表中，
								//           Name字段表示符号名称在字符串表中的偏移位置，
								//           新增加的Next字段用来保存哈希冲突链表。
	DWORD next;					// 用于保存冲突链表
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
    short   shortSection;		// 定义此符号的节索引：这个带符号整数是节表的索引(从1开始)，用以标识定义此符号的节
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
typedef struct CoffReloc 
{
    DWORD offset;	// 需要进行重定位的代码或数据的地址。
	                // 这是从节开头算起的偏移，加上节的RV A/Offset域的值，
    DWORD cfsym;	// 符号表的索引(从0开始)。这个符号给出了用于重定位的地址。
                    // 如果这个指定符号的存储类别为节，那么它的地址就是第一个与它同名的节的地址。
	BYTE  section;  // 需要重定位数据的节编号。
    BYTE  type;     // 重定位类型。合法的重定位类型依赖于机器类型，
	                // 请参考表7.13“重定位类型指示符取值”
} CoffReloc;

// 参见7.1.4节。例如， 如果节的第一个字节的地址是0xl0，那么第三个字节的地址就是0x12





void * section_ptr_add(Section * sec, int increment);
void section_realloc(Section * sec, int new_size);
void coffreloc_add(Section * sec, Symbol * sym, int offset, char type);
int coffsym_add(Section * symtab, char * name, int val, int sec_index,
		WORD typeFunc, char StorageClass);
		// short typeFunc, char StorageClass);
void coffsym_add_update(Symbol *sym, int val, int sec_index,
		WORD typeFunc, char StorageClass);
		// short typeFunc, char StorageClass);

void init_coff();
int load_objfile(char * file_name);
void output_objfile(char * name);
void free_sections();

int  make_jmpaddr_list(int jmp_addr);
void jmpaddr_backstuff(int fill_offset, int jmp_addr);

void *mallocz(int size);

int coffsym_search(Section * symtab, char * name);
char * coffstr_add(Section * strtab, char * name);
void coffreloc_redirect_add(int offset, int cfsym, char section, char type);

void fpad(FILE * fp, int new_pos);
#endif