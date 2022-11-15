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
extern  Section *sec_text,			// 代码节
		*sec_data,			// 数据节
		*sec_bss,			// 未初始化数据节
		*sec_idata,			// 导入表节
		*sec_rdata,			// 只读数据节
		*sec_rel,			// 重定位信息节
		*sec_symtab,		// 符号表节	
		*sec_dynsymtab;		// 链接库符号节

Type char_pointer_type,		// 字符串指针				
	 int_type,				// int类型
	 default_func_type;		// 缺省函数类型
// Operand functions
void gen_opInteger(int op);
void gen_opTwoInteger(int opc, int op);
int pointed_size(Type * type);
void spill_reg(char c);
void spill_regs();
void gen_call();
/************************************************************************/
/*  功能：操作数入栈                                                    */
/*  type：操作数数据类型                                                */
/*  r：操作数存储类型                                                   */
/*  value：操作数值                                                     */
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
/* 功能：弹出栈顶操作数                                                 */
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
/*  功能：交换栈顶两个操作数顺序                                        */
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
/*  功能：操作数赋值                                                    */
/*  t：操作数数据类型                                                   */
/*  r：操作数存储类型                                                   */
/*  value：操作数值                                                     */
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
/*  功能：向代码节写人一个字节                                          */
/*  c：字节值                                                           */
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
/* 功能：生成指令寻址方式字节Mod R/M                                    */
/* mod：Mod R/M[7：6]                                                   */
/* reg_opcode：ModR/M[5：3]指令的另外3位操作码源操作数(叫法不准确)      */
/* r_m：Mod R/M[2：0] 目标操作数(叫法不准确)                            */
/* sym：符号指针                                                        */
/* c：符号关联值                                                        */
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
/* r：符号存储类型                                                      */
/* sym：符号指针                                                        */
/* c：符号关联值                                                        */
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
/*  功能：将操作数opd加载到寄存器r中                                    */
/*  r：符号存储类型                                                     */
/*  opd：操作数指针                                                     */
/************************************************************************/
void load(int r, Operand * opd)
{
	int v, ft, fc, fr;
	ft = opd->r;
	fc = opd->type.t;
	fr = opd->value;

	v = fr & SC_VALMASK;
	if (fr & SC_LVAL)    // 左值
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
	else    // 右值
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
/* 功能：将寄存器'r'中的值存入操作数'opd'                               */
/* r：符号存储类型                                                      */
/* opd：操作数指针                                                      */
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
/*  功能：将栈顶操作数加载到'rc'类寄存器中                              */
/*  rc：寄存器类型                                                      */
/*  opd：操作数指针                                                     */
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
/* 功能：将栈顶操作数加载到'rc1'类寄存器，将次栈顶操作数加载到'rc2'类寄存器   */
/* rcl：栈顶操作数加载到的寄存器类型                                          */
/* rc2：次栈顶操作数加载到的寄存器类型                                        */
/******************************************************************************/
void load_two(int rc1, int rc2)
{
	load_one(rc2, operand_stack_top);
	load_one(rc2, operand_stack_second);
}

/************************************************************************/
/* 功能：将栈顶操作数存人次栈顶操作数中                                 */
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
 * 功能:	返回t所指向的数据类型
 * t:		指针类型
 **********************************************************/
Type *pointed_type(Type *t)
{
    return &t->ref->type;
}

/***********************************************************
 * 功能:	返回t所指向的数据类型尺寸
 * t:		指针类型
 **********************************************************/
int pointed_size(Type *t)
{
    int align;
    return type_size(pointed_type(t), &align);
}

/************************************************************************/
/* 功能：生成二元运算，对指针操作数进行一些特殊处理                     */
/* op：运算符类型                                                       */
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
/* 功能：生成整数运算                                                   */
/* OP：运算符类型                                                       */
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
/* 功能：生成整数二元运算                                               */
/* opc：ModR/M[5：3]                                                    */
/* op：运算符类型                                                       */
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
/* 功能：记录待定跳转地址的指令链                                       */
/* s：前一跳转指令地址                                                  */
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
/* 功能：回填函数，把t为链首的各个待定跳转地址填人相对地址              */
/* t：链首                                                              */
/* a：指令跳转位置                                                      */
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
/* 功能：生成向高地址跳转指令，跳转地址待定                             */
/* t：前一跳转指令地址                                                  */
/* makelist本来代码在书的后面                                           */
/************************************************************************/
int gen_jmpforward(int t)
{
	gen_opcodeOne(0xe9);
	return makelist(t);
}

/************************************************************************/
/* 功能：生成向低地址跳转指令，跳转地址已确定                           */
/* a：跳转到的目标地址                                                  */
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
/* 功能：生成条件跳转指令                                               */
/* t：前一跳转指令地址                                                  */
/* 返回值：新跳转指令地址                                               */
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
 * 功能:	调用完函数后恢复/释放栈,对于__cdecl
 * val:		需要释放栈空间大小(以字节计)
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
/* 功能：生成函数调用代码， 先将参数人栈， 然后生成call指令             */
/* nb_args：参数个数                                                    */
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
/* 功能：生成函数调用指令                                               */
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
/* 功能：寄存器分配，如果所需寄存器被占用，先将其内容溢出到栈中         */
/* rc：寄存器类型                                                       */
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
/* 功能：将寄存器'r'溢出到内存栈中，                                    */
/*       并且标记释放'r'寄存器的操作数为局部变量                        */
/* r：寄存器编码                                                        */
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
/* 功能：将占用的寄存器全部溢出到栈中                                   */
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

