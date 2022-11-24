// x86_generator.cpp : Defines the entry point for the console application.
//

#include "token_code.h"
#include "get_token.h"
#include "symbol_table.h"
#include <windows.h>
#include "x86_generator.h"
#include "reg_manager.h"
#include "instruction_operator.h"
// Define all of opcodes
#include "x86_opcode_define.h"

#define FUNC_PROLOG_SIZE      9

/* 本章用到的全局变量 */
// int return_symbol_pos;			// 记录return指令位置
int sec_text_opcode_ind = 0;    // 指令在代码节位置
int function_stack_loc = 0;		// 局部变量在栈中位置。
								// 因为栈顶是零，这个值基本上一直是一个负数。
								// 
int func_begin_ind;				// 函数开始指令
int func_ret_sub;				// 函数返回释放栈空间大小。默认是零。
								// 但是如果指定了__stdcall关键字，就不是零。

extern Type int_type;			// int类型

// 生成函数
/************************************************************************/
/* 功能：生成指令寻址方式字节Mod R/M                                    */
/* 因为一些Opcode并不是完整的Opcode码，它需要ModR/M字节进行辅助。       */
/* mod：       Mod R/M[7：6]                                            */
/*             与R/M一起组成32种可能的值一8个寄存器加24种寻址模式       */
/* reg_opcode：ModR/M[5：3] 源操作数(叫法不准确)                        */
/*             作为指令的另外3位操作码，要么指定一个寄存器的值，        */
/*             要么指定Opcode中额外的3个比特的信息，                    */
/*             具体作用在主操作码中指定。                               */
/* r_m：       Mod R/M[2：0] 目标操作数(叫法不准确)                     */
/*             可以指定一个寄存器作为操作数，                           */
/*             或者和mod部分合起来表示一个寻址模式                      */
/*       注意：对于定义在栈上的局部变量。这个值会被忽略。               */
/*             局部变量统一使用disp 8/32[EBP]寻址。                     */
/*             类型为：e_StorageClass                                   */
/* sym：       符号指针                                                 */
/* c：         符号关联值                                               */
/************************************************************************/
/* 阅读这个函数需要仔细研究表8.4。整个函数分为四种情况：                */
/*  1. mod == 0xC0说明是寄存器寻址。参见表8.4的后8行。                  */
/*  2. 全局变量是一个由内存偏移量指定的内存地址。是寄存器间接寻址。     */
/*     就是disp32，也就是mod = 00，R/M = 101。等于0x05。                */
/*     参见表8.4的disp32行。                                            */
/*  3. 局部变量在栈上，地址使用EBP寻址。                                */
/*     如果是8位寻址就是disp 8[EBP]，也就是mod = 01，R/M = 101。        */
/*     等于0x45。参见表8.4的disp 8[EBP]行。                             */
/*     如果是32位寻址就是disp 32[EBP]，也就是mod = 10，R/M = 101。      */
/*     等于0x85。参见表8.4的disp 32[EBP]行。                            */
/*  4. 如果上面的情况都不是，说明是寄存器间接寻址。也就是表格的前八行。 */
/*     也就是mod = 10。R/M使用参数的值。                                */
/************************************************************************/
void gen_modrm(int mod, int reg_opcode, int r_m, Symbol * sym, int c)
{
	// 生成mod：       Mod R/M[7：6]
	mod <<= MODRM_MOD_OFFSET; // 6;
	// 生成reg_opcode：ModR/M[5：3]
	reg_opcode <<= MODRM_REG_OPCODE_OFFSET; // 3;

	// 1.  mod == 0xC0说明是寄存器寻址。参见表8.4的后8行。
	// mod=11 寄存器寻址 89 E5(11 reg_opcode=100ESP r=101EBP) MOV EBP,ESP
	// 0xC0转成二进制就是1100 0000
	if (mod == MODRM_EFFECTIVE_ADDRESS_REG) // 0xC0
	{
		// 把mod，reg_opcode，r_m组合起来，写入代码节。
		gen_byte(mod | reg_opcode | (r_m & SC_VALMASK));
	}
	// 2. 全局变量是一个由内存偏移量指定的内存地址。就是disp32。
	else if ((r_m & SC_VALMASK) == SC_GLOBAL)
	{
		// mod=00 r=101 disp32  8b 05 50 30 40 00  MOV EAX,DWORD PTR DS:[403050]
		// 8B /r MOV r32,r/m32 Move r/m32 to r32
		// mod=00 r=101 disp32  89 05 14 30 40 00  MOV DWORD PTR DS:[403014],EAX
		// 89 /r MOV r/m32,r32 Move r32 to r/m32
		gen_byte(MODRM_EFFECTIVE_ADDRESS_DISP32 // 0x05
			| reg_opcode); //直接寻址
		// "disp 32"表示SIB字节后跟随一个32位的偏移量，
		// 该偏移量被加至有效地址。
		gen_addr32(r_m, sym, c);
	}
	// 3. 局部变量在栈上，地址使用EBP寻址。 
	else if ((r_m & SC_VALMASK) == SC_LOCAL)
	{
		// 如果是8位寻址就是disp 8[EBP]
		// mod=01 r=101 disp8[EBP] 89 45 fc MOV DWORD PTR SS:[EBP-4],EAX
		if (c == c)
		{
			// 还是用 char a = g_char; 语句对应的机器码为例：
			// 	0FBE 0500204000
			//  8845FF
			// 这里生成上面的第二句。这就是45的来源。
			// 0x45转成二进制就是0100 0101
			gen_byte(MODRM_EFFECTIVE_ADDRESS_DISP8_EBP  // 0x45 
					| reg_opcode);
			// "disp  8"表示SIB字节后跟随一个8位的偏移量，
			// 该偏移量将被符号扩展，然后被加至有效地址。
			gen_byte(c);
		}
		// 如果是32位寻址就是disp 32[EBP]
		else
		{
			// mod=10 r=101 disp32[EBP]
			// 0x85转成二进制就是1000 0101
			gen_byte(MODRM_EFFECTIVE_ADDRESS_DISP32_EBP // 0x85
					| reg_opcode);
			// "disp 32"表示SIB字节后跟随一个32位的偏移量，
			// 该偏移量被加至有效地址。
			gen_dword(c);
		}
	}
	// 4. 如果上面的情况都不是，说明是寄存器间接寻址。
	else 
	{
		// mod=00 89 01(00 reg_opcode=000 EAX r=001ECX) MOV DWORD PTR DS:[ECX],EAX
		gen_byte(MODRM_EFFECTIVE_ADDRESS_REG_ADDRESS_MOD // 0x00
			| reg_opcode | (r_m & SC_VALMASK));
	}
}

