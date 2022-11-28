#include "operand_stack.h"

#define OPERAND_STACK_SIZE    1024

extern std::vector<Operand> operand_stack;
std::vector<Operand>::iterator operand_stack_top = NULL;
std::vector<Operand>::iterator operand_stack_last_top = NULL;

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
		printf("No memory.\n");
	}
	// Save operand_stack_last_top before
	operand_stack_last_top = operand_stack_top;

	operand_stack_top++;
	operand_stack_top->type             = *type;
	operand_stack_top->storage_class    = storage_class;
	operand_stack_top->operand_value    = operand_value;
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