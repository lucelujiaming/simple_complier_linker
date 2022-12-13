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
#define REG_NOT_USED    0
#define REG_USED        1
int allocate_reg(int reqire_reg)
{
	int reg_index = 0;
	std::vector<Operand>::iterator iter;
	int used;

    /* ���ҿ��еļĴ��� */
	// �����ĸ������Ĵ�����
	for (reg_index = REG_EAX; reg_index <= REG_EBX; reg_index++)
	{
		// ������Է�������һ��������ָ���˻����Ĵ�����
		if (reqire_reg & REG_ANY || reg_index == reqire_reg) 
		{
			// ���ȼ���û�б�ʹ�á�
			used = REG_NOT_USED;
			// ���Ҳ�����ջ
			for (iter = operand_stack.begin(); iter != operand_stack_top; iter++)
			{
				// ������ָüĴ����Ѿ�ʹ�á�
				if ((iter->storage_class & SC_VALMASK) == reg_index)
				{
					used = REG_USED;
				}
			}
			// ����Ĵ���û��ʹ�á�ֱ�ӷ��ء�
			if (used == REG_NOT_USED)
			{
				return reg_index;
			}
		}
	}

    // ���û�п��еļĴ������Ӳ�����ջ�׿�ʼ���ҵ���һ��ռ�õļĴ�����
	// Ҳ�������类ʹ�õļĴ������ٳ���ջ�С�
	for (iter = operand_stack.begin(); iter != operand_stack_top; iter++)
	{
		// ��üĴ�����洢���͡�
		reg_index = iter->storage_class & SC_VALMASK;
		if (reg_index < SC_GLOBAL &&			// ����ǼĴ��� 
			(reqire_reg & REG_ANY 
				|| reg_index == reqire_reg))	// ���ҷ�������ļĴ������� 
		{
			spill_reg(reg_index);
			return reg_index;
		}
	}
    /* �˴���Զ�����ܵ��� */
	return -1;
}

/************************************************************************/
/* ���ܣ�     ���Ĵ���'reg_index'������ڴ�ջ�У�                       */
/*            ���ұ���ͷ�'reg_index'�Ĵ����Ĳ�����Ϊ�ֲ�������         */
/* reg_index���Ĵ�������                                                */
/************************************************************************/
void spill_reg(char reg_index)
{
	int size, align;
	std::vector<Operand>::iterator iter;
	Operand opd;
	Type * type;
	// �Ӳ�����ջ�׿�ʼ���ҵ�ջ����
	for (iter = operand_stack.begin(); 
		iter != operand_stack_top; iter++)
	{
		// �ҵ���һ��ռ�õļĴ�����
		if ((iter->storage_class & SC_VALMASK) == reg_index)
		{
			// ��һ����������ˡ�
			reg_index = iter->storage_class & SC_VALMASK;
			// ȡ��ռ�üĴ������������͡�
			type = &iter->typeOperand;
			// ��ֵ�����������͡�
			if (iter->storage_class & SC_LVAL)
			{
				type = &int_type;
			}
			// �������ʹ�С��
			size = type_size(type, &align);
			// ����ֲ�������ջ��λ�á�
			function_stack_loc = calc_align(function_stack_loc - size, align);
			// ��opd��ֵ��
			operand_assign(&opd, type->typeCode, SC_LOCAL | SC_LVAL, function_stack_loc);
			// �ٳ���ջ�С����Ĵ���'reg_index'�е�ֵ���������'opd'��
			store(reg_index, &opd);
			// �������ֵ��
			if (iter->storage_class & SC_LVAL)
			{
			    // ��ʶ����������ջ�С������8λ��־�Ļ������͡��������е���չ���͡�
				// ������������ΪSC_LLOCAL��־�������Ĵ���������ջ�С�
				iter->storage_class = (iter->storage_class & ~(SC_VALMASK)) | SC_LLOCAL;
			}
			else
			{
				iter->storage_class = SC_LOCAL | SC_LVAL;
			}
			// ����ֵ�趨Ϊ�ֲ�������ջ��λ�á�
			iter->operand_value = function_stack_loc;
			break;
		}
	}
}

/************************************************************************/
/* ���ܣ���ռ�õļĴ���ȫ�������ջ��                                   */
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
