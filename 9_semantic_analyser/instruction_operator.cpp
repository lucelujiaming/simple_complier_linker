#include "instruction_operator.h"
#include "reg_manager.h"
#include "x86_generator.h"

extern Type int_type;			// int����

// ����������
/************************************************************************/
/*  ���ܣ�   ��ջ�����������ص�'rc'��Ĵ����С�                         */
/*           ���ǣ�������ջ���������Ѿ��ڼĴ����У���ʲô����������   */
/*  rc��     �Ĵ�������                                                 */
/*  opd��    ������ָ��                                                 */
/*  ����ֵ�� ���ջ�����������ڼĴ����У�����Ϊ�����Ŀ��мĴ�����     */
/*           ���ջ���������Ѿ��ڼĴ����У����ض�Ӧ�Ŀ��мĴ�����       */
/************************************************************************/
int load_one(int rc, Operand * opd)
{
	int storage_class ;
	// ��ô洢���͡�
	storage_class = opd->storage_class & SC_VALMASK;
	// ��Ҫ���ص��Ĵ����������
	// 1.ջ��������Ŀǰ��δ����Ĵ���
	// 2.ջ���������ѷ���Ĵ�������Ϊ��ֵ *p
	if (storage_class > SC_GLOBAL ||			// δ����Ĵ�����
		(opd->storage_class & SC_LVAL))			// Ϊ��ֵ��
	{
		storage_class = allocate_reg(rc);		// ����һ�����еļĴ�����
		// �������ǵõ��˼Ĵ���������ֻ��Ҫ��ջ�����������ص�����Ĵ����С�
		load(storage_class, opd);
	}
	// �����Ҫ���ص��Ĵ������Ѵ洢�����޸�Ϊ�Ĵ�����
	// �������Ҫ���ص��Ĵ��������ǻ������չ���͡�
	opd->storage_class = storage_class;
	return storage_class;
}


/******************************************************************************/
/* ���ܣ���ջ�����������ص�'rc1'��Ĵ���������ջ�����������ص�'rc2'��Ĵ���   */
/* rcl��ջ�����������ص��ļĴ�������                                          */
/* rc2����ջ�����������ص��ļĴ�������                                        */
/******************************************************************************/
void load_two(int rc1, int rc2)
{
	// 8B 45 DC 
	load_one(rc2, operand_stack_top);
	// 8B 4D DC
	load_one(rc2, operand_stack_last_top);
}

