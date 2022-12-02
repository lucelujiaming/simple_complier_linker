#include "opcode_generator.h"
#include "coff_generator.h"

extern Section *sec_text,	// 代码节
		*sec_data,			// 数据节
		*sec_bss,			// 未初始化数据节
		*sec_idata,			// 导入表节
		*sec_rdata,			// 只读数据节
		*sec_rel,			// 重定位信息节
		*sec_symtab,		// 符号表节	
		*sec_dynsymtab;		// 链接库符号节

extern int sec_text_opcode_ind ;	 	// 指令在代码节位置

// Operation generation functions
/************************************************************************/
/*  功能：向代码节写人一个字节                                          */
/*  write_byte：字节值                                                  */
/************************************************************************/
void gen_byte(char write_byte)
{
	int indNext;
	indNext = sec_text_opcode_ind + 1;
	// 如果发现代码节已经分配的空间不够。
	if (indNext > sec_text->data_allocated)
	{
		// 重新分配代码节空间。
		section_realloc(sec_text, indNext);
	}
	// 向代码节写人一个字节。
	sec_text->data[sec_text_opcode_ind] = write_byte;
	// 移动写入下标。
	sec_text_opcode_ind = indNext;
}

/************************************************************************/
/*  功能：生成指令前缀                                                  */
/*  opcode：指令前缀编码                                                */
/************************************************************************/
void gen_prefix(char opcode)
{
	gen_byte(opcode);
}

/************************************************************************/
/*   功能：生成单字节指令                                               */
/*   opcode：指令编码                                                   */
/************************************************************************/
void gen_opcodeOne(unsigned char opcode)
{
	gen_byte(opcode);
}

/************************************************************************/
/* 功能：生成双字节指令                                                 */
/* first：指令第一个字节                                                */
/* second：指令第二个字节                                               */
/************************************************************************/
void gen_opcodeTwo(unsigned char first, unsigned char second)
{
	gen_byte(first);
	gen_byte(second);
}

/************************************************************************/
/*  功能：生成4字节操作数                                               */
/*  c：4字节操作数                                                      */
/************************************************************************/
void gen_dword(unsigned int c)
{
	gen_byte(c);
	gen_byte(c >> 8);
	gen_byte(c >> 16);
	gen_byte(c >> 24);
}

/************************************************************************/
/* 功能：生成全局符号地址，并增加COFF重定位记录                         */
/* storage_class：符号存储类型                                                      */
/* sym：          符号指针                                                        */
/* value：        符号关联值                                                        */
/************************************************************************/
void gen_addr32(int storage_class, Symbol * sym, int value)
{
    // 如果是符号，增加COFF重定位记录
	if (storage_class & SC_SYM)
	{
  		coffreloc_add(sec_text, sym, 
			sec_text_opcode_ind, IMAGE_REL_I386_DIR32);
	}
	gen_dword(value);
}



