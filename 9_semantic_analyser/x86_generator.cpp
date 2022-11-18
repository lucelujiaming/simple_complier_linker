// x86_generator.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "token_code.h"
#include "get_token.h"
#include "symbol_table.h"
#include <windows.h>
#include "x86_generator.h"

#define OPERAND_STACK_SIZE    1024
#define FUNC_PROLOG_SIZE      9

/* �����õ���ȫ�ֱ��� */
int return_symbol_pos;			// ��¼returnָ��λ��
int ind = 0;					// ָ���ڴ����λ��
int loc = 0;					// �ֲ�������ջ��λ�á�
								// ��Ϊջ�����㣬���ֵ������һֱ��һ��������
								// 
int func_begin_ind;				// ������ʼָ��
int func_ret_sub;				// ���������ͷ�ջ�ռ��С��Ĭ�����㡣
								// �������ָ����__stdcall�ؼ��֣��Ͳ����㡣
Symbol *sym_sec_rdata;			// ֻ���ڷ���

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

/************************************************************************/
/*  ���ܣ���������ջ                                                    */
/*  type����������������                                                */
/*  r���������洢����                                                   */
/*  value��������ֵ                                                     */
/************************************************************************/
void operand_push(Type* type, int storage_class, int value)
{
	Operand operand;
	if (operand_stack.size() > OPERAND_STACK_SIZE)
	{
		printf("No memory.\n");
	}

	operand.type = *type;
	operand.storage_class    = storage_class;
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
void operand_assign(Operand * opd, int token_code, int storage_class, int value)
{
	opd->type.type = token_code;
	opd->storage_class = storage_class;
	opd->value = value;
}

// Operation generation functions
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
/* ��ΪһЩOpcode������������Opcode�룬����ҪModR/M�ֽڽ��и�����       */
/* mod��Mod R/M[7��6]                                                   */
/*       ��R/Mһ�����32�ֿ��ܵ�ֵһ8���Ĵ�����24��Ѱַģʽ             */
/* reg_opcode��ModR/M[5��3]ָ�������3λ������Դ������(�з���׼ȷ)      */
/*       Ҫôָ��һ���Ĵ�����ֵ�� Ҫôָ��Opcode�ж����3�����ص���Ϣ�� */
/*       ��������������������ָ����                                     */
/* r_m��Mod R/M[2��0] Ŀ�������(�з���׼ȷ)                            */
/*       ����ָ��һ���Ĵ�����Ϊ��������                                 */
/*       ���ߺ�mod���ֺ�������ʾһ��Ѱַģʽ                            */
/*       ����Ϊ��e_StorageClass                                         */
/* sym������ָ��                                                        */
/* c�����Ź���ֵ                                                        */
/************************************************************************/
void gen_modrm(int mod, int reg_opcode, int r_m, Symbol * sym, int c)
{
	mod <<= 6;
	reg_opcode <<= 3;
	// mod=11 �Ĵ���Ѱַ 89 E5(11 reg_opcode=100ESP r=101EBP) MOV EBP,ESP
	// 0xC0ת�ɶ����ƾ���1100 0000
	if (mod == 0xC0)
	{
		gen_byte(mod | reg_opcode | (r_m & SC_VALMASK));
	}
	else if ((r_m & SC_VALMASK) == SC_GLOBAL)
	{
		// mod=00 r=101 disp32  8b 05 50 30 40 00  MOV EAX,DWORD PTR DS:[403050]
		// 8B /r MOV r32,r/m32 Move r/m32 to r32
		// mod=00 r=101 disp32  89 05 14 30 40 00  MOV DWORD PTR DS:[403014],EAX
		// 89 /r MOV r/m32,r32 Move r32 to r/m32
		gen_byte(0x05 | reg_opcode); //ֱ��Ѱַ
		gen_addr32(r_m, sym, c);
	}
	else if ((r_m & SC_VALMASK) == SC_LOCAL)
	{
		// mod=01 r=101 disp8[EBP] 89 45 fc MOV DWORD PTR SS:[EBP-4],EAX
		if (c == c)
		{
			// 0x45ת�ɶ����ƾ���0100 0101
			gen_byte(0x45 | reg_opcode);
			gen_byte(c);
		}
		else
		{
			// mod=10 r=101 disp32[EBP]
			// 0x85ת�ɶ����ƾ���1000 0101
			gen_byte(0x85 | reg_opcode);
			gen_dword(c);
		}
	}
	else 
	{
		// mod=00 89 01(00 reg_opcode=000 EAX r=001ECX) MOV DWORD PTR DS:[ECX],EAX
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
void load(int storage_class, Operand * opd)
{
	int v, ft, fc, fr;
	ft = opd->storage_class;
	fc = opd->type.type;
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
		gen_modrm(ADDR_OTHER, storage_class, fr, opd->sym, fc);
	}
	else    // ��ֵ
	{
		if (v == SC_GLOBAL)
		{
			gen_opcodeOne(0xb8 + storage_class);
			gen_addr32(fr, opd->sym, fc);
		}
		else if (v == SC_LOCAL)
		{
			gen_opcodeOne(0x8d);
			gen_modrm(ADDR_OTHER, storage_class, SC_LOCAL, opd->sym, fc);
		}
		else if (v == SC_CMP)
		{
			gen_opcodeOne(0xb8 + storage_class);
			gen_dword(0);
			gen_opcodeTwo(0x0f, fc = 16);
			gen_modrm(ADDR_REG, 0, storage_class, NULL, 0);
		}
		else if (v != storage_class)
		{
			gen_opcodeOne(0x89);
			gen_modrm(ADDR_REG, v, storage_class, NULL, 0);
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
	fr = opd->storage_class & SC_VALMASK;
	bt = opd->type.type & T_BTYPE;

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

	if (fr == SC_GLOBAL || fr == SC_LOCAL || (opd->storage_class & SC_LVAL))
	{
		gen_modrm(ADDR_OTHER, r, opd->storage_class, opd->sym, opd->value);
	}

}

/************************************************************************/
/*  ���ܣ���ջ�����������ص�'rc'��Ĵ�����                              */
/*  rc���Ĵ�������                                                      */
/*  opd��������ָ��                                                     */
/************************************************************************/
int load_one(int rc, Operand * opd)
{
	int storage_class ;
	storage_class = opd->storage_class & SC_VALMASK;
	if (storage_class > SC_GLOBAL || (opd->storage_class & SC_LVAL))
	{
	//	storage_class = allocate_reg(rc);
		load(storage_class, opd);
	}
	opd->storage_class = storage_class;
	return storage_class;
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
	int storage_class = 0,t = 0;
	storage_class = load_one(REG_ANY, operand_stack_top);
	if ((operand_stack_second->storage_class & SC_VALMASK) == SC_LLOCAL)
	{
		Operand opd;
	//	t = allocate_reg(REG_ANY);
		operand_assign(&opd, T_INT, SC_LOCAL | SC_LVAL, 
			operand_stack_second->value);
		load(t, &opd);
		operand_stack_second->storage_class = t | SC_LVAL;
	}
	store(storage_class, operand_stack_second);
	operand_swap();
	operand_pop();
}

/***********************************************************
 * ����:	����t��ָ�����������
 * t:		ָ������
 **********************************************************/
Type *pointed_type(Type *t)
{
    return &t->ref->typeSymbol;
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

	btOne = operand_stack_second->type.type & T_BTYPE;
	btTwo = operand_stack_top->type.type & T_BTYPE;

	if (btOne == T_PTR || btTwo == T_PTR)
	{
		if (op >= TK_EQ && op <= TK_GEQ)   // >,<,>=.<=...
		{
			gen_opInteger(op);
			operand_stack_top->type.type = T_INT;
		}
		else if (btOne == T_PTR && btTwo == T_PTR)
		{
			if (op != TK_MINUS)
			{
				printf("Only support - and >,<,>=.<= \n");
			}
			u = pointed_size(&operand_stack_second->type);
			gen_opInteger(op);
			operand_stack_top->type.type = T_INT;
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
			operand_stack_top->type.type = T_INT;
		}

	}
}

/************************************************************************/
/* ���ܣ�������������                                                   */
/* OP�����������                                                       */
/************************************************************************/
void gen_opInteger(int op)
{
	int storage_class, fr, opc;
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
		storage_class  = operand_stack_second->storage_class;
		fr = operand_stack_top->storage_class;
		operand_pop();
		gen_opcodeTwo(0x0f, 0xaf);
		gen_modrm(ADDR_REG, storage_class, fr, NULL, 0);
		break;
	case TK_DIVIDE:
	case TK_MOD:
		opc = 7;
		load_two(REG_EAX, REG_ECX);
		storage_class  = operand_stack_second->storage_class;
		fr = operand_stack_top->storage_class;
		operand_pop();
		spill_reg(REG_EDX);
		gen_opcodeOne(0x99);
		gen_opcodeOne(0xf7);
		gen_modrm(ADDR_REG, opc, fr, NULL, 0);
		if (op == TK_MOD)
		{
			storage_class = REG_EDX;
		}
		else
		{
			storage_class = REG_EAX;
		}
		operand_stack_top->storage_class = storage_class;
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
	int storage_class, fr, c;
	if ((operand_stack_top->storage_class & (SC_VALMASK | SC_LOCAL | SC_SYM)) == SC_GLOBAL)
	{
		storage_class = load_one(REG_ANY, operand_stack_second);
		c = operand_stack_top->value;
		if (c == (char)c)
		{
			gen_opcodeOne(0x83);
			gen_modrm(ADDR_REG, opc, storage_class, NULL, 0);
			gen_byte(c);
		}
		else
		{
			gen_opcodeOne(0x81);
			gen_modrm(ADDR_REG, opc, storage_class, NULL, 0);
			gen_byte(c);
		}
	}
	else
	{
		load_two(REG_ANY, REG_ANY);
		storage_class  = operand_stack_second->storage_class;
		fr = operand_stack_top->storage_class;
		gen_opcodeOne((opc << 3) | 0x01);
		gen_modrm(ADDR_REG, fr, storage_class, NULL, 0);
	}
	operand_pop();
	if (op >= TK_EQ && op <= TK_GEQ)   // >,<,>=.<=...
	{
		operand_stack_top->storage_class = SC_CMP;
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
	// JMP--Jump		
	// E9 cd	JMP rel32	
	// Jump near,relative,displacement relative to next instruction
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
		// EB cb	JMP rel8	
		// Jump short,relative,displacement relative to next instruction
		gen_opcodeOne(0xeb);
		gen_byte(r);
	}
	else
	{
		// E9 cd	JMP rel32	
		// Jump short,relative,displacement relative to next instruction
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
	v = operand_stack_top->storage_class & SC_VALMASK;
	// ���ǰһ����CMP������Ҫ����JLE����JGE��
	if (v == SC_CMP)
	{
		// Jcc--Jump if Condition Is Met
		// .....
		// 0F 8F cw/cd		JG rel16/32	jump near if greater(ZF=0 and SF=OF)
		// .....
		gen_opcodeTwo(0x0f, operand_stack_top->value ^ inv);
		t = makelist(t);
	}
	else
	{
		if (operand_stack_top->storage_class & 
			(SC_VALMASK | SC_LVAL | SC_SYM) == SC_LOCAL)
		{
			t = gen_jmpforward(t);
		}
		else
		{
			v = load_one(REG_ANY, operand_stack_top);
			// TEST--Logical Compare
			// 85 /r	TEST r/m32,r32	AND r32 with r/m32,set SF,ZF,PF according to result		
			gen_opcodeOne(0x85);
			gen_modrm(ADDR_REG, v, v, NULL, 0);

			//  Jcc--Jump if Condition Is Met
			// .....
			// 0F 8F cw/cd		JG rel16/32	jump near if greater(ZF=0 and SF=OF)
			// .....
			gen_opcodeTwo(0x0f, 0x85 ^ inv);
			t = makelist(t);
		}
	}
	operand_pop();
	return t;
}

/***********************************************************
 * ����:		���ɺ�����ͷ����
 * func_type:	��������
 * ����������߼��������ġ��������ﲻ������ɺ�����ͷ���롣
 * ֻ�Ǽ�¼�����忪ʼ��λ�ã��ȵ����������ʱ���ں���gen_epilog����䡣
 **********************************************************/
void gen_prolog(Type *func_type)
{
	int addr, align, size, func_call;
	int param_addr;
	Symbol * sym;
	Type * type;

	sym = func_type->ref;
	func_call = sym->storage_class;
	addr = 8;
	loc  = 0;
	// ��¼�˺����忪ʼ���Ա㺯�������ʱ��亯��ͷ����Ϊ��ʱ����ȷ�����ٵ�ջ��С��
	func_begin_ind = ind;
	ind += FUNC_PROLOG_SIZE;

	if (sym->typeSymbol.type == T_STRUCT)
	{
		print_error("Can not return T_STRUCT");
	}

	while ((sym = sym->next) != NULL)
	{
		type = &sym->typeSymbol;
		size = type_size(type, &align);
		// ����ѹջ��4�ֽڶ���
		size = calc_align(size, 4);
		// �ṹ����Ϊָ�봫��
		if ((type->type & T_BTYPE) == T_STRUCT)
		{
			size = 4;
		}
		param_addr = addr;
		addr += size;

		sym_push(sym->token_code & ~SC_PARAMS, type,
			SC_LOCAL | SC_LVAL, param_addr);
	}
	func_ret_sub = 0; // KW_CDECL
	if(func_call == KW_STDCALL)
		func_ret_sub = addr - 8;
}

/***********************************************************
 * ����:	���ɺ�����β����
 * ����������߼��������ġ����Ƕ��ں�����ͷ����ͽ�β���롣
 * �����������ɽ�β���롣֮��ͨ��func_begin_ind��ȡ������ͷ������亯����ͷ���롣
 * ������ͷ��������ָ�����XXXΪջ�ռ��С��һ����������4����������8��
 * 1. PUSH EBP 
 * 2. MOVE BP�� ESP 
 * 3. SUB  ESP�� XXX
 * ������β��������ָ�����XXXXΪ���÷�ʽ��
 * Ĭ�ϲ���Ҫ��д�����ָ����__stdcall�ؼ��֣�����4��
 * 4. MOV  ESP�� EBP 
 * 5. POP  EBP 
 * 6. RETN XXXX
 **********************************************************/
void gen_epilog()
{
	int v, saved_ind, opc;
	// 8B /r	mov r32,r/m32	mov r/m32 to r32
	gen_opcodeOne((char)0x8B);
	/* 4. MOV ESP, EBP*/
	gen_modrm(ADDR_REG, REG_ESP, REG_EBP, NULL, 0);

	// POP--Pop a Value from the Stack
	// 58+	rd		POP r32		
	//    POP top of stack into r32; increment stack pointer
	/* 5. POP EBP */
	gen_opcodeOne(0x58 + REG_EBP);
	if (func_ret_sub == 0)
	{
		// 6. RET--Return from Procedure
		// C3	RET	near return to calling procedure
		gen_opcodeOne(0xC3);
	}
	else
	{
		// 6. RET--Return from Procedure
		//    C2 iw	RET imm16	near return to calling procedure 
		//                      and pop imm16 bytes form stack
		gen_opcodeOne(0xC2);
		gen_byte(func_ret_sub);
		gen_byte(func_ret_sub >> 8);
	}
	v = calc_align(-loc, 4);
	// ��ind����Ϊ֮ǰ��¼�����忪ʼ��λ�á�
	saved_ind = ind;
	ind = func_begin_ind;

	// 1. PUSH--Push Word or Doubleword Onto the Stack
	//    50+rd	PUSH r32	Push r32
	gen_opcodeOne(0x50 + REG_EBP);

	// 89 /r	MOV r/m32,r32	Move r32 to r/m32
	// 2. MOV EBP, ESP
	gen_opcodeOne(0x89);
	gen_modrm(ADDR_REG, REG_ESP, REG_EBP, NULL, 0);

	//SUB--Subtract		81 /5 id	SUB r/m32,imm32	
	//         Subtract sign-extended imm32 from r/m32
	// 3. SUB ESP, stacksize
	gen_opcodeOne(0x81);
	opc = 5;
	gen_modrm(ADDR_REG, opc, REG_ESP, NULL, 0);
	gen_dword(v);

	// �ָ�ind��ֵ��
	ind = saved_ind;
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
	func_call = operand_stack_top->type.ref->storage_class;
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
	if (operand_stack_top->storage_class & (SC_VALMASK | SC_LVAL) == SC_GLOBAL)
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
	int storage_class;
	std::vector<Operand>::iterator p;
	int used;

	for (storage_class = 0; storage_class < REG_EBX; storage_class++)
	{
		if (rc & REG_ANY || storage_class == rc) 
		{
			used = 0;
			for (p = operand_stack.begin(); p != operand_stack_top; p++)
			{
				if ((p->storage_class & SC_VALMASK) == storage_class)
				{
					used = 1;
				}
			}
			if (used == 0)
			{
				return storage_class;
			}
		}
	}
	for (p = operand_stack.begin(); p != operand_stack_top; p++)
	{
		storage_class = p->storage_class & SC_VALMASK;
		if (storage_class < SC_GLOBAL && (rc & REG_ANY || storage_class == rc))
		{
			spill_reg(storage_class);
			return storage_class;
		}
	}
	return -1;
}

/************************************************************************/
/* ���ܣ����Ĵ���'r'������ڴ�ջ�У�                                    */
/*       ���ұ���ͷ�'r'�Ĵ����Ĳ�����Ϊ�ֲ�����                        */
/* r���Ĵ�������                                                        */
/************************************************************************/
void spill_reg(char storage_class)
{
	int size, align;
	std::vector<Operand>::iterator p;
	Operand opd;
	Type * type;

	for (p = operand_stack.begin(); p != operand_stack_top; p++)
	{
		if ((p->storage_class & SC_VALMASK) == storage_class)
		{
			storage_class = p->storage_class & SC_VALMASK;
			type = &p->type;
			if (p->storage_class & SC_LVAL)
			{
				type = &int_type;
			}
			size = type_size(type, &align);
			loc = calc_align(loc - size, align);
			operand_assign(&opd, type->type, SC_LOCAL | SC_LVAL, loc);
			store(storage_class, &opd);
			if (p->storage_class & SC_LVAL)
			{
				p->storage_class = (p->storage_class & ~(SC_VALMASK)) | SC_LLOCAL;
			}
			else
			{
				p->storage_class = SC_LOCAL | SC_LVAL;
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
		r = p->storage_class & SC_VALMASK;
		if (r < SC_GLOBAL)
		{
			spill_reg(r);
		}
	}
}


