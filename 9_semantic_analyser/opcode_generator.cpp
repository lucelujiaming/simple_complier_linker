#include "opcode_generator.h"
#include "coff_generator.h"

extern Section *sec_text,	// �����
		*sec_data,			// ���ݽ�
		*sec_bss,			// δ��ʼ�����ݽ�
		*sec_idata,			// ������
		*sec_rdata,			// ֻ�����ݽ�
		*sec_rel,			// �ض�λ��Ϣ��
		*sec_symtab,		// ���ű��	
		*sec_dynsymtab;		// ���ӿ���Ž�

extern int sec_text_opcode_ind ;	 	// ָ���ڴ����λ��

// Operation generation functions
/************************************************************************/
/*  ���ܣ�������д��һ���ֽ�                                          */
/*  write_byte���ֽ�ֵ                                                  */
/************************************************************************/
void gen_byte(char write_byte)
{
	int indNext;
	indNext = sec_text_opcode_ind + 1;
	// ������ִ�����Ѿ�����Ŀռ䲻����
	if (indNext > sec_text->data_allocated)
	{
		// ���·������ڿռ䡣
		section_realloc(sec_text, indNext);
	}
	// ������д��һ���ֽڡ�
	sec_text->data[sec_text_opcode_ind] = write_byte;
	// �ƶ�д���±ꡣ
	sec_text_opcode_ind = indNext;
}

/************************************************************************/
/*  ���ܣ�����ָ��ǰ׺                                                  */
/*  opcode��ָ��ǰ׺����                                                */
/************************************************************************/
void gen_prefix(char opcode)
{
	gen_byte(opcode);
}

/************************************************************************/
/*   ���ܣ����ɵ��ֽ�ָ��                                               */
/*   opcode��ָ�����                                                   */
/************************************************************************/
void gen_opcodeOne(unsigned char opcode)
{
	gen_byte(opcode);
}

/************************************************************************/
/* ���ܣ�����˫�ֽ�ָ��                                                 */
/* first��ָ���һ���ֽ�                                                */
/* second��ָ��ڶ����ֽ�                                               */
/************************************************************************/
void gen_opcodeTwo(unsigned char first, unsigned char second)
{
	gen_byte(first);
	gen_byte(second);
}

/************************************************************************/
/*  ���ܣ�����4�ֽڲ�����                                               */
/*  c��4�ֽڲ�����                                                      */
/************************************************************************/
void gen_dword(unsigned int c)
{
	gen_byte(c);
	gen_byte(c >> 8);
	gen_byte(c >> 16);
	gen_byte(c >> 24);
}

/************************************************************************/
/* ���ܣ�����ȫ�ַ��ŵ�ַ��������COFF�ض�λ��¼                         */
/* storage_class�����Ŵ洢����                                                      */
/* sym��          ����ָ��                                                        */
/* value��        ���Ź���ֵ                                                        */
/************************************************************************/
void gen_addr32(int storage_class, Symbol * sym, int value)
{
    // ����Ƿ��ţ�����COFF�ض�λ��¼
	if (storage_class & SC_SYM)
	{
  		coffreloc_add(sec_text, sym, 
			sec_text_opcode_ind, IMAGE_REL_I386_DIR32);
	}
	gen_dword(value);
}



