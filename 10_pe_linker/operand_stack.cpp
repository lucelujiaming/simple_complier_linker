#include "operand_stack.h"

#define OPERAND_STACK_SIZE    1024

std::vector<Operand> operand_stack;
std::vector<Operand>::iterator operand_stack_top = NULL;
std::vector<Operand>::iterator operand_stack_last_top = NULL;
// 调试用数据。
int operand_stack_count = 0;
/************************************************************************/
/*  功能：操作数入栈                                                    */
/*  type：操作数数据类型                                                */
/*  r：操作数存储类型                                                   */
/*  value：操作数值                                                     */
/************************************************************************/
void operand_push(Type* type, int storage_class, int operand_value)
{
	if (operand_stack_top == operand_stack.end())
	{
		// printf("No memory.\n");
	}
	// Save operand_stack_last_top before
	operand_stack_last_top = operand_stack_top;

	operand_stack_top++;
	operand_stack_top->type             = *type;
	operand_stack_top->storage_class    = storage_class;
	operand_stack_top->operand_value    = operand_value;
	if (get_current_token())
	{
		strcpy(operand_stack_top->token_str, get_current_token());
	}
	operand_stack_count++;
}

/************************************************************************/
/* 功能：弹出栈顶操作数                                                 */
/************************************************************************/
void operand_pop()
{
	operand_stack_top--;
	
	if (operand_stack_top == operand_stack.begin())
		operand_stack_last_top = NULL;
	else
	{
		operand_stack_last_top = operand_stack_top;
		operand_stack_last_top--;
	}
	operand_stack_count--;
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
		*operand_stack_top = *operand_stack_last_top;
		*operand_stack_last_top = tmp;
	}
}

void check_leftvalue()
{
    // 如果不是左值就报错。
	if (!(operand_stack_top->storage_class & SC_LVAL))
	{
		print_error("Need left_value", get_current_token());
	}
}

void cancel_lvalue()
{
	// 判断是否为左值。
	check_leftvalue();
	// 清除左值标志。
	operand_stack_top->storage_class &= ~SC_LVAL;
}

/************************************************************************/
/*  功能：操作数赋值                                                    */
/*  t：操作数数据类型                                                   */
/*  r：操作数存储类型                                                   */
/*  value：操作数值                                                     */
/************************************************************************/
void operand_assign(Operand * opd, int token_code, int storage_class, int operand_value)
{
	opd->type.type     = token_code;
	opd->storage_class = storage_class;
	opd->operand_value = operand_value;
}