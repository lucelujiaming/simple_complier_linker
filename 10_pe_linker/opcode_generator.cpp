#include "opcode_generator.h"
#include "coff_generator.h"

extern Section *sec_text;	// 代码节

extern int sec_text_opcode_offset ;	 	// 指令在代码节位置

// Operation generation functions
/************************************************************************/
/*  功能：向代码节写人一个字节                                          */
/*  write_byte：字节值                                                  */
/************************************************************************/
void gen_byte(char write_byte)
{
	int indNext;
	indNext = sec_text_opcode_offset + 1;
	// 如果发现代码节已经分配的空间不够。
	if (indNext > sec_text->data_allocated)
	{
		// 重新分配代码节空间。
		section_realloc(sec_text, indNext);
	}
	// 向代码节写人一个字节。
	sec_text->bufferSectionData[sec_text_opcode_offset] = write_byte;
	// 移动写入下标。
	sec_text_opcode_offset = indNext;
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
/*  功能： 生成4字节操作数                                              */
/*  value：4字节操作数                                                  */
/************************************************************************/
void gen_dword(unsigned int value)
{
	gen_byte(value);
	gen_byte(value >> 8);
	gen_byte(value >> 16);
	gen_byte(value >> 24);
}

/************************************************************************/
/* 功能：生成全局符号地址，并增加COFF重定位记录                         */
/* storage_class：符号存储类型                                          */
/* sym：          符号指针                                              */
/* value：        符号关联值                                            */
/************************************************************************/
void gen_addr32(int storage_class, Symbol * sym, int value)
{
    // 如果是符号，增加COFF重定位记录
	if (storage_class & SC_SYM)
	{
  		coffreloc_add(sec_text, sym, 
			sec_text_opcode_offset, IMAGE_REL_I386_DIR32);
	}
	gen_dword(value);
}



