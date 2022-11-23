// x86_generator.cpp : Defines the entry point for the console application.
//

#include "token_code.h"
#include "get_token.h"
#include "symbol_table.h"
#include <windows.h>
#include "x86_generator.h"
#include "reg_manager.h"

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

// 包裹函数。
/************************************************************************/
/*  功能：将栈顶操作数加载到'rc'类寄存器中                              */
/*  rc：寄存器类型                                                      */
/*  opd：操作数指针                                                     */
/************************************************************************/
int load_one(int rc, Operand * opd)
{
	int storage_class ;
	// 获得存储类型。
	storage_class = opd->storage_class & SC_VALMASK;
	// 需要加载到寄存器中情况：
	// 1.栈顶操作数目前尝未分配寄存器
	// 2.栈顶操作数已分配寄存器，但为左值 *p
	if (storage_class > SC_GLOBAL ||			// 不是全局变量。
		(opd->storage_class & SC_LVAL))			// 为左值。
	{
		storage_class = allocate_reg(rc);		// 分配一个空闲的寄存器。
		// 现在我们得到了寄存器，我们只需要将栈顶操作数加载到这个寄存器中。
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
	// 8B 45 DC 
	load_one(rc2, operand_stack_top);
	// 8B 4D DC
	load_one(rc2, operand_stack_last_top);
}

/************************************************************************/
/* 功能：将栈顶操作数存入次栈顶操作数中。                               */
/*     也就是把栈里面的第零个元素存入第一个元素。                       */
/*     这就是store_zero_to_one的含义。                                  */
/*     通过阅读下面的例子，可以看出来栈顶保存的是右值。                 */
/*     而次栈顶保存的是左值。这和我们的语法解析恰好也是对应的。         */
/*     我们在处理赋值语句的时候，一定是先获得左值，之后压栈。           */
/*     之后获得赋值等号，让自己进入最后获得右值。在                                           */
/************************************************************************/
/* 一条形如 char a='a'; 的语句包括如下的指令。                          */
/*  1. MOV EAX, 61                                                      */
/*  2. MOV BYTE PTR SS: [EBP-1], AL                                     */
/* 一条形如 short b=6; 的语句包括如下的指令。                           */
/*  1. MOV EAX, 6                                                       */
/*  2. MOV WORD PTR SS: [EBP-2], AX                                     */
/* 一条形如 int c=8; 的语句包括如下的指令。                             */
/*  1. MOV EAX, 8                                                       */
/*  2. MOV DWORD PTR SS: [EBP-4], EAX                                   */
/* 一条形如 char str1[] = "abe"; 的语句包括如下的指令。                 */
/*  1. MOV ECX, 4                                                       */
/*  2. MOV ESI.scc_anal.00403000; ASCII"abe"                            */
/*  3. LEA EDI, DWORD PTR SS: [EBP-8]                                   */
/*  4. REP MOVS BYTE PTR ES: [EDI], BYTE PTR DS: [ESI]                  */
/*  这里的REP表示重复执行后面的MOV指令。                                */
/* 这里用到了变址寄存器(Index Register)ESI和EDI，它们主要用于存放存储   */
/* 单元在段内的偏移量，用它们可实现多种存储器操作数的寻址方式，为以不同 */
/* 的地址形式访问存储单元提供方便。这里用于字符串操作指令的执行过程。   */
/************************************************************************/
/* 一条形如 char* str2 = "XYZ"; 的语句包括如下的指令。                  */
/*  1. MOV EAX, scc_anal.00403004; ASCII"XYZ"                           */
/*  2. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/************************************************************************/
void store_zero_to_one()
{
	// 根据上面的注释可以看出来，这个赋值操作包含两个部分：
	// 一个是取出右值。一个是放入左值所在的内存空间。
	int storage_class = 0,t = 0;
	// 取出位于栈顶的右值，生成机器码。同时返回保存的寄存器。
	storage_class = load_one(REG_ANY, operand_stack_top);
	// 如果次栈顶操作数为寄存器溢出存放栈中。
	if ((operand_stack_last_top->storage_class & SC_VALMASK) == SC_LLOCAL)
	{
		Operand opd;
	//	t = allocate_reg(REG_ANY);
		operand_assign(&opd, T_INT, SC_LOCAL | SC_LVAL, 
			operand_stack_last_top->value);
		load(t, &opd);
		operand_stack_last_top->storage_class = t | SC_LVAL;
	}
	// 生成将寄存器'r'中的值存入操作数'opd'的机器码。
	store(storage_class, operand_stack_last_top);
	// 就交换栈顶操作数和次栈顶操作数。
	operand_swap();
	// 弹出上面交换过来的次栈顶操作数。和上面的操作结合等于删除次栈顶操作数。
	operand_pop();
}

// 生成函数
/************************************************************************/
/* 功能：生成二元运算，对指针操作数进行一些特殊处理                     */
/* op：运算符类型                                                       */
/************************************************************************/
/* 这个函数有这么几种用法:                                              */
/*   1. 在表达式解析的过程中，生成对应数学运算的代码。                  */
/*   2. 处理结构体成员的时候，执行gen_op(TK_PLUS)计算成员变量偏移。     */
/*   3. 处理数组下标的时候，执行gen_op(TK_PLUS)计算数组偏移地址。       */
/************************************************************************/
void gen_op(int op)
{
	int u, btLastTop, btTop;
	Type typeOne;

	btLastTop = operand_stack_last_top->type.type & T_BTYPE;
	btTop = operand_stack_top->type.type & T_BTYPE;

	// 如果比较的两个元素有一个是指针。
	if (btLastTop == T_PTR || btTop == T_PTR)
	{
		// 如果是比较大小。
		if (op >= TK_EQ && op <= TK_GEQ)   // >,<,>=.<=...
		{
			gen_opInteger(op);
			operand_stack_top->type.type = T_INT;
		}
		// 两个操作数都为指针。说明是求两个指针的地址差。例如：
		//   char * ptr_one, * ptr_two;
		//   char * test_one = "11111111111111111111111111111111" ;
		//   char * test_two = "22222222" ;
		//   ptr_one = test_one;
		//   ptr_two = test_one + strlen(test_two);
		//   int iDiff = ptr_two - ptr_one ;
		else if (btLastTop == T_PTR && btTop == T_PTR)
		{
			if (op != TK_MINUS)
			{
				printf("Only support - and >,<,>=.<= \n");
			}
			// 取出被操作数的大小。例如char * ptr_one的大小就是1。
			u = pointed_size(&operand_stack_last_top->type);
			gen_opInteger(op);
			operand_stack_top->type.type = T_INT;
			operand_push(&int_type, SC_GLOBAL, 
				pointed_size(&operand_stack_last_top->type));
			gen_op(TK_DIVIDE);
		}
		/************************************************************************/
		/* 两个操作数一个是指针，另一个不是指针，并且非关系运算。               */
		/* 例如对于a[3]来说，a就是指针，3则不是指针。                           */
		/* 此时栈顶元素为数组下标3，类型为T_INT。                               */
		/* 次栈顶元素为数组变量a，类型为T_PTR。                                 */
		/************************************************************************/
		/* 下面是一个例子。请看arr[i] = i中的左值部分arr[i]对应的指令:          */
		/*   11. MOV  EAX, 4                                                    */
		/*   12. MOV  ECX, DWORD PTR SS: [EBP-4]                                */
		/*   13. IMUL ECX, EAX                                                  */
		/*   14. LEA  EAX, DWORD PTR SS: [EBP-2C]                               */
		/*   15. ADD  EAX, ECX                                                  */
		/* 可以看到逻辑也非常简单。                                             */
		/*   11. 首先EAX设置为4，                                               */
		/*   12. 之后取出i的值放入ECX。                                         */
		/*   13. 之后把ECX和EAX相乘，放入ECX。                                  */
		/*   14. 之后取出数组首地址放入EAX。                                    */
		/*   15. 之后加上ECX，得到了r[i]的地址。结果放入EAX。                   */
		/************************************************************************/
		/* 顺便说一句，下面是arr[i] = i中的对数组元素赋值对应的指令:            */
		/*   16. MOV  ECX, DWORD PTR SS: [EBP-4]                                */
		/*   17. MOV  DWORD PTR DS: [EAX], ECX                                  */
		/* 可以看到逻辑也非常简单。                                             */
		/*   16. 取出i的值放入ECX。                                             */
		/*   17. 完成arr[i] = i的赋值操作。                                     */
		/************************************************************************/
		else 
		{
			if (op != TK_MINUS && op != TK_PLUS)
			{
				printf("Only support +- and >,<,>=.<= \n");
			}
			if (btTop == T_PTR)
			{
				operand_swap();
			}
			// 得到次栈顶元素数组变量的类型，把类型作为一个全局变量压栈。
			typeOne = operand_stack_last_top->type;
			operand_push(&int_type, SC_GLOBAL, 
				pointed_size(&operand_stack_last_top->type));
			
			// 生成乘法指令。让数组变量的类型乘上栈顶元素数组下标3。
			// 这样我们就计算出来了数组偏移地址。
			gen_op(TK_STAR);
			// 我们得到了数组偏移地址以后，就可以进行op指定的操作了。
			// 在这个场景下，op = TK_PLUS，也就是加法。
			gen_opInteger(op);
            // 变换类型为成员变量数据类型。因为数组是一个指针类型。
			// 而当我们对于数组取下标以后，我们的类型就变成了数组元素的类型。
			operand_stack_top->type = typeOne;
		}
	}
	// 如果都不是指针，那就是数学运算。
	else
	{
		// 生成数学运算对应的机器汇编码。
		gen_opInteger(op);
		// 运算结果是整数。
		if (op >= TK_EQ && op <= TK_GEQ)   // >,<,>=.<=...
		{
			operand_stack_top->type.type = T_INT;
		}

	}
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
	// 还是用 char a = g_char; 语句对应的机器码为例：
	// 	0FBE 0500204000
	// 前两个字节0FBE，前面已经生成好了。这里生成后面的部分。
	// 其中gen_byte生成05，gen_addr32生成后面的部分。
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
			// 还是用 char a = g_char; 语句对应的机器码为例：
			// 	0FBE 0500204000
			//  8845FF
			// 这里生成上面的第二句。这就是45的来源。
			// 0x45转成二进制就是0100 0101
			gen_byte(0x45 | reg_opcode);
			// 这里生成上面的第二句。这就是FF的来源。
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
		/* 一条形如 c = a * b; 的语句包括如下的指令。                           */
		/*  1. MOV  EAX,  DWORD PTR SS: [EBP-8]                                 */
		/*  2. MOV  ECX,  DWORD PTR SS: [EBP-4]                                 */
		/*  3. IMUL ECX,  EAX                                                   */
		/*  4. MOV  DWORD PTR SS: [EBP-C], ECX                                  */
		// 可以看到乘法操作分为三步。就是获得乘数，获得被乘数，执行IMUL完成乘法。
		// 获得乘数，获得被乘数。
		load_two(REG_ANY, REG_ANY);
		storage_class  = operand_stack_last_top->storage_class;
		fr = operand_stack_top->storage_class;
		operand_pop();
		// 执行IMUL完成乘法。
		gen_opcodeTwo(OPCODE_IMUL_HIGH_BYTE, OPCODE_IMUL_LOW_BYTE);
		// 生成IMUL需要的指令寻址方式。
		gen_modrm(ADDR_REG, storage_class, fr, NULL, 0);
		break;
	case TK_DIVIDE:
	case TK_MOD:
		opc = 7;
		load_two(REG_EAX, REG_ECX);
		storage_class  = operand_stack_last_top->storage_class;
		fr = operand_stack_top->storage_class;
		operand_pop();
		spill_reg(REG_EDX);
		gen_opcodeOne(OPCODE_CDQ);
		gen_opcodeOne(OPCODE_IDIV);
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
		storage_class = load_one(REG_ANY, operand_stack_last_top);
		c = operand_stack_top->value;
		if (c == (char)c)
		{
			// ADC--Add with Carry			83 /2 ib	ADC r/m32,imm8	Add with CF sign-extended imm8 to r/m32
			// ADD--Add						83 /0 ib	ADD r/m32,imm8	Add sign-extended imm8 from r/m32
			// SUB--Subtract				83 /5 ib	SUB r/m32,imm8	Subtract sign-extended imm8 to r/m32
			// CMP--Compare Two Operands	83 /7 ib	CMP r/m32,imm8	Compare imm8 with r/m32
			gen_opcodeOne(0x83);
			gen_modrm(ADDR_REG, opc, storage_class, NULL, 0);
			gen_byte(c);
		}
		else
		{
			// ADD--Add					    81 /0 id	ADD r/m32,imm32	Add sign-extended imm32 to r/m32
			// SUB--Subtract				81 /5 id	SUB r/m32,imm32	Subtract sign-extended imm32 from r/m32
			// CMP--Compare Two Operands	81 /7 id	CMP r/m32,imm32	Compare imm32 with r/m32
			gen_opcodeOne(0x81);
			gen_modrm(ADDR_REG, opc, storage_class, NULL, 0);
			gen_byte(c);
		}
	}
	else
	{
		load_two(REG_ANY, REG_ANY);
		storage_class  = operand_stack_last_top->storage_class;
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



