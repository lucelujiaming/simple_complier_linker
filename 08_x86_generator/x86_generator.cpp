// x86_generator.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "token_code.h"
#include "symbol_table.h"
#include <windows.h>

#define OPERAND_STACK_SIZE    1024

int loc = 0 ; 
int ind = 0 ;  // It should be the written offset of Section .
std::vector<Operand> operand_stack;
std::vector<Operand>::iterator operand_stack_top = NULL;
std::vector<Operand>::iterator operand_stack_second = NULL;

extern std::vector<Section> vecSection;
extern  Section *sec_text,			// �����
		*sec_data,			// ���ݽ�
		*sec_bss,			// δ��ʼ�����ݽ�
		*sec_idata,			// ������
		*sec_rdata,			// ֻ�����ݽ�
		*sec_rel,			// �ض�λ��Ϣ��
		*sec_symtab,		// ���ű��	
		*sec_dynsymtab;		// ���ӿ���Ž�

Type char_pointer_type,		// �ַ���ָ��				
	 int_type,				// int����
	 default_func_type;		// ȱʡ��������
// Operand functions
void gen_opInteger(int op);
void gen_opTwoInteger(int opc, int op);
int pointed_size(Type * type);
void spill_reg(char c);
void spill_regs();
void gen_call();
/************************************************************************/
/*  ���ܣ���������ջ                                                    */
/*  type����������������                                                */
/*  r���������洢����                                                   */
/*  value��������ֵ                                                     */
/************************************************************************/
void operand_push(Type* type, int r, int value)
{
	Operand operand;
	if (operand_stack.size() > OPERAND_STACK_SIZE)
	{
		printf("No memory.\n");
	}

	operand.type = *type;
	operand.r    = r;
	operand.value = value;
	operand_stack.push_back(operand);

	operand_stack_top = operand_stack.end();
	if (operand_stack.size() >= 2)
		operand_stack_second = operand_stack_top - 1;
	else
		operand_stack_second = NULL;
}

/************************************************************************/
/* ���ܣ�����ջ��������                                                 */
/************************************************************************/
void operand_pop()
{
	operand_stack.pop_back();
	
	operand_stack_top = operand_stack.end();
	if (operand_stack.size() >= 2)
		operand_stack_second = operand_stack_top - 1;
	else
		operand_stack_second = NULL;
}

/************************************************************************/
/*  ���ܣ�����ջ������������˳��                                        */
/************************************************************************/
void operand_swap()
{
	Operand tmp ;
	if (operand_stack.size() >= 2)
	{
		tmp = *operand_stack_top;
		*operand_stack_top = *operand_stack_second;
		*operand_stack_second = tmp;
	}
}

/************************************************************************/
/*  ���ܣ���������ֵ                                                    */
/*  t����������������                                                   */
/*  r���������洢����                                                   */
/*  value��������ֵ                                                     */
/************************************************************************/
void operand_assign(Operand * opd, int t, int r, int value)
{
	opd->type.t = t;
	opd->r = r;
	opd->value = value;
}

// Operation generation functions

