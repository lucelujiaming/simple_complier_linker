#ifndef COFF_GENERATOR_H
#define COFF_GENERATOR_H
#include <stdio.h>
#include <windows.h>

#include "get_token.h"

#define MAXKEY	1024				// 哈希表容量

enum{
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

/* COFF符号结构定义 */
typedef struct CoffSym 
{

    DWORD name;					// 符号名称
	DWORD next;					// 用于保存冲突链表*/
    /* 
    struct {
		DWORD   Short;			// if 0, use LongName
        DWORD   Long;			// offset into string table
    } name;
	*/
    DWORD   value;				// 与符号相关的值
    short   shortSection;		// 节表的索引(从1开始),用以标识定义此符号的节*/
    WORD    type;				// 一个表示类型的数字
    BYTE    storageClass;		// 这是一个表示存储类别的枚举类型值
    BYTE    numberOfAuxSymbols;	// 跟在本记录后面的辅助符号表项的个数
} CoffSym;

/* 重定位结构定义 */
typedef struct CoffReloc 
{
    DWORD offset;	// 需要进行重定位的代码或数据的地址
    DWORD cfsym;				// 符号表的索引(从0开始)
	BYTE  section;  // 此处讲一下为什么对COFF重定位结构进行修改记录Section信息*/
    BYTE  type;    
} CoffReloc;

void section_realloc(Section * sec, int new_size);
void coffreloc_add(Section * sec, Symbol * sym, int offset, char type);
void coffsym_add_update(Symbol *s, int val, int sec_index,
		short type, char StorageClass);

void init_coff();
void write_obj(char * name);
void free_sections();

#endif