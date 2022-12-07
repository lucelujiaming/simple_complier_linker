#ifndef X86_GENERATOR_H
#define X86_GENERATOR_H

#include "operand_stack.h"
#include "opcode_generator.h"


extern std::vector<Operand> operand_stack;
extern std::vector<Operand>::iterator operand_stack_top ;
extern std::vector<Operand>::iterator operand_stack_last_top ;

extern std::vector<Section> vecSection;
extern  Section *sec_text,			// �����
		*sec_data,			// ���ݽ�
		*sec_bss,			// δ��ʼ�����ݽ�
		*sec_idata,			// ������
		*sec_rdata,			// ֻ�����ݽ�
		*sec_rel,			// �ض�λ��Ϣ��
		*sec_symtab,		// ���ű��	
		*sec_dynsymtab;		// ���ӿ���Ž�



void gen_modrm(int mod, int reg_opcode, int r_m, Symbol * sym, int value);
void load(int r, Operand * opd);
void store(int r, Operand * opd);
void gen_opInteger(int op);
void gen_opTwoInteger(int opc, int op);
void gen_addsp(int val);
void gen_invoke(int nb_args);
void gen_call();

#endif