/************************************************************************/
/* ���ܣ���ջ�������������ջ���������С�                               */
/*     Ҳ���ǰ�ջ����ĵ����Ԫ�ش����һ��Ԫ�ء�                       */
/*     �����store_zero_to_one�ĺ��塣                                  */
/*     Ҳ���������ڴ�����ʹ�õĸ�ֵ������                               */
/*     ͨ���Ķ���������ӣ����Կ�����ջ�����������ֵ��                 */
/*     ����ջ�����������ֵ��������ǵ��﷨����ǡ��Ҳ�Ƕ�Ӧ�ġ�         */
/*     �����ڴ���ֵ����ʱ��һ�����Ȼ����ֵ��֮��ѹջ��           */
/*     ֮���ø�ֵ�Ⱥţ����Լ������������ֵ��                       */
/************************************************************************/
/* һ������ char a='a'; �����������µ�ָ�                          */
/*  1. MOV EAX, 61                                                      */
/*  2. MOV BYTE PTR SS: [EBP-1], AL                                     */
/* һ������ short b=6; �����������µ�ָ�                           */
/*  1. MOV EAX, 6                                                       */
/*  2. MOV WORD PTR SS: [EBP-2], AX                                     */
/* һ������ int c=8; �����������µ�ָ�                             */
/*  1. MOV EAX, 8                                                       */
/*  2. MOV DWORD PTR SS: [EBP-4], EAX                                   */
/* һ������ char str1[] = "abe"; �����������µ�ָ�                 */
/*  1. MOV ECX, 4                                                       */
/*  2. MOV ESI.scc_anal.00403000; ASCII"abe"                            */
/*  3. LEA EDI, DWORD PTR SS: [EBP-8]                                   */
/*  4. REP MOVS BYTE PTR ES: [EDI], BYTE PTR DS: [ESI]                  */
/*  �����REP��ʾ�ظ�ִ�к����MOVָ�                                */
/* �����õ��˱�ַ�Ĵ���(Index Register)ESI��EDI��������Ҫ���ڴ�Ŵ洢   */
/* ��Ԫ�ڶ��ڵ�ƫ�����������ǿ�ʵ�ֶ��ִ洢����������Ѱַ��ʽ��Ϊ�Բ�ͬ */
/* �ĵ�ַ��ʽ���ʴ洢��Ԫ�ṩ���㡣���������ַ�������ָ���ִ�й��̡�   */
/************************************************************************/
/* һ������ char* str2 = "XYZ"; �����������µ�ָ�                  */
/*  1. MOV EAX, scc_anal.00403004; ASCII"XYZ"                           */
/*  2. MOV DWORD PTR SS: [EBP-C], EAX                                   */
/************************************************************************/
void store_zero_to_one()
{
	// ���������ע�Ϳ��Կ������������ֵ���������������֣�
	// һ����ȡ����ֵ��һ���Ƿ�����ֵ���ڵ��ڴ�ռ䡣
	int right_storage_class = 0, left_storage_class = 0;
	// ȡ��λ��ջ������ֵ�����ɻ����롣ͬʱ���ر���ļĴ�����
	right_storage_class = load_one(REG_ANY, operand_stack_top);
	
	// �����ջ��������Ϊ�Ĵ���������ջ�С�
	// Ҳ����˵�������ֵ���ڱ������ջ�У������ٴΰ������ص��Ĵ����С�
	if ((operand_stack_last_top->storage_class & SC_VALMASK) == SC_LLOCAL)
	{
		Operand opd;
		left_storage_class = allocate_reg(REG_ANY);
		operand_assign(&opd, T_INT, SC_LOCAL | SC_LVAL, 
			operand_stack_last_top->operand_value);
		load(left_storage_class, &opd);
		operand_stack_last_top->storage_class = left_storage_class | SC_LVAL;
	}
	// ���ɽ��Ĵ���right_storage_class�е�ֵ���������operand_stack_last_top�Ļ����롣
	store(right_storage_class, operand_stack_last_top);
	// �ͽ���ջ���������ʹ�ջ����������
	operand_swap();
	// �������潻�������Ĵ�ջ����������������Ĳ�����ϵ���ɾ����ջ����������
	operand_pop();
}

