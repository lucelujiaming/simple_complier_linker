#ifndef OPERAND_STACK_H
#define OPERAND_STACK_H

#include "get_token.h"

#define OPSTACK_SIZE 256 // + 1

/* 
  这个东西有两个作用。第一个作用就是用于进行运算。例如：
      a = b + c * d;
  因为我们需要首先计算乘法，因此上，我们需要把b压栈，之后把c压栈，把d压栈，之后计算c * d。
  把得到的结果放在c中，之后把d弹出来。之后使用放在c中结果和b进行加法运算。
  这是表达式分析的一个自然的流程。
  但是，因为这个流程其实体现的是操作数和被操作数的关系。
  也就是让操作数和被操作数进行运算，结果放在被操作数中。
  因此上，这个流程还可以扩展到诸如赋值，关系运算等很多方面。
  
  */
typedef struct Operand_t{
	Type typeOperand;				// 数据类型
	unsigned short storage_class;   // 寄存器或存储类型 e_StorageClass
	
	int operand_value;				// 关联值，适用于SC_GLOBAL。
									// 如果这个变量的寄存器或存储类型是：
	                                // 1. 立即数，那就是立即数本身。
	                                // 2. 栈中变量，那就是栈内地址。
	                                // 3. 寄存器溢出存放栈中，那就是局部变量在栈中位置。
	                                // 4. 使用标志寄存器，
	                                //    那就是在gen_opTwoInteger中生成的比较结果。
									// 如果是符号引用，常量值无效。
	struct Symbol * sym;			// 关联符号，适用于(SC_SYM | SC_GLOBAL)
	// 调试用数据
	char token_str[128];
} Operand;

// Operand operation functions
void operand_push(Type* type, int r, int operand_value);
void operand_pop();
void operand_swap();
void operand_assign(Operand *opd, int token_code, int storage_class, int operand_value);

void check_leftvalue();
void cancel_lvalue();
#endif

