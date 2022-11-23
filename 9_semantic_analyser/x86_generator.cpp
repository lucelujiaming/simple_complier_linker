// x86_generator.cpp : Defines the entry point for the console application.
//

#include "token_code.h"
#include "get_token.h"
#include "symbol_table.h"
#include <windows.h>
#include "x86_generator.h"
#include "reg_manager.h"

#define FUNC_PROLOG_SIZE      9

/* �����õ���ȫ�ֱ��� */
// int return_symbol_pos;			// ��¼returnָ��λ��
int sec_text_opcode_ind = 0;    // ָ���ڴ����λ��
int function_stack_loc = 0;		// �ֲ�������ջ��λ�á�
								// ��Ϊջ�����㣬���ֵ������һֱ��һ��������
								// 
int func_begin_ind;				// ������ʼָ��
int func_ret_sub;				// ���������ͷ�ջ�ռ��С��Ĭ�����㡣
								// �������ָ����__stdcall�ؼ��֣��Ͳ����㡣

extern Type int_type;			// int����

// ����������
/************************************************************************/
/*  ���ܣ���ջ�����������ص�'rc'��Ĵ�����                              */
/*  rc���Ĵ�������                                                      */
/*  opd��������ָ��                                                     */
/************************************************************************/
int load_one(int rc, Operand * opd)
{
	int storage_class ;
	// ��ô洢���͡�
	storage_class = opd->storage_class & SC_VALMASK;
	// ��Ҫ���ص��Ĵ����������
	// 1.ջ��������Ŀǰ��δ����Ĵ���
	// 2.ջ���������ѷ���Ĵ�������Ϊ��ֵ *p
	if (storage_class > SC_GLOBAL ||			// ����ȫ�ֱ�����
		(opd->storage_class & SC_LVAL))			// Ϊ��ֵ��
	{
		storage_class = allocate_reg(rc);		// ����һ�����еļĴ�����
		// �������ǵõ��˼Ĵ���������ֻ��Ҫ��ջ�����������ص�����Ĵ����С�
		load(storage_class, opd);
	}
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
/*     ͨ���Ķ���������ӣ����Կ�����ջ�����������ֵ��                 */
/*     ����ջ�����������ֵ��������ǵ��﷨����ǡ��Ҳ�Ƕ�Ӧ�ġ�         */
/*     �����ڴ���ֵ����ʱ��һ�����Ȼ����ֵ��֮��ѹջ��           */
/*     ֮���ø�ֵ�Ⱥţ����Լ������������ֵ����                                           */
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
	int storage_class = 0,t = 0;
	// ȡ��λ��ջ������ֵ�����ɻ����롣ͬʱ���ر���ļĴ�����
	storage_class = load_one(REG_ANY, operand_stack_top);
	// �����ջ��������Ϊ�Ĵ���������ջ�С�
	if ((operand_stack_last_top->storage_class & SC_VALMASK) == SC_LLOCAL)
	{
		Operand opd;
	//	t = allocate_reg(REG_ANY);
		operand_assign(&opd, T_INT, SC_LOCAL | SC_LVAL, 
			operand_stack_last_top->value);
		load(t, &opd);
		operand_stack_last_top->storage_class = t | SC_LVAL;
	}
	// ���ɽ��Ĵ���'r'�е�ֵ���������'opd'�Ļ����롣
	store(storage_class, operand_stack_last_top);
	// �ͽ���ջ���������ʹ�ջ����������
	operand_swap();
	// �������潻�������Ĵ�ջ����������������Ĳ�����ϵ���ɾ����ջ����������
	operand_pop();
}

// ���ɺ���
/************************************************************************/
/* ���ܣ����ɶ�Ԫ���㣬��ָ�����������һЩ���⴦��                     */
/* op�����������                                                       */
/************************************************************************/
/* �����������ô�����÷�:                                              */
/*   1. �ڱ��ʽ�����Ĺ����У����ɶ�Ӧ��ѧ����Ĵ��롣                  */
/*   2. ����ṹ���Ա��ʱ��ִ��gen_op(TK_PLUS)�����Ա����ƫ�ơ�     */
/*   3. ���������±��ʱ��ִ��gen_op(TK_PLUS)��������ƫ�Ƶ�ַ��       */
/************************************************************************/
void gen_op(int op)
{
	int u, btLastTop, btTop;
	Type typeOne;

	btLastTop = operand_stack_last_top->type.type & T_BTYPE;
	btTop = operand_stack_top->type.type & T_BTYPE;

	// ����Ƚϵ�����Ԫ����һ����ָ�롣
	if (btLastTop == T_PTR || btTop == T_PTR)
	{
		// ����ǱȽϴ�С��
		if (op >= TK_EQ && op <= TK_GEQ)   // >,<,>=.<=...
		{
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
			u = pointed_size(&operand_stack_last_top->type);
			gen_opInteger(op);
			operand_stack_top->type.type = T_INT;
			operand_push(&int_type, SC_GLOBAL, 
				pointed_size(&operand_stack_last_top->type));
			gen_op(TK_DIVIDE);
		}
		/************************************************************************/
		/* ����������һ����ָ�룬��һ������ָ�룬���ҷǹ�ϵ���㡣               */
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
		// ��������������
		if (op >= TK_EQ && op <= TK_GEQ)   // >,<,>=.<=...
		{
			operand_stack_top->type.type = T_INT;
		}

	}
}


