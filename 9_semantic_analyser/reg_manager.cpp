#include "get_token.h"
#include "symbol_table.h"
#include "x86_generator.h"
#include "reg_manager.h"

extern int function_stack_loc ;		// �ֲ�������ջ��λ��

extern Type char_pointer_type,		// �ַ���ָ��
			 int_type,				// int����
			 default_func_type;		// ȱʡ��������

/************************************************************************/
/* ���ܣ��Ĵ������䣬�������Ĵ�����ռ�ã��Ƚ������������ջ��         */
/* rc���Ĵ�������                                                       */
/************************************************************************/
int allocate_reg(int rc)
{
	int storage_class;
	std::vector<Operand>::iterator p;
	int used;

    /* ���ҿ��еļĴ��� */
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

    // ���û�п��еļĴ������Ӳ�����ջ�׿�ʼ���ҵ���һ��ռ�õļĴ����ٳ���ջ��
	for (p = operand_stack.begin(); p != operand_stack_top; p++)
	{
		storage_class = p->storage_class & SC_VALMASK;
		if (storage_class < SC_GLOBAL && (rc & REG_ANY || storage_class == rc))
		{
			spill_reg(storage_class);
			return storage_class;
		}
	}
    /* �˴���Զ�����ܵ��� */
	return -1;
}

/************************************************************************/
/* ���ܣ����Ĵ���'r'������ڴ�ջ�У�                                    */
/*       ���ұ���ͷ�'r'�Ĵ����Ĳ�����Ϊ�ֲ�����                        */
/* r���Ĵ�������                                                        */
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
			    // ��ʶ����������ջ��
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
/* ���ܣ���ռ�õļĴ���ȫ�������ջ��                                   */
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