/************************************************************************/
/* ���ܣ����ɶ�Ԫ���㣬��ָ�����������һЩ���⴦��                     */
/* op����������͡���������Ϊe_TokenCode��������ϵ�������ѧ���㡣      */
/************************************************************************/
/* �����������ô�����÷�:                                              */
/*   1. �ڱ��ʽ�����Ĺ����У����ɶ�Ӧ��ѧ����Ĵ��롣                  */
/*   2. ����ṹ���Ա��ʱ��ִ��gen_op(TK_PLUS)�����Ա����ƫ�ơ�     */
/*   3. ���������±��ʱ��ִ��gen_op(TK_PLUS)��������ƫ�Ƶ�ַ��       */
/************************************************************************/
void gen_op(int op)
{
	int type_size, btLastTop, btTop;
	Type typeOne;

	btLastTop = operand_stack_last_top->type.type & T_BTYPE;
	btTop = operand_stack_top->type.type & T_BTYPE;

	// ����Ƚϵ�����Ԫ����һ����ָ�롣
	if (btLastTop == T_PTR || btTop == T_PTR)
	{
		// ����ǱȽϴ�С��
		if (op >= TK_EQ && op <= TK_GEQ)   // >,<,>=.<=...
		{
			// ���ɻ����롣
			gen_opInteger(op);
			operand_stack_top->type.type = T_INT;
		}
		// ������������Ϊָ�롣˵����������ָ��ĵ�ַ����磺
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
			// ȡ�����������Ĵ�С������char * ptr_one�Ĵ�С����1��
			type_size = pointed_size(&operand_stack_last_top->type);
			// ���ɻ����롣
			gen_opInteger(op);
			// ����ָ��ĵ�ַ��Ϊ�������͡�
			operand_stack_top->type.type = T_INT;
			// �������Ҫ����������С��Ҳ����4��
			operand_push(&int_type, SC_GLOBAL, type_size);
			gen_op(TK_DIVIDE);
		}
		/************************************************************************/
		/* ����������һ����ָ�룬��һ������ָ�룬���ҷǹ�ϵ���㡣Ϊָ���ƶ�     */
		/* �������a[3]��˵��a����ָ�룬3����ָ�롣                           */
		/* ��ʱջ��Ԫ��Ϊ�����±�3������ΪT_INT��                               */
		/* ��ջ��Ԫ��Ϊ�������a������ΪT_PTR��                                 */
		/************************************************************************/
		/* ������һ�����ӡ��뿴arr[i] = i�е���ֵ����arr[i]��Ӧ��ָ��:          */
		/*   11. MOV  EAX, 4                                                    */
		/*   12. MOV  ECX, DWORD PTR SS: [EBP-4]                                */
		/*   13. IMUL ECX, EAX                                                  */
		/*   14. LEA  EAX, DWORD PTR SS: [EBP-2C]                               */
		/*   15. ADD  EAX, ECX                                                  */
		/* ���Կ����߼�Ҳ�ǳ��򵥡�                                             */
		/*   11. ����EAX����Ϊ4��                                               */
		/*   12. ֮��ȡ��i��ֵ����ECX��                                         */
		/*   13. ֮���ECX��EAX��ˣ�����ECX��                                  */
		/*   14. ֮��ȡ�������׵�ַ����EAX��                                    */
		/*   15. ֮�����ECX���õ���r[i]�ĵ�ַ���������EAX��                   */
		/************************************************************************/
		/* ˳��˵һ�䣬������arr[i] = i�еĶ�����Ԫ�ظ�ֵ��Ӧ��ָ��:            */
		/*   16. MOV  ECX, DWORD PTR SS: [EBP-4]                                */
		/*   17. MOV  DWORD PTR DS: [EAX], ECX                                  */
		/* ���Կ����߼�Ҳ�ǳ��򵥡�                                             */
		/*   16. ȡ��i��ֵ����ECX��                                             */
		/*   17. ���arr[i] = i�ĸ�ֵ������                                     */
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
			// �õ���ջ��Ԫ��������������ͣ���������Ϊһ��ȫ�ֱ���ѹջ��
			typeOne = operand_stack_last_top->type;
			operand_push(&int_type, SC_GLOBAL, 
				pointed_size(&operand_stack_last_top->type));
			
			// ���ɳ˷�ָ���������������ͳ���ջ��Ԫ�������±�3��
			// �������Ǿͼ������������ƫ�Ƶ�ַ��
			gen_op(TK_STAR);
			// ���ǵõ�������ƫ�Ƶ�ַ�Ժ󣬾Ϳ��Խ���opָ���Ĳ����ˡ�
			// ����������£�op = TK_PLUS��Ҳ���Ǽӷ���
			gen_opInteger(op);
            // �任����Ϊ��Ա�����������͡���Ϊ������һ��ָ�����͡�
			// �������Ƕ�������ȡ�±��Ժ����ǵ����;ͱ��������Ԫ�ص����͡�
			operand_stack_top->type = typeOne;
		}
	}
	// ���������ָ�룬�Ǿ�����ѧ���㡣
	else
	{
		// ������ѧ�����Ӧ�Ļ�������롣
		gen_opInteger(op);
		// ����ǹ�ϵ����
		if (op >= TK_EQ && op <= TK_GEQ)   // >,<,>=.<=...
		{
			// ��ϵ������ΪT_INT���ͣ���������
			operand_stack_top->type.type = T_INT;
		}

	}
}

