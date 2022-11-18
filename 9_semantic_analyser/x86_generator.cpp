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

/* 本章用到的全局变量 */
int return_symbol_pos;			// 记录return指令位置
int ind = 0;					// 指令在代码节位置
int loc = 0;					// 局部变量在栈中位置。
								// 因为栈顶是零，这个值基本上一直是一个负数。
								// 
int func_begin_ind;				// 函数开始指令
int func_ret_sub;				// 函数返回释放栈空间大小。默认是零。
								// 但是如果指定了__stdcall关键字，就不是零。
Symbol *sym_sec_rdata;			// 只读节符号

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

/************************************************************************/
/*  功能：操作数入栈                                                    */
/*  type：操作数数据类型                                                */
/*  r：操作数存储类型                                                   */
/*  value：操作数值                                                     */
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
void operand_assign(Operand * opd, int token_code, int storage_class, int value)
{
	opd->type.type = token_code;
	opd->storage_class = storage_class;
	opd->value = value;
}

// Operation generation functions
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
/* 因为一些Opcode并不是完整的Opcode码，它需要ModR/M字节进行辅助。       */
/* mod：Mod R/M[7：6]                                                   */
/*       与R/M一起组成32种可能的值一8个寄存器加24种寻址模式             */
/* reg_opcode：ModR/M[5：3]指令的另外3位操作码源操作数(叫法不准确)      */
/*       要么指定一个寄存器的值， 要么指定Opcode中额外的3个比特的信息， */
/*       具体作用在主操作码中指定。                                     */
/* r_m：Mod R/M[2：0] 目标操作数(叫法不准确)                            */
/*       可以指定一个寄存器作为操作数，                                 */
/*       或者和mod部分合起来表示一个寻址模式                            */
/*       类型为：e_StorageClass                                         */
/* sym：符号指针                                                        */
/* c：符号关联值                                                        */
/************************************************************************/
void gen_modrm(int mod, int reg_opcode, int r_m, Symbol * sym, int c)
{
	mod <<= 6;
	reg_opcode <<= 3;
	// mod=11 寄存器寻址 89 E5(11 reg_opcode=100ESP r=101EBP) MOV EBP,ESP
	// 0xC0转成二进制就是1100 0000
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
		gen_byte(0x05 | reg_opcode); //直接寻址
		gen_addr32(r_m, sym, c);
	}
	else if ((r_m & SC_VALMASK) == SC_LOCAL)
	{
		// mod=01 r=101 disp8[EBP] 89 45 fc MOV DWORD PTR SS:[EBP-4],EAX
		if (c == c)
		{
			// 0x45转成二进制就是0100 0101
			gen_byte(0x45 | reg_opcode);
			gen_byte(c);
		}
		else
		{
			// mod=10 r=101 disp32[EBP]
			// 0x85转成二进制就是1000 0101
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
void load(int storage_class, Operand * opd)
{
	int v, ft, fc, fr;
	ft = opd->storage_class;
	fc = opd->type.type;
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
		gen_modrm(ADDR_OTHER, storage_class, fr, opd->sym, fc);
	}
	else    // 右值
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
/* 功能：将寄存器'r'中的值存入操作数'opd'                               */
/* r：符号存储类型                                                      */
/* opd：操作数指针                                                      */
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
/*  功能：将栈顶操作数加载到'rc'类寄存器中                              */
/*  rc：寄存器类型                                                      */
/*  opd：操作数指针                                                     */
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
 * 功能:	返回t所指向的数据类型
 * t:		指针类型
 **********************************************************/
Type *pointed_type(Type *t)
{
    return &t->ref->typeSymbol;
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
/* 功能：生成整数运算                                                   */
/* OP：运算符类型                                                       */
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
/* 功能：生成整数二元运算                                               */
/* opc：ModR/M[5：3]                                                    */
/* op：运算符类型                                                       */
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
	// JMP--Jump		
	// E9 cd	JMP rel32	
	// Jump near,relative,displacement relative to next instruction
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
/* 功能：生成条件跳转指令                                               */
/* t：前一跳转指令地址                                                  */
/* 返回值：新跳转指令地址                                               */
/************************************************************************/
int gen_jcc(int t)
{
	int v;
	int inv = 1;
	v = operand_stack_top->storage_class & SC_VALMASK;
	// 如果前一句是CMP。这里要生成JLE或者JGE。
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
 * 功能:		生成函数开头代码
 * func_type:	函数类型
 * 这个函数的逻辑是这样的。就是这里不真的生成函数开头代码。
 * 只是记录函数体开始的位置，等到函数体结束时，在函数gen_epilog中填充。
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
	// 记录了函数体开始，以便函数体结束时填充函数头，因为那时才能确定开辟的栈大小。
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
		// 参数压栈以4字节对齐
		size = calc_align(size, 4);
		// 结构体作为指针传递
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
 * 功能:	生成函数结尾代码
 * 这个函数的逻辑是这样的。就是对于函数开头代码和结尾代码。
 * 他是首先生成结尾代码。之后通过func_begin_ind获取到函数头，再填充函数开头代码。
 * 函数开头包括三条指令。其中XXX为栈空间大小。一个整数就是4，两个就是8。
 * 1. PUSH EBP 
 * 2. MOVE BP， ESP 
 * 3. SUB  ESP， XXX
 * 函数结尾包括三条指令。其中XXXX为调用方式。
 * 默认不需要填写，如果指定了__stdcall关键字，就填4。
 * 4. MOV  ESP， EBP 
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
	// 把ind设置为之前记录函数体开始的位置。
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

	// 恢复ind的值。
	ind = saved_ind;
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
	func_call = operand_stack_top->type.ref->storage_class;
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
/* 功能：寄存器分配，如果所需寄存器被占用，先将其内容溢出到栈中         */
/* rc：寄存器类型                                                       */
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
/* 功能：将寄存器'r'溢出到内存栈中，                                    */
/*       并且标记释放'r'寄存器的操作数为局部变量                        */
/* r：寄存器编码                                                        */
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
/* 功能：将占用的寄存器全部溢出到栈中                                   */
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


