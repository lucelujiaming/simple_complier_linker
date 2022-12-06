#include "instruction_operator.h"
#include "reg_manager.h"
#include "x86_generator.h"

extern Type int_type;			// int类型

// 包裹函数。
/************************************************************************/
/*  功能：   将栈顶操作数加载到'rc'类寄存器中。                         */
/*           但是，如果这个栈顶操作数已经在寄存器中，则什么都不用做。   */
/*  rc：     寄存器类型                                                 */
/*  opd：    操作数指针                                                 */
/*  返回值： 如果栈顶操作数不在寄存器中，返回为其分配的空闲寄存器。     */
/*           如果栈顶操作数已经在寄存器中，返回对应的空闲寄存器。       */
/************************************************************************/
int load_one(int rc, Operand * opd)
{
	int storage_class ;
	// 获得存储类型。
	storage_class = opd->storage_class & SC_VALMASK;
	// 需要加载到寄存器中情况：
	// 1.栈顶操作数目前尝未分配寄存器
	// 2.栈顶操作数已分配寄存器，但为左值 *p
	if (storage_class > SC_GLOBAL ||			// 未分配寄存器。
		(opd->storage_class & SC_LVAL))			// 为左值。
	{
		storage_class = allocate_reg(rc);		// 分配一个空闲的寄存器。
		// 现在我们得到了寄存器，我们只需要将栈顶操作数加载到这个寄存器中。
		load(storage_class, opd);
	}
	// 如果需要加载到寄存器，把存储类型修改为寄存器。
	// 如果不需要加载到寄存器，我们会清除扩展类型。
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
/*     也就是我们在代码中使用的赋值操作。                               */
/*     通过阅读下面的例子，可以看出来栈顶保存的是右值。                 */
/*     而次栈顶保存的是左值。这和我们的语法解析恰好也是对应的。         */
/*     我们在处理赋值语句的时候，一定是先获得左值，之后压栈。           */
/*     之后获得赋值等号，让自己进入最后获得右值。                       */
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
	int right_storage_class = 0, left_storage_class = 0;
	// 取出位于栈顶的右值，生成机器码。同时返回保存的寄存器。
	right_storage_class = load_one(REG_ANY, operand_stack_top);
	
	// 如果次栈顶操作数为寄存器溢出存放栈中。
	// 也就是说，如果左值现在被溢出到栈中，必须再次把他加载到寄存器中。
	if ((operand_stack_last_top->storage_class & SC_VALMASK) == SC_LLOCAL)
	{
		Operand opd;
		left_storage_class = allocate_reg(REG_ANY);
		operand_assign(&opd, T_INT, SC_LOCAL | SC_LVAL, 
			operand_stack_last_top->operand_value);
		load(left_storage_class, &opd);
		operand_stack_last_top->storage_class = left_storage_class | SC_LVAL;
	}
	// 生成将寄存器right_storage_class中的值存入操作数operand_stack_last_top的机器码。
	store(right_storage_class, operand_stack_last_top);
	// 就交换栈顶操作数和次栈顶操作数。
	operand_swap();
	// 弹出上面交换过来的次栈顶操作数。和上面的操作结合等于删除次栈顶操作数。
	operand_pop();
}

/************************************************************************/
/* 功能：生成二元运算，对指针操作数进行一些特殊处理                     */
/* op：运算符类型。输入类型为e_TokenCode。包括关系运算和数学运算。      */
/************************************************************************/
/* 这个函数有这么几种用法:                                              */
/*   1. 在表达式解析的过程中，生成对应数学运算的代码。                  */
/*   2. 处理结构体成员的时候，执行gen_op(TK_PLUS)计算成员变量偏移。     */
/*   3. 处理数组下标的时候，执行gen_op(TK_PLUS)计算数组偏移地址。       */
/************************************************************************/
void gen_op(int op)
{
	int type_size, btLastTop, btTop;
	Type typeOne;

	btLastTop = operand_stack_last_top->type.type & T_BTYPE;
	btTop = operand_stack_top->type.type & T_BTYPE;

	// 如果比较的两个元素有一个是指针。
	if (btLastTop == T_PTR || btTop == T_PTR)
	{
		// 如果是比较大小。
		if (op >= TK_EQ && op <= TK_GEQ)   // >,<,>=.<=...
		{
			// 生成机器码。
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
			type_size = pointed_size(&operand_stack_last_top->type);
			// 生成机器码。
			gen_opInteger(op);
			// 两个指针的地址差为整数类型。
			operand_stack_top->type.type = T_INT;
			// 结果还需要除以整数大小，也就是4。
			operand_push(&int_type, SC_GLOBAL, type_size);
			gen_op(TK_DIVIDE);
		}
		/************************************************************************/
		/* 两个操作数一个是指针，另一个不是指针，并且非关系运算。为指针移动     */
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
		// 如果是关系运算
		if (op >= TK_EQ && op <= TK_GEQ)   // >,<,>=.<=...
		{
			// 关系运算结果为T_INT类型，是整数。
			operand_stack_top->type.type = T_INT;
		}

	}
}