void gen_dword(unsigned int c);
void gen_addr32(int r, Symbol * sym, int c);
/************************************************************************/
/*  ���ܣ�������д��һ���ֽ�                                          */
/*  c���ֽ�ֵ                                                           */
/************************************************************************/
void gen_byte(char c)
{
	int indNext;
	indNext = ind + 1;
	if (indNext > sec_text->data_allocated)
	{
		section_realloc(sec_text, indNext);
	}
	sec_text->data[ind] = c;
	ind = indNext;
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
/* ���ܣ�����ָ��Ѱַ��ʽ�ֽ�Mod R/M                                    */
/* mod��Mod R/M[7��6]                                                   */
/* reg_opcode��ModR/M[5��3]ָ�������3λ������Դ������(�з���׼ȷ)      */
/* r_m��Mod R/M[2��0] Ŀ�������(�з���׼ȷ)                            */
/* sym������ָ��                                                        */
/* c�����Ź���ֵ                                                        */
/************************************************************************/
void gen_modrm(int mod, int reg_opcode, int r_m, Symbol * sym, int c)
{
	mod <<= 6;
	reg_opcode <<= 3;
	if (mod == 0xc0)
	{
		gen_byte(mod | reg_opcode | (r_m & SC_VALMASK));
	}
	else if ((r_m & SC_VALMASK) == SC_GLOBAL)
	{
		gen_byte(0x05 | reg_opcode);
		gen_addr32(r_m, sym, c);
	}
	else if ((r_m & SC_VALMASK) == SC_LOCAL)
	{
		if (c == c)
		{
			gen_byte(0x45 | reg_opcode);
			gen_byte(c);
		}
		else
		{
			gen_byte(0x85 | reg_opcode);
			gen_dword(c);
		}
	}
	else 
	{
		gen_byte(0x00 | reg_opcode | (r_m & SC_VALMASK));
	}
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
/* r�����Ŵ洢����                                                      */
/* sym������ָ��                                                        */
/* c�����Ź���ֵ                                                        */
/************************************************************************/
void gen_addr32(int r, Symbol * sym, int c)
{
	if (r & SC_SYM)
	{
  		coffreloc_add(sec_text, sym, ind, IMAGE_REL_I386_DIR32);
	}
	gen_dword(c);
}

/************************************************************************/
/*  ���ܣ���������opd���ص��Ĵ���r��                                    */
/*  r�����Ŵ洢����                                                     */
/*  opd��������ָ��                                                     */
/************************************************************************/
void load(int r, Operand * opd)
{
	int v, ft, fc, fr;
	ft = opd->r;
	fc = opd->type.t;
	fr = opd->value;

	v = fr & SC_VALMASK;
	if (fr & SC_LVAL)    // ��ֵ
	{
		if ((ft & T_BTYPE) == T_CHAR)
		{
			gen_opcodeTwo(0x0f, 0xbe); // 0F BE -> movsx r32, r/m8
		}
		else if ((ft & T_BTYPE) == T_SHORT)
		{
			gen_opcodeTwo(0x0f, 0xbf); // 0F BF -> movsx r32, r/m16
		}
		else 
		{
			gen_opcodeOne(0x8b); // 8B -> mov r32, r/m32
		}
		gen_modrm(ADDR_OTHER, r, fr, opd->sym, fc);
	}
	else    // ��ֵ
	{
		if (v == SC_GLOBAL)
		{
			gen_opcodeOne(0xb8 + r);
			gen_addr32(fr, opd->sym, fc);
		}
		else if (v == SC_LOCAL)
		{
			gen_opcodeOne(0x8d);
			gen_modrm(ADDR_OTHER, r, SC_LOCAL, opd->sym, fc);
		}
		else if (v == SC_CMP)
		{
			gen_opcodeOne(0xb8 + r);
			gen_dword(0);
			gen_opcodeTwo(0x0f, fc = 16);
			gen_modrm(ADDR_REG, 0, r, NULL, 0);
		}
		else if (v != r)
		{
			gen_opcodeOne(0x89);
			gen_modrm(ADDR_REG, v, r, NULL, 0);
		}
	}
}

/************************************************************************/
/* ���ܣ����Ĵ���'r'�е�ֵ���������'opd'                               */
/* r�����Ŵ洢����                                                      */
/* opd��������ָ��                                                      */
/************************************************************************/
void store(int r, Operand * opd)
{
	int fr, bt;
	fr = opd->r & SC_VALMASK;
	bt = opd->type.t & T_BTYPE;

	if (bt == T_SHORT)
	{
		gen_prefix(0x66);
	}

	if (bt == T_CHAR)
	{
		gen_opcodeOne(0x88);
	}
	else
	{
		gen_opcodeOne(0x89);
	}

	if (fr == SC_GLOBAL || fr == SC_LOCAL || (opd->r & SC_LVAL))
	{
		gen_modrm(ADDR_OTHER, r, opd->r, opd->sym, opd->value);
	}

}

/************************************************************************/
/*  ���ܣ���ջ�����������ص�'rc'��Ĵ�����                              */
/*  rc���Ĵ�������                                                      */
/*  opd��������ָ��                                                     */
/************************************************************************/
int load_one(int rc, Operand * opd)
{
	int r ;
	r = opd->r & SC_VALMASK;
	if (r > SC_GLOBAL || (opd->r & SC_LVAL))
	{
	//	r = allocate_reg(rc);
		load(r, opd);
	}
	opd->r = r;
	return r;
}


/******************************************************************************/
/* ���ܣ���ջ�����������ص�'rc1'��Ĵ���������ջ�����������ص�'rc2'��Ĵ���   */
/* rcl��ջ�����������ص��ļĴ�������                                          */
/* rc2����ջ�����������ص��ļĴ�������                                        */
/******************************************************************************/
void load_two(int rc1, int rc2)
{
	load_one(rc2, operand_stack_top);
	load_one(rc2, operand_stack_second);
}

/************************************************************************/
/* ���ܣ���ջ�����������˴�ջ����������                                 */
/************************************************************************/
void store_one()
{
	int r = 0,t = 0;
	r = load_one(REG_ANY, operand_stack_top);
	if ((operand_stack_second->r & SC_VALMASK) == SC_LLOCAL)
	{
		Operand opd;
	//	t = allocate_reg(REG_ANY);
		operand_assign(&opd, T_INT, SC_LOCAL | SC_LVAL, 
			operand_stack_second->value);
		load(t, &opd);
		operand_stack_second->r = t | SC_LVAL;
	}
	store(r, operand_stack_second);
	operand_swap();
	operand_pop();
}

/***********************************************************
 * ����:	����t��ָ�����������
 * t:		ָ������
 **********************************************************/
Type *pointed_type(Type *t)
{
    return &t->ref->type;
}

/***********************************************************
 * ����:	����t��ָ����������ͳߴ�
 * t:		ָ������
 **********************************************************/
int pointed_size(Type *t)
{
    int align;
    return type_size(pointed_type(t), &align);
}

/************************************************************************/
/* ���ܣ����ɶ�Ԫ���㣬��ָ�����������һЩ���⴦��                     */
/* op�����������                                                       */
/************************************************************************/
void gen_op(int op)
{
	int u, btOne, btTwo;
	Type typeOne;

	btOne = operand_stack_second->type.t & T_BTYPE;
	btTwo = operand_stack_top->type.t & T_BTYPE;

	if (btOne == T_PTR || btTwo == T_PTR)
	{
		if (op >= TK_EQ && op <= TK_GEQ)   // >,<,>=.<=...
		{
			gen_opInteger(op);
			operand_stack_top->type.t = T_INT;
		}
		else if (btOne == T_PTR && btTwo == T_PTR)
		{
			if (op != TK_MINUS)
			{
				printf("Only support - and >,<,>=.<= \n");
			}
			u = pointed_size(&operand_stack_second->type);
			gen_opInteger(op);
			operand_stack_top->type.t = T_INT;
			operand_push(&int_type, SC_GLOBAL, 
				pointed_size(&operand_stack_second->type));
			gen_op(TK_DIVIDE);
		}
		else 
		{
			if (op != TK_MINUS && op != TK_PLUS)
			{
				printf("Only support - and >,<,>=.<= \n");
			}
			if (btTwo == T_PTR)
			{
				operand_swap();
			}
			typeOne = operand_stack_second->type;
			operand_push(&int_type, SC_GLOBAL, 
				pointed_size(&operand_stack_second->type));
			gen_op(TK_STAR);
			gen_opInteger(op);
			operand_stack_top->type = typeOne;
		}
	}
	else
	{
		gen_opInteger(op);
		if (op >= TK_EQ && op <= TK_GEQ)   // >,<,>=.<=...
		{
			operand_stack_top->type.t = T_INT;
		}

	}
}

/************************************************************************/
/* ���ܣ�������������                                                   */
/* OP�����������                                                       */
/************************************************************************/
void gen_opInteger(int op)
{
	int r, fr, opc;
	switch(op) {
	case TK_PLUS:
		opc = 0;
		gen_opTwoInteger(opc, op);
		break;
	case TK_MINUS:
		opc = 5;
		gen_opTwoInteger(opc, op);
		break;
	case TK_STAR:
		load_two(REG_ANY, REG_ANY);
		r  = operand_stack_second->r;
		fr = operand_stack_top->r;
		operand_pop();
		gen_opcodeTwo(0x0f, 0xaf);
		gen_modrm(ADDR_REG, r, fr, NULL, 0);
		break;
	case TK_DIVIDE:
	case TK_MOD:
		opc = 7;
		load_two(REG_EAX, REG_ECX);
		r  = operand_stack_second->r;
		fr = operand_stack_top->r;
		operand_pop();
		spill_reg(REG_EDX);
		gen_opcodeOne(0x99);
		gen_opcodeOne(0xf7);
		gen_modrm(ADDR_REG, opc, fr, NULL, 0);
		if (op == TK_MOD)
		{
			r = REG_EDX;
		}
		else
		{
			r = REG_EAX;
		}
		operand_stack_top->r = r;
		break;
	default:
		opc = 7;
		gen_opTwoInteger(opc, op);
		break;
	}
}

/************************************************************************/
/* ���ܣ�����������Ԫ����                                               */
/* opc��ModR/M[5��3]                                                    */
/* op�����������                                                       */
/************************************************************************/
void gen_opTwoInteger(int opc, int op)
{
	int r, fr, c;
	if ((operand_stack_top->r & (SC_VALMASK | SC_LOCAL | SC_SYM)) == SC_GLOBAL)
	{
		r = load_one(REG_ANY, operand_stack_second);
		c = operand_stack_top->value;
		if (c == (char)c)
		{
			gen_opcodeOne(0x83);
			gen_modrm(ADDR_REG, opc, r, NULL, 0);
			gen_byte(c);
		}
		else
		{
			gen_opcodeOne(0x81);
			gen_modrm(ADDR_REG, opc, r, NULL, 0);
			gen_byte(c);
		}
	}
	else
	{
		load_two(REG_ANY, REG_ANY);
		r  = operand_stack_second->r;
		fr = operand_stack_top->r;
		gen_opcodeOne((opc << 3) | 0x01);
		gen_modrm(ADDR_REG, fr, r, NULL, 0);
	}
	operand_pop();
	if (op >= TK_EQ && op <= TK_GEQ)   // >,<,>=.<=...
	{
		operand_stack_top->r = SC_CMP;
		switch(op) {
		case TK_EQ:
			operand_stack_top->value = 0x84;
			break;
		case TK_NEQ:
			operand_stack_top->value = 0x85;
			break;
		case TK_LT:
			operand_stack_top->value = 0x8c;
			break;
		case TK_LEQ:
			operand_stack_top->value = 0x8e;
			break;
		case TK_GT:
			operand_stack_top->value = 0x8f;
			break;
		case TK_GEQ:
			operand_stack_top->value = 0x8d;
			break;
		}
	}
}

/************************************************************************/
/* ���ܣ���¼������ת��ַ��ָ����                                       */
/* s��ǰһ��תָ���ַ                                                  */
/************************************************************************/
int makelist(int s)
{
	int indOne;
	indOne = ind + 4;
	if (indOne > sec_text->data_allocated)
	{
		section_realloc(sec_text, indOne);
	}
	*(int *)(sec_text->data + ind) = s;
	s = ind;
	ind = indOne;
	return s;
}

/************************************************************************/
/* ���ܣ����������tΪ���׵ĸ���������ת��ַ������Ե�ַ              */
/* t������                                                              */
/* a��ָ����תλ��                                                      */
/************************************************************************/
void backpatch(int t, int a)
{
	int n, *ptr;
	while (t)
	{
		ptr = (int *)(sec_text->data + t);
		n = *ptr;
		*ptr = a - t - 4;
		t = n ;
	}
}

/************************************************************************/
/* ���ܣ�������ߵ�ַ��תָ���ת��ַ����                             */
/* t��ǰһ��תָ���ַ                                                  */
/* makelist������������ĺ���                                           */
/************************************************************************/
int gen_jmpforward(int t)
{
	gen_opcodeOne(0xe9);
	return makelist(t);
}

/************************************************************************/
/* ���ܣ�������͵�ַ��תָ���ת��ַ��ȷ��                           */
/* a����ת����Ŀ���ַ                                                  */
/************************************************************************/
void gen_jmpbackward(int a)
{
	int r;
	r = a - ind - 1;
	if (r = (char)r)
	{
		gen_opcodeOne(0xeb);
		gen_byte(r);
	}
	else
	{
		gen_opcodeOne(0xe9);
		gen_dword(a - ind - 4);
	}
}

/************************************************************************/
/* ���ܣ�����������תָ��                                               */
/* t��ǰһ��תָ���ַ                                                  */
/* ����ֵ������תָ���ַ                                               */
/************************************************************************/
int gen_jcc(int t)
{
	int v;
	int inv = 1;
	v = operand_stack_top->r & SC_VALMASK;
	if (v == SC_CMP)
	{
		gen_opcodeTwo(0x0f, operand_stack_top->value ^ inv);
		t = makelist(t);
	}
	else
	{
		if (operand_stack_top->r & (SC_VALMASK | SC_LVAL | SC_SYM) == SC_LOCAL)
		{
			t = gen_jmpforward(t);
		}
		else
		{
			v = load_one(REG_ANY, operand_stack_top);
			gen_opcodeOne(0x85);
			gen_modrm(ADDR_REG, v, v, NULL, 0);

			gen_opcodeTwo(0x0f, 0x85 ^ inv);
			t = makelist(t);
		}
	}
	operand_pop();
	return t;
}

/***********************************************************
 * ����:	�����꺯����ָ�/�ͷ�ջ,����__cdecl
 * val:		��Ҫ�ͷ�ջ�ռ��С(���ֽڼ�)
 **********************************************************/
void gen_addsp(int val)
{
	int opc = 0;
	if (val == (char)val) 
	{
		// ADD--Add	83 /0 ib	ADD r/m32,imm8	Add sign-extended imm8 from r/m32
		gen_opcodeOne(0x83);	// ADD esp,val
		gen_modrm(ADDR_REG,opc,REG_ESP,NULL,0);
        gen_byte(val);
    } 
	else 
	{
		// ADD--Add	81 /0 id	ADD r/m32,imm32	Add sign-extended imm32 to r/m32
		gen_opcodeOne(81);	// add esp, val
		gen_modrm(ADDR_REG,opc,REG_ESP,NULL,0);
		gen_dword(val);
    }
}

/************************************************************************/
/* ���ܣ����ɺ������ô��룬 �Ƚ�������ջ�� Ȼ������callָ��             */
/* nb_args����������                                                    */
/************************************************************************/
void gen_invoke(int nb_args)
{
	int size, r, args_size, i, func_call;
	args_size = 0;
	for (i = 0; i < nb_args; i++)
	{
		r = load_one(REG_ANY, operand_stack_top);
		size =4;
		
		gen_opcodeOne(0x50 + r);
		args_size += size;
		operand_pop();
	}

	spill_regs();
	func_call = operand_stack_top->type.ref->r;
	gen_call();
	if (args_size && func_call != KW_STDCALL)
	{
		gen_addsp(args_size);
	}
	operand_pop();
}

/************************************************************************/
/* ���ܣ����ɺ�������ָ��                                               */
/************************************************************************/
void gen_call()
{
	int r;
	if (operand_stack_top->r & (SC_VALMASK | SC_LVAL) == SC_GLOBAL)
	{
		coffreloc_add(sec_text, operand_stack_top->sym, ind + 1, IMAGE_REL_I386_REL32);
		gen_opcodeOne(0xe8);
		gen_dword(operand_stack_top->value - 4);
	}
	else
	{
		r = load_one(REG_ANY, operand_stack_top);
		gen_opcodeOne(0xff);
		gen_opcodeOne(0xd0 + r);
	}
}


/************************************************************************/
/* ���ܣ��Ĵ������䣬�������Ĵ�����ռ�ã��Ƚ������������ջ��         */
/* rc���Ĵ�������                                                       */
/************************************************************************/
int allocate_reg(int rc)
{
	int r;
	std::vector<Operand>::iterator p;
	int used;

	for (r = 0; r < REG_EBX; r++)
	{
		if (rc & REG_ANY || r == rc) 
		{
			used = 0;
			for (p = operand_stack.begin(); p != operand_stack_top; p++)
			{
				if ((p->r & SC_VALMASK) == r)
				{
					used = 1;
				}
			}
			if (used == 0)
			{
				return r;
			}
		}
	}
	for (p = operand_stack.begin(); p != operand_stack_top; p++)
	{
		r = p->r & SC_VALMASK;
		if (r < SC_GLOBAL && (rc & REG_ANY || r == rc))
		{
			spill_reg(r);
			return r;
		}
	}
	return -1;
}

/************************************************************************/
/* ���ܣ����Ĵ���'r'������ڴ�ջ�У�                                    */
/*       ���ұ���ͷ�'r'�Ĵ����Ĳ�����Ϊ�ֲ�����                        */
/* r���Ĵ�������                                                        */
/************************************************************************/
void spill_reg(char r)
{
	int size, align;
	std::vector<Operand>::iterator p;
	Operand opd;
	Type * type;

	for (p = operand_stack.begin(); p != operand_stack_top; p++)
	{
		if ((p->r & SC_VALMASK) == r)
		{
			r = p->r & SC_VALMASK;
			type = &p->type;
			if (p->r & SC_LVAL)
			{
				type = &int_type;
			}
			size = type_size(type, &align);
			loc = calc_align(loc - size, align);
			operand_assign(&opd, type->t, SC_LOCAL | SC_LVAL, loc);
			store(r, &opd);
			if (p->r & SC_LVAL)
			{
				p->r = (p->r & ~(SC_VALMASK)) | SC_LLOCAL;
			}
			else
			{
				p->r = SC_LOCAL | SC_LVAL;
			}
			p->value = loc;
			break;
		}
	}
}

/************************************************************************/
/* ���ܣ���ռ�õļĴ���ȫ�������ջ��                                   */
/************************************************************************/
void spill_regs()
{
	int r;
	std::vector<Operand>::iterator p;
	for (p = operand_stack.begin(); p != operand_stack_top; p++)
	{
		r = p->r & SC_VALMASK;
		if (r < SC_GLOBAL)
		{
			spill_reg(r);
		}
	}
}

int main(int argc, char* argv[])
{
	printf("Hello World!\n");
	return 0;
}