/************************************************************************/
/* ���ܣ�����ָ��Ѱַ��ʽ�ֽ�Mod R/M                                    */
/* ��ΪһЩOpcode������������Opcode�룬����ҪModR/M�ֽڽ��и�����       */
/* mod��Mod R/M[7��6]                                                   */
/*       ��R/Mһ�����32�ֿ��ܵ�ֵһ8���Ĵ�����24��Ѱַģʽ             */
/* reg_opcode��ModR/M[5��3]ָ�������3λ������Դ������(�з���׼ȷ)      */
/*       Ҫôָ��һ���Ĵ�����ֵ�� Ҫôָ��Opcode�ж����3�����ص���Ϣ�� */
/*       ��������������������ָ����                                     */
/* r_m��Mod R/M[2��0] Ŀ�������(�з���׼ȷ)                            */
/*       ����ָ��һ���Ĵ�����Ϊ��������                                 */
/*       ���ߺ�mod���ֺ�������ʾһ��Ѱַģʽ                            */
/*       ����Ϊ��e_StorageClass                                         */
/* sym������ָ��                                                        */
/* c�����Ź���ֵ                                                        */
/************************************************************************/
void gen_modrm(int mod, int reg_opcode, int r_m, Symbol * sym, int c)
{
	mod <<= 6;
	reg_opcode <<= 3;
	// mod=11 �Ĵ���Ѱַ 89 E5(11 reg_opcode=100ESP r=101EBP) MOV EBP,ESP
	// 0xC0ת�ɶ����ƾ���1100 0000
	if (mod == 0xC0)
	{
		gen_byte(mod | reg_opcode | (r_m & SC_VALMASK));
	}
	// ������ char a = g_char; ����Ӧ�Ļ�����Ϊ����
	// 	0FBE 0500204000
	// ǰ�����ֽ�0FBE��ǰ���Ѿ����ɺ��ˡ��������ɺ���Ĳ��֡�
	// ����gen_byte����05��gen_addr32���ɺ���Ĳ��֡�
	else if ((r_m & SC_VALMASK) == SC_GLOBAL)
	{
		// mod=00 r=101 disp32  8b 05 50 30 40 00  MOV EAX,DWORD PTR DS:[403050]
		// 8B /r MOV r32,r/m32 Move r/m32 to r32
		// mod=00 r=101 disp32  89 05 14 30 40 00  MOV DWORD PTR DS:[403014],EAX
		// 89 /r MOV r/m32,r32 Move r32 to r/m32
		gen_byte(0x05 | reg_opcode); //ֱ��Ѱַ
		gen_addr32(r_m, sym, c);
	}
	else if ((r_m & SC_VALMASK) == SC_LOCAL)
	{
		// mod=01 r=101 disp8[EBP] 89 45 fc MOV DWORD PTR SS:[EBP-4],EAX
		if (c == c)
		{
			// ������ char a = g_char; ����Ӧ�Ļ�����Ϊ����
			// 	0FBE 0500204000
			//  8845FF
			// ������������ĵڶ��䡣�����45����Դ��
			// 0x45ת�ɶ����ƾ���0100 0101
			gen_byte(0x45 | reg_opcode);
			// ������������ĵڶ��䡣�����FF����Դ��
			gen_byte(c);
		}
		else
		{
			// mod=10 r=101 disp32[EBP]
			// 0x85ת�ɶ����ƾ���1000 0101
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
/*  ���ܣ���������opd���ص��Ĵ���r��                                    */
/*  r�����Ŵ洢����                                                     */
/*  opd��������ָ��                                                     */
/************************************************************************/
/* Ϊ�˷������ۣ����ȼ���ȫ�ֱ����������£�                             */
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
	if (fr & SC_LVAL)    // ��ֵ
	{
		// �����֧�Ļ����������ɷ������£�
		// ���磺һ������ char a = g_char; �����������µ�ָ�
		//    ; MOVSX - ��������չ����ָ��
		//    MOVSX EAX,  BYTE PTR DS: [402000]
		//    MOV BYTE PTR SS: [EBP-1], AL
		// ��������������и����ġ�aΪ��ֵ����Ϊchar��
		// ��Ӧ�Ļ�����Ϊ��
		// 	0FBE 0500204000
		//  8845FF
		// �������������е��������ֵ���Դ��
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
	else    // ��ֵ
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
		else if (v == SC_CMP) // ������c=a>b���
		{
		/*����������ɵ���������
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
/* ���ܣ����Ĵ���'r'�е�ֵ���������'opd'                               */
/* r�����Ŵ洢����                                                      */
/* opd��������ָ��                                                      */
/************************************************************************/
/* һ������ char a = g_char; ������������µ�ָ���еĵڶ��䡣           */
/*    ; MOVSX - ��������չ����ָ��                                      */
/*    MOVSX EAX,  BYTE PTR DS: [402000]                                 */
/*    MOV BYTE PTR SS: [EBP-1], AL                                      */
/* �ڶ����Ӧ�Ļ��������£� 8845 FF                                     */
/* һ������ short b = g_short; ������������µ�ָ���еĵڶ��䡣         */
/*    MOVSX EAX,  WORD PTR DS: [402002]                                 */
/*    MOV WORD PTR SS: [EBP-2], AX                                      */
/* �ڶ����Ӧ�Ļ��������£� 66: 8945FE                                  */
/* һ������ c = g_int; ������������µ�ָ���еĵڶ��䡣                 */
/*    MOV  EAX,  DWORD PTR DS: [402004]                                 */
/*    MOV  DWORD PTR SS: [EBP-4], EAX                                   */
/* �ڶ����Ӧ�Ļ��������£� 8945 FC                                     */
/************************************************************************/
/* ���Կ���������������������ۺ��߼����£�                             */
/*    1. ���short��ֵ����ǰ׺0x66��                                    */
/*    2. ����char��ֵ���ɻ�����0x88��                                   */
/*    3. ����short��int��ֵ���ɻ�����0x89��                             */
/*    4. ���ɺ���Ĳ��֡�                                               */
/************************************************************************/
void store(int r, Operand * opd)
{
	int fr, bt;
	fr = opd->storage_class & SC_VALMASK;
	bt = opd->type.type & T_BTYPE;
	// 1. ���short��ֵ����ǰ׺0x66��
	// �� short b = g_short; ����Ӧ�Ļ�����Ϊ����
	// 	0FBF 0502204000
	//  66: 8945FE
	// ������������ĵڶ����е�ǰ׺�������66����Դ��
	if (bt == T_SHORT)
	{
		gen_prefix(OPCODE_STORE_T_SHORT_PREFIX); //Operand-size override, 66H
	}
	
	// 2. ����char��ֵ���ɻ�����0x88��
	// �� char a = g_char; ����Ӧ�Ļ�����Ϊ����
	// 	0FBE05 00204000
	//  8845 FF
	// ������������ĵڶ��䡣�����88����Դ��
	if (bt == T_CHAR)
	{
		// 88 /r	MOV r/m,r8	Move r8 to r/m8
		gen_opcodeOne(OPCODE_STORE_CHAR_OPCODE);
	}
	// 3. ����short��int��ֵ���ɻ�����0x89�� 
	// �� int c = g_int; ����Ӧ�Ļ�����Ϊ����
	// 	8B05 04204000
	//  8945 FC
	// ������������ĵڶ��䡣�����89����Դ��
	// ����Ҳͬʱ������ b = g_short; ����Ӧ�Ļ��������ɡ�
	else
	{
		// 89 /r	MOV r/m32,r32	Move r32 to r/m32
		gen_opcodeOne(OPCODE_STORE_SHORT_INT_OPCODE);
	}
	// 4. ���ɺ���Ĳ��֡�
	// ���ڳ�����ȫ�ֱ�������������;ֲ�������Ҳ����ջ�б�����������ֵ��
	if (fr == SC_GLOBAL ||				 // ������ȫ�ֱ�������������
		fr == SC_LOCAL  ||				 // �ֲ�������Ҳ����ջ�б���
		(opd->storage_class & SC_LVAL))  // ��ֵ
	{
		gen_modrm(ADDR_OTHER, r, opd->storage_class, opd->sym, opd->value);
	}

}

/************************************************************************/
/* ���ܣ�������������                                                   */
/* OP�����������                                                       */
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
		/* һ������ c = a * b; �����������µ�ָ�                           */
		/*  1. MOV  EAX,  DWORD PTR SS: [EBP-8]                                 */
		/*  2. MOV  ECX,  DWORD PTR SS: [EBP-4]                                 */
		/*  3. IMUL ECX,  EAX                                                   */
		/*  4. MOV  DWORD PTR SS: [EBP-C], ECX                                  */
		// ���Կ����˷�������Ϊ���������ǻ�ó�������ñ�������ִ��IMUL��ɳ˷���
		// ��ó�������ñ�������
		load_two(REG_ANY, REG_ANY);
		storage_class  = operand_stack_last_top->storage_class;
		fr = operand_stack_top->storage_class;
		operand_pop();
		// ִ��IMUL��ɳ˷���
		gen_opcodeTwo(OPCODE_IMUL_HIGH_BYTE, OPCODE_IMUL_LOW_BYTE);
		// ����IMUL��Ҫ��ָ��Ѱַ��ʽ��
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
/* ���ܣ�����������Ԫ����                                               */
/* opc��ModR/M[5��3]                                                    */
/* op�����������                                                       */
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
/* ���ܣ���¼������ת��ַ��ָ����                                       */
/* s��ǰһ��תָ���ַ                                                  */
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
/* ���ܣ����������tΪ���׵ĸ���������ת��ַ������Ե�ַ              */
/* t������                                                              */
/* a��ָ����תλ��                                                      */
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
/* ���ܣ�������ߵ�ַ��תָ���ת��ַ����                             */
/* t��ǰһ��תָ���ַ                                                  */
/* makelist������������ĺ���                                           */
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
/* ���ܣ�������͵�ַ��תָ���ת��ַ��ȷ��                           */
/* a����ת����Ŀ���ַ                                                  */
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
/* ���ܣ�����������תָ��                                               */
/* t��ǰһ��תָ���ַ                                                  */
/* ����ֵ������תָ���ַ                                               */
/************************************************************************/
int gen_jcc(int t)
{
	int v;
	int inv = 1;
	v = operand_stack_top->storage_class & SC_VALMASK;
	// ���ǰһ����CMP������Ҫ����JLE����JGE��
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
 * ����:		���ɺ�����ͷ����
 * func_type:	��������
 * ����������߼��������ġ��������ﲻ������ɺ�����ͷ���롣
 * ֻ�Ǽ�¼�����忪ʼ��λ�ã��ȵ����������ʱ���ں���gen_epilog����䡣
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
	// ��¼�˺����忪ʼ���Ա㺯�������ʱ��亯��ͷ����Ϊ��ʱ����ȷ�����ٵ�ջ��С��
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
		// ����ѹջ��4�ֽڶ���
		size = calc_align(size, 4);
		// �ṹ����Ϊָ�봫��
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
 * ����:	���ɺ�����β����
 * ����������߼��������ġ����Ƕ��ں�����ͷ����ͽ�β���롣
 * �����������ɽ�β���롣֮��ͨ��func_begin_ind��ȡ������ͷ������亯����ͷ���롣
 * ������ͷ��������ָ�����XXXΪջ�ռ��С��һ����������4����������8��
 * 1. PUSH EBP 
 * 2. MOVE BP�� ESP 
 * 3. SUB  ESP�� XXX
 * ������β��������ָ�����XXXXΪ���÷�ʽ��
 * Ĭ�ϲ���Ҫ��д�����ָ����__stdcall�ؼ��֣�����4��
 * 4. MOV  ESP�� EBP 
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
	// ��ind����Ϊ֮ǰ��¼�����忪ʼ��λ�á�
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

	// �ָ�ind��ֵ��
	sec_text_opcode_ind = saved_ind;
}

/***********************************************************
 * ����:	�����꺯����ָ�/�ͷ�ջ,����__cdecl
 * val:		��Ҫ�ͷ�ջ�ռ��С(���ֽڼ�)
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
/* ���ܣ����ɺ������ô��룬 �Ƚ�������ջ�� Ȼ������callָ��             */
/* nb_args����������                                                    */
/************************************************************************/
void gen_invoke(int nb_args)
{
	int size, r, args_size, i, func_call;
	args_size = 0;
	// ����������ջ
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
/* ���ܣ����ɺ�������ָ��                                               */
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



