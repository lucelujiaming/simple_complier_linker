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
int allocate_reg(int rc)
{
	int storage_class;
	std::vector<Operand>::iterator p;
	int used;

    /* 查找空闲的寄存器 */
	for (storage_class = 0; storage_class <= REG_EBX; storage_class++)
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

    // 如果没有空闲的寄存器，从操作数栈底开始查找到第一个占用的寄存器举出到栈中
	for (p = operand_stack.begin(); p != operand_stack_top; p++)
	{
		storage_class = p->storage_class & SC_VALMASK;
		if (storage_class < SC_GLOBAL && (rc & REG_ANY || storage_class == rc))
		{
			spill_reg(storage_class);
			return storage_class;
		}
	}
    /* 此处永远不可能到达 */
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
			function_stack_loc = calc_align(function_stack_loc - size, align);
			operand_assign(&opd, type->type, SC_LOCAL | SC_LVAL, function_stack_loc);
			store(storage_class, &opd);
			if (p->storage_class & SC_LVAL)
			{
			    // 标识操作数放在栈中
				p->storage_class = (p->storage_class & ~(SC_VALMASK)) | SC_LLOCAL;
			}
			else
			{
				p->storage_class = SC_LOCAL | SC_LVAL;
			}
			p->operand_value = function_stack_loc;
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