/************************************************************************/
/*  功能：将操作数opd加载到寄存器r中                                    */
/*  r：符号存储类型                                                     */
/*  opd：操作数指针                                                     */
/************************************************************************/
/* 为了方便讨论，首先假设全局变量定义如下：                             */
/*    char g_char = 'a';                                                */
/*    short g_short = 123;                                              */
/*    int g_int = 123456;                                               */
/*    char g_str1[] = "g_strl";                                         */
/*    char* g_str2 = "g_str2";                                          */  
/************************************************************************/ 
void load(int storage_class, Operand * opd)
{
	int v, ft, fc, fr;
	fr = opd->storage_class;
	ft = opd->type.type;
	fc = opd->value;

	v = fr & SC_VALMASK;
	if (fr & SC_LVAL)    // 左值
	{
		// 这个分支的汇编机器码生成方法如下：
		// 例如：一条形如 char a = g_char; 的语句包括如下的指令。
		//    ; MOVSX - 带符号扩展传送指令
		//    MOVSX EAX,  BYTE PTR DS: [402000]
		//    MOV BYTE PTR SS: [EBP-1], AL
		// 正如上面的例子中给出的。a为左值。且为char。
		// 对应的机器码为：
		// 	0FBE 0500204000
		//  8845FF
		// 这就是下面代码中的神秘数字的来源。
		if ((ft & T_BTYPE) == T_CHAR)
		{
			// movsx -- move with sign-extention	
			// 0F BE /r	movsx r32,r/m8	move byte to doubleword,sign-extention
			gen_opcodeTwo(0x0f, 0xbe); // 0F BE -> movsx r32, r/m8
		}
		else if ((ft & T_BTYPE) == T_SHORT)
		{
			// movsx -- move with sign-extention	
			// 0F BF /r	movsx r32,r/m16	move word to doubleword,sign-extention
			gen_opcodeTwo(0x0f, 0xbf); // 0F BF -> movsx r32, r/m16
		}
		else 
		{
			// 8B /r	mov r32,r/m32	mov r/m32 to r32
			gen_opcodeOne(0x8b); // 8B -> mov r32, r/m32
		}
		gen_modrm(ADDR_OTHER, storage_class, fr, opd->sym, fc);
	}
	else    // 右值
	{
		if (v == SC_GLOBAL)
		{
			// B8+ rd	mov r32,imm32		mov imm32 to r32
			gen_opcodeOne(0xb8 + storage_class);
			gen_addr32(fr, opd->sym, fc);
		}
		else if (v == SC_LOCAL)
		{
			// LEA--Load Effective Address
			// 8D /r	LEA r32,m	Store effective address for m in register r32
			gen_opcodeOne(0x8d);
			gen_modrm(ADDR_OTHER, storage_class, SC_LOCAL, opd->sym, fc);
		}
		else if (v == SC_CMP) // 适用于c=a>b情况
		{
		/*适用情况生成的样例代码
		  00401384   39C8             CMP EAX,ECX
		  00401386   B8 00000000      MOV EAX,0
		  0040138B   0F9FC0           SETG AL
		  0040138E   8945 FC          MOV DWORD PTR SS:[EBP-4],EAX*/

			/*B8+ rd	mov r32,imm32		mov imm32 to r32*/
			gen_opcodeOne(0xb8 + storage_class); /* mov r, 0*/
			gen_dword(0);
			
			// SETcc--Set Byte on Condition
			// OF 9F			SETG r/m8	Set byte if greater(ZF=0 and SF=OF)
			// 0F 8F cw/cd		JG rel16/32	jump near if greater(ZF=0 and SF=OF)
			gen_opcodeTwo(0x0f, fc + 16);
			gen_modrm(ADDR_REG, 0, storage_class, NULL, 0);
		}
		else if (v != storage_class)
		{
			// 89 /r	MOV r/m32,r32	Move r32 to r/m32
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
/* 一条形如 char a = g_char; 的语句生成如下的指令中的第二句。           */
/*    ; MOVSX - 带符号扩展传送指令                                      */
/*    MOVSX EAX,  BYTE PTR DS: [402000]                                 */
/*    MOV BYTE PTR SS: [EBP-1], AL                                      */
/* 第二句对应的机器码如下： 8845 FF                                     */
/* 一条形如 short b = g_short; 的语句生成如下的指令中的第二句。         */
/*    MOVSX EAX,  WORD PTR DS: [402002]                                 */
/*    MOV WORD PTR SS: [EBP-2], AX                                      */
/* 第二句对应的机器码如下： 66: 8945FE                                  */
/* 一条形如 c = g_int; 的语句生成如下的指令中的第二句。                 */
/*    MOV  EAX,  DWORD PTR DS: [402004]                                 */
/*    MOV  DWORD PTR SS: [EBP-4], EAX                                   */
/* 第二句对应的机器码如下： 8945 FC                                     */
/************************************************************************/
/* 可以看出来，上面三种情况的综合逻辑如下：                             */
/*    1. 针对short赋值生成前缀0x66。                                    */
/*    2. 对于char赋值生成机器码0x88。                                   */
/*    3. 对于short和int赋值生成机器码0x89。                             */
/*    4. 生成后面的部分。                                               */
/************************************************************************/
void store(int r, Operand * opd)
{
	int fr, bt;
	fr = opd->storage_class & SC_VALMASK;
	bt = opd->type.type & T_BTYPE;
	// 1. 针对short赋值生成前缀0x66。
	// 用 short b = g_short; 语句对应的机器码为例：
	// 	0FBF 0502204000
	//  66: 8945FE
	// 这里生成上面的第二句中的前缀。这就是66的来源。
	if (bt == T_SHORT)
	{
		gen_prefix(OPCODE_STORE_T_SHORT_PREFIX); //Operand-size override, 66H
	}
	
	// 2. 对于char赋值生成机器码0x88。
	// 用 char a = g_char; 语句对应的机器码为例：
	// 	0FBE05 00204000
	//  8845 FF
	// 这里生成上面的第二句。这就是88的来源。
	if (bt == T_CHAR)
	{
		// 88 /r	MOV r/m,r8	Move r8 to r/m8
		gen_opcodeOne(OPCODE_STORE_CHAR_OPCODE);
	}
	// 3. 对于short和int赋值生成机器码0x89。 
	// 用 int c = g_int; 语句对应的机器码为例：
	// 	8B05 04204000
	//  8945 FC
	// 这里生成上面的第二句。这就是89的来源。
	// 这里也同时处理了 b = g_short; 语句对应的机器码生成。
	else
	{
		// 89 /r	MOV r/m32,r32	Move r32 to r/m32
		gen_opcodeOne(OPCODE_STORE_SHORT_INT_OPCODE);
	}
	// 4. 生成后面的部分。
	// 对于常量，全局变量，函数定义和局部变量，也就是栈中变量，还有左值。
	if (fr == SC_GLOBAL ||				 // 常量，全局变量，函数定义
		fr == SC_LOCAL  ||				 // 局部变量，也就是栈中变量
		(opd->storage_class & SC_LVAL))  // 左值
	{
		gen_modrm(ADDR_OTHER, r, opd->storage_class, opd->sym, opd->value);
	}

}

/************************************************************************/
/* 功能：生成整数运算。该函数被gen_op调用。                             */
/* op：运算符类型。输入类型为e_TokenCode。包括关系运算和数学运算。      */
/************************************************************************/
void gen_opInteger(int op)
{
	int dst_storage_class, src_storage_class, opc;
	switch(op) {
	// 数学运算。
	case TK_PLUS:
		opc = 0;
		gen_opTwoInteger(opc, op);
		break;
	case TK_MINUS:
		opc = 5;
		gen_opTwoInteger(opc, op);
		break;
	case TK_STAR:
		/* 一条形如 c = a * b; 的语句包括如下的指令。                           */
		/*  1. MOV  EAX,  DWORD PTR SS: [EBP-8]                                 */
		/*  2. MOV  ECX,  DWORD PTR SS: [EBP-4]                                 */
		/*  3. IMUL ECX,  EAX                                                   */
		/*  4. MOV  DWORD PTR SS: [EBP-C], ECX                                  */
		// 可以看到乘法操作分为三步。就是获得乘数，获得被乘数，执行IMUL完成乘法。
		// 获得乘数，获得被乘数。
		load_two(REG_ANY, REG_ANY);
		dst_storage_class  = operand_stack_last_top->storage_class;
		src_storage_class = operand_stack_top->storage_class;
		operand_pop();
		// 执行IMUL完成乘法。
		gen_opcodeTwo(OPCODE_IMUL_HIGH_BYTE, OPCODE_IMUL_LOW_BYTE);
		// 生成IMUL需要的指令寻址方式。
		gen_modrm(ADDR_REG, dst_storage_class, src_storage_class, NULL, 0);
		break;
	case TK_DIVIDE:
	case TK_MOD:
		opc = 7;
		load_two(REG_EAX, REG_ECX);
		dst_storage_class  = operand_stack_last_top->storage_class;
		src_storage_class = operand_stack_top->storage_class;
		operand_pop();
		spill_reg(REG_EDX);
		gen_opcodeOne(OPCODE_CDQ);
		gen_opcodeOne(OPCODE_IDIV);
		gen_modrm(ADDR_REG, opc, src_storage_class, NULL, 0);
		if (op == TK_MOD)
		{
			dst_storage_class = REG_EDX;
		}
		else
		{
			dst_storage_class = REG_EAX;
		}
		operand_stack_top->storage_class = dst_storage_class;
		break;
	// 关系运算。
	default:
		opc = 7;
		gen_opTwoInteger(opc, op);
		break;
	}
}

/************************************************************************/
/* 功能：生成整数二元运算。该函数被gen_opInteger调用。                  */
/*       只处理加法，减法和关系操作。                                   */
/* opc： ModR/M[5：3]                                                   */
/* op：  运算符类型。输入类型为e_TokenCode。只包括加法，减法和关系运算。*/
/************************************************************************/
/* 这个0x83的解释如下：                                                 */
/* 参考Add/Cmp/Sub的命令格式在Intel白皮书604/726/1776页可以发现：       */
/*     0x83表示是"Add sign-extended imm8 to r/m32."。                   */
/*     0x83表示是"Compare imm8 with r/m32."。                           */
/*     0x83表示是"Subtract sign-extended imm8 from r/m32."。            */
/* 这个0x83的解释过程如下：                                             */
/*     首先查看Intel白皮书2517页的One-byte Opcode Map表。               */
/*     查出来是Immediate Grp 1，格式是Ev,lb。说明这个包含了多条指令。   */
/*     要根据后面的一个字节的ModR/M确定相应的指令                       */
/*     操作数中含有Ev符号，那么紧跟后面的一个字节就是MODR/M，           */
/*     通过拆分ModR/M得到ModR/M[5：3]就可以推断出指令。                 */
/************************************************************************/
/* 这个0x81的解释如下：                                                 */
/* 参考Add/Cmp/Sub的命令格式在Intel白皮书604/726/1776页可以发现：       */
/*     0x81表示是"Add imm32 to r/m32."。                                */
/*     0x81表示是"Compare imm32 with r/m32."。                          */
/*     0x81表示是"Subtract imm32 from r/m32."。                         */
/* 这个0x81的解释过程如下：                                             */
/*     首先查看Intel白皮书2517页的One-byte Opcode Map表。               */
/*     查出来是Immediate Grp 1，格式是Ev,lz。说明这个包含了多条指令。   */
/*     要根据后面的一个字节的ModR/M确定相应的指令                       */
/*     操作数中含有Ev符号，那么紧跟后面的一个字节就是MODR/M，           */
/*     通过拆分ModR/M得到ModR/M[5：3]就可以推断出指令。                 */
/************************************************************************/
void gen_opTwoInteger(int opc, int op)
{
	int dst_storage_class, src_storage_class, c;
	// 如果栈顶元素不是符号，也就不是全局变量和函数定义，
	// 而且如果也不是栈中变量。那就只能是常量。
	if ((operand_stack_top->storage_class & (SC_VALMASK | SC_LOCAL | SC_SYM)) == SC_GLOBAL)
	{
		dst_storage_class = load_one(REG_ANY, operand_stack_last_top);
		// 如果c是一个8位立即数。
		c = operand_stack_top->value;
		if (c == (char)c)
		{
			// ADC--Add with Carry			83 /2 ib	ADC r/m32,imm8	Add with CF sign-extended imm8 to r/m32
			// ADD--Add						83 /0 ib	ADD r/m32,imm8	Add sign-extended imm8 from r/m32
			// SUB--Subtract				83 /5 ib	SUB r/m32,imm8	Subtract sign-extended imm8 to r/m32
			// CMP--Compare Two Operands	83 /7 ib	CMP r/m32,imm8	Compare imm8 with r/m32
			// 不管是加减法还是比较操作，只要是第一个字节都是0x83。
			gen_opcodeOne(0x83);
			// 只有第二个字节不同。
			gen_modrm(ADDR_REG, opc, dst_storage_class, NULL, 0);
			// 最后跟上立即数就好了。
			gen_byte(c);
		}
		// 否则就是32位的立即数。
		else
		{
			// ADD--Add					    81 /0 id	ADD r/m32,imm32	Add sign-extended imm32 to r/m32
			// SUB--Subtract				81 /5 id	SUB r/m32,imm32	Subtract sign-extended imm32 from r/m32
			// CMP--Compare Two Operands	81 /7 id	CMP r/m32,imm32	Compare imm32 with r/m32
			gen_opcodeOne(0x81);
			gen_modrm(ADDR_REG, opc, dst_storage_class, NULL, 0);
			gen_byte(c);
		}
	}
	// 如果栈顶元素不是常量，第一个字节就和opc有关系了。
	else
	{
		load_two(REG_ANY, REG_ANY);
		dst_storage_class  = operand_stack_last_top->storage_class;
		src_storage_class  = operand_stack_top->storage_class;
		// 在这里加法时，opc是0。减法时，opc是5。比较时，opc是7。
		// 因此上，计算结果就是：加法时为0x01。减法时为0x29。比较时为0x39。
		// 查看Intel白皮书2517页的One-byte Opcode Map表。
		// 可以找到对应的指令，分别为ADD，SUB，CMP
		gen_opcodeOne((opc << 3) | 0x01);
		// 根据操作数存储类型和目标操作数存储类型生成指令寻址方式字节。
		gen_modrm(ADDR_REG, src_storage_class, dst_storage_class, NULL, 0);
	}
	// 计算完成。弹出栈顶元素。
	operand_pop();
	// CMP影响位在Intel白皮书417页。
	// 也就是Table B-1. EFLAGS Condition Codes
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
	indOne = sec_text_opcode_ind + 4;
	if (indOne > sec_text->data_allocated)
	{
		section_realloc(sec_text, indOne);
	}
	*(int *)(sec_text->data + sec_text_opcode_ind) = s;
	s = sec_text_opcode_ind;
	sec_text_opcode_ind = indOne;
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
	r = a - sec_text_opcode_ind - 1;
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
		gen_dword(a - sec_text_opcode_ind - 4);
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
		// 如果栈顶元素不是符号，也就不是全局变量和函数定义，
		// 而且如果也不是栈中变量。那就只能是常量。
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
	function_stack_loc  = 0;
	// 记录了函数体开始，以便函数体结束时填充函数头，因为那时才能确定开辟的栈大小。
	func_begin_ind = sec_text_opcode_ind;
	sec_text_opcode_ind += FUNC_PROLOG_SIZE;

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
	v = calc_align(-function_stack_loc, 4);
	// 把ind设置为之前记录函数体开始的位置。
	saved_ind = sec_text_opcode_ind;
	sec_text_opcode_ind = func_begin_ind;

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
	sec_text_opcode_ind = saved_ind;
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
		gen_opcodeOne(0x81);	// add esp, val
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
	// 参数依次入栈
	for (i = 0; i < nb_args; i++)
	{
		r = load_one(REG_ANY, operand_stack_top);
		size =4;
		// PUSH--Push Word or Doubleword Onto the Stack
		// 50+rd	PUSH r32	Push r32
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
		coffreloc_add(sec_text, operand_stack_top->sym, 
			sec_text_opcode_ind + 1, IMAGE_REL_I386_REL32);
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



