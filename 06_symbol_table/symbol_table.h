#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include <stdio.h>
#include <vector>
#pragma warning(disable : 4786)
#include "translation_unit.h"

/* 类型存储结构定义 */
typedef struct structType
{
    // e_TypeCode t;
	int    t;
    struct Symbol *ref;
} Type;

/* 符号存储结构定义 */
typedef struct Symbol 
{
    int v;						// 符号的单词编码 e_TokenCode
    int r;						// 符号关联的寄存器存储类型 e_StorageClass
    int c;						// 符号关联值。
	                            // 这个关联值据我观察，应该如果是变量，则会记录变量的值。
	                            // 如果是结构体，记录结构体的大小，如果是数组，记录数组元素个数。
								// 如果是静态字符串。则会被当成数组指针。此时这个的值为-1。
    Type type;					// 符号类型
    struct Symbol *next;		// 关联的其它符号，结构体定义关联成员变量符号，函数定义关联参数符号
    struct Symbol *prev_tok;	// 指向前一定义的同名符号
} Symbol;

/* 单词存储结构定义 */
/* 说一下如何理解 */
typedef struct TkWord
{
    int  tkcode;					// 单词编码 
    struct TkWord *next;			// 指向哈希冲突的其它单词
    char *spelling;					// 单词字符串 
    struct Symbol *sym_struct;		// 指向单词所表示的结构定义
    struct Symbol *sym_identifier;	// 指向单词所表示的标识符
} TkWord;

Symbol * struct_search(int v);
Symbol * sym_search(int v);
void sym_pop(std::vector<Symbol> * pop, Symbol *b);
Symbol * sym_push(int v, Type * type, int r, int c);

Symbol * sym_direct_push(std::vector<Symbol> &ss, int v, Type * type, int c);
Symbol * func_sym_push(int v, Type * type);

Symbol * var_sym_put(Type * type, int r, int v, int addr);
Symbol * sec_sym_put(char * sec, int c);

void print_all_stack(char* strPrompt);
void mk_pointer(Type *t);

void init();
#endif