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
int sec_text_opcode_offset = 0;    // 指令在代码节位置
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
/* value：     符号关联值                                               */
/************************************************************************/
/* 阅读这个函数需要仔细研究书上的表8.4。或者查看Intel白皮书509页的      */
/*     Table 2-1. 16-Bit Addressing Forms with the ModR/M Byte          */
/* 整个函数分为四种情况：                                               */
/*  1. mod == 0xC0说明是寄存器寻址。参见书上的表8.4的后8行。            */
/*  2. 全局变量是一个由内存偏移量指定的内存地址。是寄存器间接寻址。     */
/*     就是disp32，也就是mod = 00，R/M = 101。等于0x05。                */
/*     参见书上的表8.4的disp32行。                                      */
/*  3. 局部变量在栈上，地址使用EBP寻址。                                */
/*     如果是8位寻址就是disp 8[EBP]，也就是mod = 01，R/M = 101。        */
/*     等于0x45。参见书上的表8.4的disp 8[EBP]行。                       */
/*     如果是32位寻址就是disp 32[EBP]，也就是mod = 10，R/M = 101。      */
/*     等于0x85。参见书上的表8.4的disp 32[EBP]行。                      */
/*  4. 如果上面的情况都不是，说明是寄存器间接寻址。也就是表格的前八行。 */
/*     也就是mod = 10。R/M使用参数的值。                                */
/************************************************************************/
void gen_modrm(int mod, int reg_opcode, int r_m, Symbol * sym, int value)
{
	// 生成mod：       Mod R/M[7：6]
	mod <<= MODRM_MOD_OFFSET; // 6;
	// 生成reg_opcode：ModR/M[5：3]
	reg_opcode <<= MODRM_REG_OPCODE_OFFSET; // 3;

	// 1.  mod == 0xC0说明是寄存器寻址。参见书上的表8.4的后8行。
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
		gen_addr32(r_m, sym, value);
	}
	// 3. 局部变量在栈上，地址使用EBP寻址。 
	else if ((r_m & SC_VALMASK) == SC_LOCAL)
	{
		// 如果是8位寻址就是disp 8[EBP]
		// mod=01 r=101 disp8[EBP] 89 45 fc MOV DWORD PTR SS:[EBP-4],EAX
		if (value == (char)value)
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
			gen_byte(value);
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
			gen_dword(value);
		}
	}
	// 4. 如果上面的情况都不是，说明是寄存器间接寻址。
	else 
	{
		// mod=00 89 01(00 reg_opcode=000 EAX r=001ECX) MOV DWORD PTR DS:[ECX],EAX
		gen_byte(MODRM_EFFECTIVE_ADDRESS_REG_ADD_MOD // 0x00
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
	int opd_storage_class_val;
	int opd_type, opd_operand_value, opd_storage_class;
	opd_storage_class = opd->storage_class;
	opd_type = opd->type.type;
	opd_operand_value = opd->operand_value;

	opd_storage_class_val = opd_storage_class & SC_VALMASK;
	if (opd_storage_class & SC_LVAL)    // 左值
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
		if ((opd_type & T_BTYPE) == T_CHAR)
		{
			// 参考MOVSX的命令格式在Intel白皮书1250页可以发现：
			//     0F BE表示是" Move byte to doubleword with sign-extension."。
			// movsx -- move with sign-extention	
			// 0F BE /r	movsx r32,r/m8	move byte to doubleword,sign-extention
			// 0F BE -> movsx r32, r/m8
			gen_opcodeTwo(OPCODE_MOVSX_BYTE_TO_DOUBLEWORD_HIGH_BYTE, // 0x0f, 
				OPCODE_MOVSX_BYTE_TO_DOUBLEWORD_LOW_BYTE); // 0xbe); 
		}
		else if ((opd_type & T_BTYPE) == T_SHORT)
		{
			// 参考MOVSX的命令格式在Intel白皮书1250页可以发现：
			//     0F BF表示是"Move word to doubleword, with sign-extension."。
			// movsx -- move with sign-extention	
			// 0F BF /r	movsx r32,r/m16	move word to doubleword,sign-extention
			// 0F BF -> movsx r32, r/m16
			gen_opcodeTwo(OPCODE_MOVSX_WORD_TO_DOUBLEWORD_HIGH_BYTE, // 0x0f, 
				OPCODE_MOVSX_WORD_TO_DOUBLEWORD_LOW_BYTE); // 0xbf); 
		}
		else 
		{
			// 参考MOV的命令格式在Intel白皮书1161页可以发现：
			//     8B表示是"Move r/m32 to r32."。
			// 8B /r	mov r32,r/m32	mov r/m32 to r32
			gen_opcodeOne(OPCODE_MOV_RM32_TO_R32); // 0x8b); // 8B -> mov r32, r/m32
		}
		gen_modrm(ADDR_OTHER, storage_class, 
			opd_storage_class, opd->sym, opd_operand_value);
	}
	else    // 右值
	{
		// 如果右值是全局变量。例如：立即数。注意：
		// 很多局部变量其实是放在寄存器中的。此时，不会为右值生成指令。例如：
		//     int  c = g_a ; // g_a为全局变量。
		if (opd_storage_class_val == SC_GLOBAL)
		{
			// B8+ rd	mov r32,imm32		mov imm32 to r32
			gen_opcodeOne(OPCODE_MOVE_IMM32_TO_R32 + storage_class);
			gen_addr32(opd_storage_class, opd->sym, opd_operand_value);
		}
		// 如果右值是局部变量。注意：
		// 很多局部变量其实是放在寄存器中的。此时，不会为右值生成指令。
		// 只有需要对右值寻址的情况下，才会进入这个分支。例如：
		//    int  * pa = &a;
		//    int x = pt.x;
		else if (opd_storage_class_val == SC_LOCAL)
		{
			// LEA--Load Effective Address
			// 8D /r	LEA r32,m	Store effective address for m in register r32
			gen_opcodeOne(OPCODE_LEA_EFFECTIVE_ADDRESS_IN_R32);
			gen_modrm(ADDR_OTHER, storage_class, SC_LOCAL, opd->sym, opd_operand_value);
		}
		else if (opd_storage_class_val == SC_CMP) // 适用于c=a>b情况
		{
			/************************************************************************/
			/*  适用情况生成的样例代码                                              */
			/*  00401384   39C8             CMP EAX,ECX                             */
			/*  00401386   B8 00000000      MOV EAX,0                               */
			/*  0040138B   0F9FC0           SETG AL                                 */
			/*  0040138E   8945 FC          MOV DWORD PTR SS:[EBP-4],EAX            */
			/************************************************************************/
			/*B8+ rd	mov r32,imm32		mov imm32 to r32*/
			gen_opcodeOne(OPCODE_MOVE_IMM32_TO_R32 + storage_class); /* mov r, 0*/
			gen_dword(0);
			
			// 参考SETcc的命令格式在Intel白皮书1718页可以发现：
			//     0F 9F表示是"Set byte if greater (ZF=0 and SF=OF)."。
			// SETcc--Set Byte on Condition
			// OF 9F			SETG r/m8	Set byte if greater(ZF=0 and SF=OF)
			// 0F 8F cw/cd		JG rel16/32	jump near if greater(ZF=0 and SF=OF)
			gen_opcodeTwo(OPCODE_SET_BYTE_IF_GREATER_HIGH_BYTE // 0x0f
						, opd_operand_value + 16);
			gen_modrm(ADDR_REG, 0, storage_class, NULL, 0);
		}
		else if (opd_storage_class_val != storage_class)
		{
			// 89 /r	MOV r/m32,r32	Move r32 to r/m32
			gen_opcodeOne(OPCODE_STORE_SHORT_INT_OPCODE);
			gen_modrm(ADDR_REG, opd_storage_class_val, storage_class, NULL, 0);
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
void store(int reg_index, Operand * opd)
{
	int opd_storage_class, opd_type;
	opd_storage_class = opd->storage_class & SC_VALMASK;
	opd_type          = opd->type.type & T_BTYPE;
	// 1. 针对short赋值生成前缀0x66。
	// 用 short b = g_short; 语句对应的机器码为例：
	// 	0FBF 0502204000
	//  66: 8945FE
	// 这里生成上面的第二句中的前缀。这就是66的来源。
	if (opd_type == T_SHORT)
	{
		gen_prefix(OPCODE_STORE_T_SHORT_PREFIX); //Operand-size override, 66H
	}
	
	// 2. 对于char赋值生成机器码0x88。
	// 用 char a = g_char; 语句对应的机器码为例：
	// 	0FBE05 00204000
	//  8845 FF
	// 这里生成上面的第二句。这就是88的来源。
	if (opd_type == T_CHAR)
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
	if (opd_storage_class == SC_GLOBAL ||  // 常量，全局变量，函数定义
		opd_storage_class == SC_LOCAL  ||  // 局部变量，也就是栈中变量
		(opd->storage_class & SC_LVAL))    // 左值
	{
		gen_modrm(ADDR_OTHER, reg_index, opd->storage_class, opd->sym, opd->operand_value);
	}

}

/************************************************************************/
/* 功能：生成整数运算。该函数被gen_op调用。                             */
/* op：运算符类型。输入类型为e_TokenCode。包括关系运算和数学运算。      */
/************************************************************************/
void gen_opInteger(int op)
{
	int dst_storage_class, src_storage_class;
	int reg_code = REG_ANY;  // Set error code as default value
	switch(op) {
	// 数学运算。
	case TK_PLUS:
		// 查看Intel白皮书2517页的One-byte Opcode Map表。
		// 可以看到ADD的取值为0x00 ~ 0x05。
		// 这里取高5位。也就是除以8。为0。
		reg_code = ONE_BYTE_OPCODE_ADD_GIGH_FIVE_BYTES;
		gen_opTwoInteger(reg_code, op);
		break;
	case TK_MINUS:
		// 查看Intel白皮书2517页的One-byte Opcode Map表。
		// 可以看到ADD的取值为0x28 ~ 0x2D。
		// 这里取高5位。也就是除以8。为5。
		reg_code = ONE_BYTE_OPCODE_SUB_GIGH_FIVE_BYTES;
		gen_opTwoInteger(reg_code, op);
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
		/* 我们在Intel白皮书509页的Table 2-1.中查找F9。可以发现：               */
		/*     因为我们是直接使用ECX。因此上Mod=0b11也就是3，R/M为001           */
		/*     而reg_code = 0b111，也就是REG_EDI。                              */
		reg_code = REG_EDI; // 7;
		// 因为IDIV ECX会导致EAX除以ECX。
		// 把除数和被除数加载到EAX和ECX中。
		load_two(REG_EAX, REG_ECX);
		dst_storage_class  = operand_stack_last_top->storage_class;
		src_storage_class = operand_stack_top->storage_class;
		operand_pop();
		// 因为IDIV ECX的结果中，商放在EAX，余数在EDX。
		// 因此上，这里需要预留REG_EDX。
		spill_reg(REG_EDX);

		// CWD/CDQ--Convert Word to Doubleword/Convert Doubleword to Qradword
		// 99	CWQ	EDX:EAX<--sign_entended EAX
		gen_opcodeOne(OPCODE_CDQ);

		/* 一条形如 IDIV ECX  对应的机器码为：F7F9。下面先写第一个字符F7        */
		/************************************************************************/
		/* 指令参见Intel白皮书1015页。使用IDIV r/m32，也就是F7。解释如下：      */
		/*  Signed divide EDX:EAX by r/m32, with result                         */
		/*  stored in EAX <- Quotient, EDX <- Remainder.                        */
		/************************************************************************/
		/* IDIV--Signed Divide                                                  */
		/* F7 /7	IDIV r/m32	Signed divide EDX:EAX                           */
		/* (where EDX must contain signed extension of EAX)                     */
		/* by r/m doubleword.(Result:EAX=Quotient, EDX=Remainder)               */
		/* EDX:EAX被除数 r/m32除数                                              */
		/************************************************************************/
		gen_opcodeOne(OPCODE_IDIV);
		/* 写第二个字符F9                                                       */
		/* 我们在Intel白皮书509页的Table 2-1.中查找F9。可以发现：               */
		/*     因为我们是直接使用ECX。因此上Mod=0b11也就是3，R/M为001           */
		/*     而reg_code = 0b111，也就是REG_EDI。                              */
		gen_modrm(ADDR_REG, reg_code, src_storage_class, NULL, 0);
		// 因为IDIV ECX的结果中，商放在EAX，余数在EDX。
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
		// 查看Intel白皮书2517页的One-byte Opcode Map表。
		// 可以看到ADD的取值为0x38 ~ 0x3D。
		// 这里取高5位。也就是除以8。为7。
		reg_code = ONE_BYTE_OPCODE_CMP_GIGH_FIVE_BYTES;
		gen_opTwoInteger(reg_code, op);
		break;
	}
}

/************************************************************************/
/* 功能：     生成整数二元运算。该函数被gen_opInteger调用。             */
/*            只处理加法，减法和关系操作。                              */
/* reg_code： ModR/M[5：3]                                              */
/* op：       运算符类型。输入类型为e_TokenCode。                       */
/*            只包括加法，减法和关系运算。                              */
/************************************************************************/
/* 这个函数逻辑如下：                                                   */
/* 1. 首先计算包括三种情况。                                            */
/*   1.1. 如果目标操作数是8位立即数。例如a + 8; 6 + 8; a - 8;           */
/*        那第一个字节就是0x83。使用传入的reg_code生成指令寻址方式字节。*/
/*   1.2. 如果目标操作数是32位立即数。那第一个字节就是0x81。            */
/*        使用传入的reg_code生成指令寻址方式字节。                      */
/*   1.3. 如果目标操作数不是立即数。                                    */
/*        根据操作数存储类型和目标操作数存储类型生成指令寻址方式字节。  */
/* 2. 对于关系操作，更新CMP影响位。                                    */
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
void gen_opTwoInteger(int reg_code, int op)
{
	int dst_storage_class, src_storage_class, value;
	// 如果栈顶元素不是符号，也就不是全局变量和函数定义，
	// 而且如果也不是左值。那就只能是常量。
	if ((operand_stack_top->storage_class & 
		(SC_VALMASK | SC_LVAL | SC_SYM)) == SC_GLOBAL)
	{
		dst_storage_class = load_one(REG_ANY, operand_stack_last_top);
		// 如果c是一个8位立即数。
		value = operand_stack_top->operand_value;
		if (value == (char)value)
		{
			// ADC--Add with Carry			83 /2 ib	ADC r/m32,imm8	Add with CF sign-extended imm8 to r/m32
			// ADD--Add						83 /0 ib	ADD r/m32,imm8	Add sign-extended imm8 from r/m32
			// SUB--Subtract				83 /5 ib	SUB r/m32,imm8	Subtract sign-extended imm8 to r/m32
			// CMP--Compare Two Operands	83 /7 ib	CMP r/m32,imm8	Compare imm8 with r/m32
			// 不管是加减法还是比较操作，只要是第一个字节都是0x83。
			gen_opcodeOne(ONE_BYTE_OPCODE_IMME_GRP_TO_IMM8); // (0x83);
			// 只有第二个字节不同。
			gen_modrm(ADDR_REG, reg_code, dst_storage_class, NULL, 0);
			// 最后跟上立即数就好了。
			gen_byte(value);
		}
		// 否则就是32位的立即数。
		else
		{
			// ADD--Add					    81 /0 id	ADD r/m32,imm32	Add sign-extended imm32 to r/m32
			// SUB--Subtract				81 /5 id	SUB r/m32,imm32	Subtract sign-extended imm32 from r/m32
			// CMP--Compare Two Operands	81 /7 id	CMP r/m32,imm32	Compare imm32 with r/m32
			gen_opcodeOne(ONE_BYTE_OPCODE_IMME_GRP_TO_IMM32); //(0x81);
			gen_modrm(ADDR_REG, reg_code, dst_storage_class, NULL, 0);
			gen_byte(value);
		}
	}
	// 如果栈顶元素不是常量，第一个字节就和opc有关系了。
	else
	{
		// More clear
		int one_byte_opcode = reg_code;
		load_two(REG_ANY, REG_ANY);
		dst_storage_class  = operand_stack_last_top->storage_class;
		src_storage_class  = operand_stack_top->storage_class;
		// 在这里加法时，opc是0。减法时，opc是5。比较时，opc是7。
		// 因此上，计算结果就是：加法时为0x01。减法时为0x29。比较时为0x39。
		// 查看Intel白皮书2517页的One-byte Opcode Map表。
		// 可以找到对应的指令，分别为ADD，SUB，CMP
		gen_opcodeOne((one_byte_opcode << 3)
				| ONE_BYTE_OPCODE_LOW_THREE_BYTE_EV_GV); // 0x01);
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
			operand_stack_top->operand_value
				= OPCODE_CONDITION_CODES_EQUAL;           // 0x84;
			break;
		case TK_NEQ:
			operand_stack_top->operand_value
				= OPCODE_CONDITION_CODES_NOT_EQUAL;       // 0x85;
			break;
		case TK_LT:
			operand_stack_top->operand_value
				= OPCODE_CONDITION_CODES_LESS;            // 0x8c;
			break;
		case TK_LEQ:
			operand_stack_top->operand_value
				= OPCODE_CONDITION_CODES_LESS_OR_EQUAL;   // 0x8e;
			break;
		case TK_GT:
			operand_stack_top->operand_value
				= OPCODE_CONDITION_CODES_GREATER;          // 0x8f;
			break;
		case TK_GEQ:
			operand_stack_top->operand_value
				= OPCODE_CONDITION_CODES_GREATER_OR_EQUAL; // 0x8d;
			break;
		}
	}
}

/************************************************************************/
/* 功能：          生成向高地址跳转指令，跳转地址待定                   */
/* target_address：前一跳转指令地址                                     */
/* makelist本来代码在书的后面                                           */
/************************************************************************/
int gen_jmpforward(int target_address)
{
	// 参考JMP的命令格式在Intel白皮书1064页可以发现：
	//     E9 cd表示是"Jump near, relative, RIP = RIP + 32-bit 
    //                 displacement sign extended to 64-bits."。
	// JMP--Jump		
	// E9 cd	JMP rel32	
	// Jump near,relative,displacement relative to next instruction
	gen_opcodeOne(OPCODE_JUMP_NEAR); // (0xe9);
	return make_jmpaddr_list(target_address);
}

/************************************************************************/
/* 功能：生成向低地址跳转指令，跳转地址已确定                           */
/* target_address：跳转到的目标地址                                     */
/************************************************************************/
#define  OPCODE_SIZE_JMP         2
void gen_jmpbackward(int target_address)
{
	int displacement;
	// displacement - 偏移量。这里可以理解为相对地址。
	// 这个偏移量的计算方法如下：
	// 首先假设指令在代码节的偏移量为51，也就是sec_text_opcode_ind = 51。
	// 我想跳转到偏移量28上面，也就是target_address = 28。
	// 我们需要将两个值相减。同时我们还需要减去JMP指令的机器码本身的大小。
	displacement = target_address - sec_text_opcode_offset - OPCODE_SIZE_JMP;
	// 8位转移是短转移。短跳转范围是-128-127。
	if (displacement = (char)displacement)
	{
		// 参考JMP的命令格式在Intel白皮书1064页可以发现：
		//     EB cb表示是"Jump short, RIP = RIP + 8-bit displacement sign 
	    //                 extended to 64-bits."。
		// EB cb	JMP rel8	
		// Jump short,relative,displacement relative to next instruction
		gen_opcodeOne(OPCODE_JUMP_SHORT); //(0xeb);
		// 8-bit displacement
		gen_byte(displacement);
	}
	// 否则是32位转移 - 远转移
	else
	{
		// 参考JMP的命令格式在Intel白皮书1064页可以发现：
		//     E9 cd表示是"Jump near, relative, RIP = RIP + 32-bit 
	    //                 displacement sign extended to 64-bits."。
		// E9 cd	JMP rel32	
		// Jump short,relative,displacement relative to next instruction
		gen_opcodeOne(OPCODE_JUMP_NEAR); //(0xe9);
		// 偏移量 - 32-bit displacement
		gen_dword(target_address - sec_text_opcode_offset - 4);
	}
}

/************************************************************************/
/* 功能：    生成条件跳转指令                                           */
/* jmp_addr：前一跳转指令地址                                           */
/* 返回值：  由make_jmpaddr_list生成的新跳转指令地址。                  */
/*           gen_jmpforward也调用了make_jmpaddr_list并返回。            */
/************************************************************************/
int gen_jcc(int jmp_addr)
{
	int storage_class;
	// 参考Jcc的命令格式在Intel白皮书1060页可以发现：
	// 当ZF=1时，最低位会改变
	int inv = 1;  // if 0 (ZF=1)
	storage_class = operand_stack_top->storage_class & SC_VALMASK;
	// 如果前一句是CMP。这里要生成JLE或者JGE。
	// 这里处理的是条件表达式的正常写法：
	//     if (a > b)
	if (storage_class == SC_CMP)
	{
		// 当storage_class为SC_CMP的时候，operand_value为对应的CMP影响位。
		// 参见在Intel白皮书417页。也就是Table B-1. EFLAGS Condition Codes
		// 参考Jcc的命令格式在Intel白皮书1060页可以发现：
		//     0F 8F cw/cd 表示是"Jump near if greater (ZF=0 and SF=OF)."。
		// Jcc--Jump if Condition Is Met
		// .....
		// 0F 8F cw/cd		JG rel16/32	jump near if greater(ZF=0 and SF=OF)
		// .....
		gen_opcodeTwo(OPCODE_JCC_JUMP_NEAR_IF_GREATER // 0x0f
				, operand_stack_top->operand_value ^ inv);
		jmp_addr = make_jmpaddr_list(jmp_addr);
	}
	else
	{
		// 如果栈顶元素不是符号，也就不是全局变量和函数定义，
		// 而且如果也不是左值。那就只能是常量。
		// 也就是这种神奇的情况，就是会有人这么写代码：
		//     if (1)
		// 因此只需要直接JMP即可。
		if (operand_stack_top->storage_class & 
			(SC_VALMASK | SC_LVAL | SC_SYM) == SC_GLOBAL)
		{
			jmp_addr = gen_jmpforward(jmp_addr);
		}
		// 这里处理了另一种神奇的情况，就是会有人这么写代码：
		//     if (a = 7)
		// 也就是这种情况下，if后面的表达式不是一个比较操作。而是一个计算操作。
		else
		{
			storage_class = load_one(REG_ANY, operand_stack_top);
			// 参考TEST的命令格式在Intel白皮书1801页可以发现：
			//     85 /r表示是"AND r32 with r/m32; set SF, ZF, PF according to result."。
			// TEST指令跟CMP指令相似，运算结果只改变相应的标志寄存器的位，不把值保存到第一个操作数中。
			// 例如：test ecx，ecx。指令这条指令只有ecx中的值为0时，标志寄存器ZF位才能为0。
			// TEST--Logical Compare
			// 85 /r	TEST r/m32,r32	AND r32 with r/m32,set SF,ZF,PF according to result		
			gen_opcodeOne(OPCODE_TEST_AND_R32_WITH_RM32); // 0x85);
			gen_modrm(ADDR_REG, storage_class, storage_class, NULL, 0);
			
			// 参考Jcc的命令格式在Intel白皮书1060页可以发现：
			//	   0F 8F cw/cd 表示是"Jump near if greater (ZF=0 and SF=OF)."。
			// 也就是根据标志寄存器ZF位进行跳转。
			//  Jcc--Jump if Condition Is Met
			// .....
			// 0F 8F cw/cd		JG rel16/32	jump near if greater(ZF=0 and SF=OF)
			// .....
			gen_opcodeTwo(OPCODE_JCC_JUMP_NEAR_IF_GREATER // 0x0f
							, 0x85 ^ inv);
			jmp_addr = make_jmpaddr_list(jmp_addr);
		}
	}
	operand_pop();
	return jmp_addr;
}

/***********************************************************
 * 功能:		生成函数开头代码
 * func_type:	函数类型
 * 这个函数的逻辑是这样的。就是这里不真的生成函数开头代码。
 * 只是记录函数体开始的位置，等到函数体结束时，在函数gen_epilog中填充。
 **********************************************************/
void gen_prologue(Type *func_type)
{
	int addr, align, size, func_call;
	int param_addr;
	Symbol * sym;
	Type * type;
	// 函数类型对象对应的符号表。
	sym = func_type->ref;
	// 获得函数的调用方式。默认为CDECL
	func_call = sym->storage_class;
	addr = 8;
	function_stack_loc  = 0;
	// 记录了函数体开始，以便函数体结束时填充函数头，因为那时才能确定开辟的栈大小。
	func_begin_ind = sec_text_opcode_offset;
	sec_text_opcode_offset += FUNC_PROLOG_SIZE;
	// 不支持返回结构体，可以返回结构体指针
	if (sym->typeSymbol.type == T_STRUCT)
	{
		print_error("Can not return T_STRUCT", get_current_token());
	}
	//  如果函数有参数，处理参数定义
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
		// 计算栈内偏移。作为符号的关联值。
		param_addr = addr;
		addr += size;
		// 把参数符号放在符号栈
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
void gen_epilogue()
{
	int reserved_stack_size, saved_ind, opc;
	// 生成函数结尾指令。包括三条：
	//    参考MOV的命令格式在Intel白皮书1161页可以发现：
	//        8B表示是"Move r/m32 to r32."。
	//    8B /r	mov r32,r/m32	mov r/m32 to r32
	gen_opcodeOne((char)OPCODE_MOV_RM32_TO_R32); // 0x8B);
	/* 4. MOV ESP, EBP*/
	gen_modrm(ADDR_REG, REG_ESP, REG_EBP, NULL, 0);

	// 参考POP的命令格式在Intel白皮书1510页可以发现：
	//     58 + rd表示是"Pop top of stack into r32; increment stack pointer."。
	// POP--Pop a Value from the Stack
	// 58+	rd		POP r32		
	//    POP top of stack into r32; increment stack pointer
	/* 5. POP EBP */
	gen_opcodeOne(OPCODE_POP_STACK_TOP_INTO_R32 // 0x58
					+ REG_EBP);
	// KW_CDECL
	if (func_ret_sub == 0)
	{
		// 参考RET的命令格式在Intel白皮书1675页可以发现：
		//     C3表示是"Near return to calling procedure."。
		// 6. RET--Return from Procedure
		// C3	RET	near return to calling procedure
		gen_opcodeOne(OPCODE_NEAR_RETURN); // 0xC3);
	}
	// KW_STDCALL
	else
	{
		// 参考RET的命令格式在Intel白皮书1675页可以发现：
		//     C2 iw表示是"Near return to calling procedure and pop imm16 bytes from stack."。
		// 6. RET--Return from Procedure
		//    C2 iw	RET imm16	near return to calling procedure 
		//                      and pop imm16 bytes form stack
		gen_opcodeOne(OPCODE_NEAR_RETURN_AND_POP_IMM16_BYTES); // 0xC2);
		gen_byte(func_ret_sub);
		gen_byte(func_ret_sub >> 8);
	}
	reserved_stack_size = calc_align(-function_stack_loc, 4);
	// 把ind设置为之前记录函数体开始的位置。
	saved_ind = sec_text_opcode_offset;
	sec_text_opcode_offset = func_begin_ind;

	// 参考PUSH的命令格式在Intel白皮书1633页可以发现：
	//     50+rd表示是"Push r32."。
	// 1. PUSH--Push Word or Doubleword Onto the Stack
	//    50+rd	PUSH r32	Push r32
	gen_opcodeOne(OPCODE_PUSH_R32 // 0x50
						+ REG_EBP);

	// 参考MOV的命令格式在Intel白皮书1161页可以发现：
	//     89 /r表示是"Move r32 to r/m32."。
	// 89 /r	MOV r/m32,r32	Move r32 to r/m32
	// 2. MOV EBP, ESP
	gen_opcodeOne(OPCODE_MOVE_R32_TO_RM32); // 0x89);
	gen_modrm(ADDR_REG, REG_ESP, REG_EBP, NULL, 0);

	// 参考SUB的命令格式在Intel白皮书1776页可以发现：
	//     81 /5 id表示是"Subtract imm32 from r/m32."。
	//SUB--Subtract		81 /5 id	SUB r/m32,imm32	
	//         Subtract sign-extended imm32 from r/m32
	// 3. SUB ESP, stacksize
	gen_opcodeOne(OPCODE_SUBTRACT_IMM32_FROM_RM32); // 0x81);
	opc = 5;
	gen_modrm(ADDR_REG, opc, REG_ESP, NULL, 0);
	gen_dword(reserved_stack_size);

	// 恢复ind的值。
	sec_text_opcode_offset = saved_ind;
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
		gen_opcodeOne(ONE_BYTE_OPCODE_IMME_GRP_TO_IMM8);	// ADD esp,val
		gen_modrm(ADDR_REG,opc,REG_ESP,NULL,0);
        gen_byte(val);
    } 
	else 
	{
		// ADD--Add	81 /0 id	ADD r/m32,imm32	Add sign-extended imm32 to r/m32
		gen_opcodeOne(ONE_BYTE_OPCODE_IMME_GRP_TO_IMM32);	// add esp, val
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
	int size, reg_idx, args_size, idx, func_call;
	args_size = 0;
	// 参数依次入栈。
	// 这里有一个问题。就是我们其实只有四个寄存器。
	// 如果函数参数多于四个怎么办？？
	for (idx = 0; idx < nb_args; idx++)
	{
		reg_idx = load_one(REG_ANY, operand_stack_top);
		size =4;
		// 参考PUSH的命令格式在Intel白皮书1633页可以发现：
		//     50+rd表示是"Push r32."。
		// PUSH--Push Word or Doubleword Onto the Stack
		// 50+rd	PUSH r32	Push r32
		gen_opcodeOne(OPCODE_PUSH_R32 // 0x50
						+ reg_idx);
		args_size += size;
		operand_pop();
	}
	// 将占用的寄存器全部溢出到栈中。
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
	int reg_idx;
	if (operand_stack_top->storage_class & (SC_VALMASK | SC_LVAL) == SC_GLOBAL)
	{
		// 记录重定位信息
		coffreloc_add(sec_text, operand_stack_top->sym, 
			sec_text_opcode_offset + 1, IMAGE_REL_I386_REL32);
			
		// 参考CALL的命令格式在Intel白皮书695页可以发现：
		//     E8 cd表示是"Call near, relative, displacement relative to next instruction. "。
		//	CALL--Call Procedure E8 cd   
		//	CALL rel32    call near,relative,displacement relative to next instrution
		gen_opcodeOne(OPCODE_CALL_NEAR_RELATIVE_32_BIT_DISPLACE); // 0xe8);
		gen_dword(operand_stack_top->operand_value - 4);
	}
	else
	{
		reg_idx = load_one(REG_ANY, operand_stack_top);
		// 参考CALL的命令格式在Intel白皮书695页可以发现：
		//     FF /2表示是" Call near, absolute indirect, address given in r/m32."。
        // FF /2 CALL r/m32 Call near, absolute indirect, address given in r/m32
		gen_opcodeOne(OPCODE_CALL_NEAR_ABSOLUTE); // 0xff);
		gen_opcodeOne(OPCODE_CALL_NEAR_ABSOLUTE_32_RM32_ADDRESS // 0xd0
						+ reg_idx);
	}
}

/************************************************************************/
/* 一条形如 char str1[]  = "str1"; 的语句包括如下的指令。               */
/*    MOV ECX,  5                                                       */
/*    MOV ESI,  scc_anal.00403015; ASCII"strl"                          */
/*    LEA EDI,  DWORD PTR SS: [EBP-9]                                   */
/*    REP MOVS  BYTE PTR ES: [EDI], BYTE PTR DS: [ESI]                  */
/* 对应的机器码如下：                                                   */
/*    B9 05000000                                                       */
/*    BE 15304000                                                       */
/*    8D7D F7                                                           */
/*    F3: A4                                                            */
/************************************************************************/
void array_initialize()
{
	// 空出REG_ECX作为赋值次数。
	spill_reg(REG_ECX);

	/*    MOV ECX,  5 对应的机器码如下：        */
	/*    B9 05000000                           */
	// 参考MOV的命令格式在Intel白皮书1161页可以发现：
	//     0x0xB8表示是"Move imm32 to r32."。 
	gen_opcodeOne(OPCODE_MOVE_IMM32_TO_R32 + REG_ECX);
	gen_dword(operand_stack_top->type.ref->related_value);
	
	/*    MOV ESI,  scc_anal.00403015; ASCII"strl"                          */
	/*    BE 15304000                                                       */
	gen_opcodeOne(OPCODE_MOVE_IMM32_TO_R32 + REG_ESI);
	gen_addr32(operand_stack_top->storage_class, operand_stack_top->sym, operand_stack_top->operand_value);
	operand_swap();
	
	/*    LEA EDI,  DWORD PTR SS: [EBP-9] 对应的机器码如下：   */
	/*    8D7D F7                                              */
	// 参考LEA的命令格式在Intel白皮书1101页可以发现：
	//     0xB8表示是"Store effective address for m in register r32."。
	gen_opcodeOne(OPCODE_LEA_EFFECTIVE_ADDRESS_IN_R32);
	gen_modrm(ADDR_OTHER, REG_EDI, SC_LOCAL, operand_stack_top->sym, operand_stack_top->operand_value);
	/* REP MOVS  BYTE PTR ES: [EDI], BYTE PTR DS: [ESI]对应的机器码如下：   */
	/*    F3: A4                                                            */
	// 参考REP MOVS的命令格式在Intel白皮书1671页可以发现：
	//     F3 A4表示是"Move (E)CX bytes from DS:[(E)SI] to ES:[(E)DI]."。
	gen_prefix((char)OPCODE_REP_PREFIX);
	gen_opcodeOne((char)OPCODE_MOVE_ECX_BYTES_DS_ESI_TO_ES_EDI);
	// 弹出ECX和ESI
	operand_stack_top -= 2;
}


