#ifndef COFF_GENERATOR_H
#define COFF_GENERATOR_H
#include <stdio.h>
#include <windows.h>


#define MAXKEY	1024				// 哈希表容量

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

/* 类型存储结构定义 */
typedef struct Type 
{
    int t;
    struct Symbol *ref;
} Type;

/* 符号存储结构定义 */
typedef struct Symbol 
{
    int v;						// 符号的单词编码
    int r;						// 符号关联的寄存器
    int c;						// 符号关联值
    Type type;					// 符号类型
    struct Symbol *next;		// 关联的其它符号，结构体定义关联成员变量符号，函数定义关联参数符号
    struct Symbol *prev_tok;	// 指向前一定义的同名符号
} Symbol;

/* 单词存储结构定义 */
typedef struct TkWord
{
    int  tkcode;					// 单词编码 
    struct TkWord *next;			// 指向哈希冲突的其它单词
    char *spelling;					// 单词字符串 
    struct Symbol *sym_struct;		// 指向单词所表示的结构定义
    struct Symbol *sym_identifier;	// 指向单词所表示的标识符
} TkWord;

/* 重定位结构定义 */
typedef struct CoffReloc 
{
    DWORD offset;	// 需要进行重定位的代码或数据的地址
    DWORD cfsym;				// 符号表的索引(从0开始)
	BYTE  section;  // 此处讲一下为什么对COFF重定位结构进行修改记录Section信息*/
    BYTE  type;    
} CoffReloc;

#endif