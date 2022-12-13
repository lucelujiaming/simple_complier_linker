#include "get_token.h"
#include "symbol_table.h"
#include "x86_generator.h"
#include "reg_manager.h"

extern int function_stack_loc ;		// 局部变量在栈中位置

extern Type char_pointer_type,		// 字符串指针
			 int_type,				// int类型
			 default_func_type;		// 缺省函数类型

/************************************************************************/
/* 功能：寄存器分配，如果所需寄存器被占用，先将其内容溢出到栈中         */
/* rc：寄存器类型                                                       */
/************************************************************************/
#define REG_NOT_USED    0
#define REG_USED        1
int allocate_reg(int reqire_reg)
{
	int reg_index = 0;
	std::vector<Operand>::iterator iter;
	int used;

    /* 查找空闲的寄存器 */
	// 查找四个基础寄存器。
	for (reg_index = REG_EAX; reg_index <= REG_EBX; reg_index++)
	{
		// 如果可以分配任意一个，或者指定了基础寄存器。
		if (reqire_reg & REG_ANY || reg_index == reqire_reg) 
		{
			// 首先假设没有被使用。
			used = REG_NOT_USED;
			// 查找操作数栈
			for (iter = operand_stack.begin(); iter != operand_stack_top; iter++)
			{
				// 如果发现该寄存器已经使用。
				if ((iter->storage_class & SC_VALMASK) == reg_index)
				{
					used = REG_USED;
				}
			}
			// 如果寄存器没有使用。直接返回。
			if (used == REG_NOT_USED)
			{
				return reg_index;
			}
		}
	}

    // 如果没有空闲的寄存器，从操作数栈底开始查找到第一个占用的寄存器，
	// 也就是最早被使用的寄存器。举出到栈中。
	for (iter = operand_stack.begin(); iter != operand_stack_top; iter++)
	{
		// 获得寄存器或存储类型。
		reg_index = iter->storage_class & SC_VALMASK;
		if (reg_index < SC_GLOBAL &&			// 如果是寄存器 
			(reqire_reg & REG_ANY 
				|| reg_index == reqire_reg))	// 而且符合请求的寄存器类型 
		{
			spill_reg(reg_index);
			return reg_index;
		}
	}
    /* 此处永远不可能到达 */
	return -1;
}

/************************************************************************/
/* 功能：     将寄存器'reg_index'溢出到内存栈中，                       */
/*            并且标记释放'reg_index'寄存器的操作数为局部变量。         */
/* reg_index：寄存器编码                                                */
/************************************************************************/
void spill_reg(char reg_index)
{
	int size, align;
	std::vector<Operand>::iterator iter;
	Operand opd;
	Type * type;
	// 从操作数栈底开始查找到栈顶。
	for (iter = operand_stack.begin(); 
		iter != operand_stack_top; iter++)
	{
		// 找到第一个占用的寄存器。
		if ((iter->storage_class & SC_VALMASK) == reg_index)
		{
			// 这一步好像多余了。
			reg_index = iter->storage_class & SC_VALMASK;
			// 取出占用寄存器的数据类型。
			type = &iter->typeOperand;
			// 左值都是整数类型。
			if (iter->storage_class & SC_LVAL)
			{
				type = &int_type;
			}
			// 计算类型大小。
			size = type_size(type, &align);
			// 计算局部变量在栈中位置。
			function_stack_loc = calc_align(function_stack_loc - size, align);
			// 给opd赋值。
			operand_assign(&opd, type->typeCode, SC_LOCAL | SC_LVAL, function_stack_loc);
			// 举出到栈中。将寄存器'reg_index'中的值存入操作数'opd'。
			store(reg_index, &opd);
			// 如果是左值。
			if (iter->storage_class & SC_LVAL)
			{
			    // 标识操作数放在栈中。清除低8位标志的基本类型。保留所有的扩展类型。
				// 基本类型设置为SC_LLOCAL标志，表明寄存器溢出存放栈中。
				iter->storage_class = (iter->storage_class & ~(SC_VALMASK)) | SC_LLOCAL;
			}
			else
			{
				iter->storage_class = SC_LOCAL | SC_LVAL;
			}
			// 关联值设定为局部变量在栈中位置。
			iter->operand_value = function_stack_loc;
			break;
		}
	}
}

/************************************************************************/
/* 功能：将占用的寄存器全部溢出到栈中                                   */
/************************************************************************/
void spill_regs()
{
	int reg_idx;
	std::vector<Operand>::iterator iter;
	for (iter = operand_stack.begin(); iter != operand_stack_top; iter++)
	{
		reg_idx = iter->storage_class & SC_VALMASK;
		if (reg_idx < SC_GLOBAL)
		{
			spill_reg(reg_idx);
		}
	}
}
