// x86_generator.cpp : Defines the entry point for the console application.
//

#include "token_code.h"
#include "get_token.h"
#include "symbol_table.h"
#include <windows.h>
#include "x86_generator.h"
#include "reg_manager.h"
#include "instruction_operator.h"
// Define all of opcodes
#include "x86_opcode_define.h"

#define FUNC_PROLOG_SIZE      9

/* �����õ���ȫ�ֱ��� */
// int return_symbol_pos;			// ��¼returnָ��λ��
int sec_text_opcode_offset = 0;    // ָ���ڴ����λ��
int function_stack_loc = 0;		// �ֲ�������ջ��λ�á�
								// ��Ϊջ�����㣬���ֵ������һֱ��һ��������
								// 
int func_begin_ind;				// ������ʼָ��
int func_ret_sub;				// ���������ͷ�ջ�ռ��С��Ĭ�����㡣
								// �������ָ����__stdcall�ؼ��֣��Ͳ����㡣

extern Type int_type;			// int����

// ���ɺ���
/************************************************************************/
/* ���ܣ�����ָ��Ѱַ��ʽ�ֽ�Mod R/M                                    */
/* ��ΪһЩOpcode������������Opcode�룬����ҪModR/M�ֽڽ��и�����       */
/* mod��       Mod R/M[7��6]                                            */
/*             ��R/Mһ�����32�ֿ��ܵ�ֵһ8���Ĵ�����24��Ѱַģʽ       */
/* reg_opcode��ModR/M[5��3] Դ������(�з���׼ȷ)                        */
/*             ��Ϊָ�������3λ�����룬Ҫôָ��һ���Ĵ�����ֵ��        */
/*             Ҫôָ��Opcode�ж����3�����ص���Ϣ��                    */
/*             ��������������������ָ����                               */
/* r_m��       Mod R/M[2��0] Ŀ�������(�з���׼ȷ)                     */
/*             ����ָ��һ���Ĵ�����Ϊ��������                           */
/*             ���ߺ�mod���ֺ�������ʾһ��Ѱַģʽ                      */
/*       ע�⣺���ڶ�����ջ�ϵľֲ����������ֵ�ᱻ���ԡ�               */
/*             �ֲ�����ͳһʹ��disp 8/32[EBP]Ѱַ��                     */
/*             ����Ϊ��e_StorageClass                                   */
/* sym��       ����ָ��                                                 */
/* value��     ���Ź���ֵ                                               */
/************************************************************************/
/* �Ķ����������Ҫ��ϸ�о����ϵı�8.4�����߲鿴Intel��Ƥ��509ҳ��      */
/*     Table 2-1. 16-Bit Addressing Forms with the ModR/M Byte          */
/* ����������Ϊ���������                                               */
/*  1. mod == 0xC0˵���ǼĴ���Ѱַ���μ����ϵı�8.4�ĺ�8�С�            */
/*  2. ȫ�ֱ�����һ�����ڴ�ƫ����ָ�����ڴ��ַ���ǼĴ������Ѱַ��     */
/*     ����disp32��Ҳ����mod = 00��R/M = 101������0x05��                */
/*     �μ����ϵı�8.4��disp32�С�                                      */
/*  3. �ֲ�������ջ�ϣ���ַʹ��EBPѰַ��                                */
/*     �����8λѰַ����disp 8[EBP]��Ҳ����mod = 01��R/M = 101��        */
/*     ����0x45���μ����ϵı�8.4��disp 8[EBP]�С�                       */
/*     �����32λѰַ����disp 32[EBP]��Ҳ����mod = 10��R/M = 101��      */
/*     ����0x85���μ����ϵı�8.4��disp 32[EBP]�С�                      */
/*  4. ����������������ǣ�˵���ǼĴ������Ѱַ��Ҳ���Ǳ����ǰ���С� */
/*     Ҳ����mod = 10��R/Mʹ�ò�����ֵ��                                */
/************************************************************************/
void gen_modrm(int mod, int reg_opcode, int r_m, Symbol * sym, int value)
{
	// ����mod��       Mod R/M[7��6]
	mod <<= MODRM_MOD_OFFSET; // 6;
	// ����reg_opcode��ModR/M[5��3]
	reg_opcode <<= MODRM_REG_OPCODE_OFFSET; // 3;

	// 1.  mod == 0xC0˵���ǼĴ���Ѱַ���μ����ϵı�8.4�ĺ�8�С�
	// mod=11 �Ĵ���Ѱַ 89 E5(11 reg_opcode=100ESP r=101EBP) MOV EBP,ESP
	// 0xC0ת�ɶ����ƾ���1100 0000
	if (mod == MODRM_EFFECTIVE_ADDRESS_REG) // 0xC0
	{
		// ��mod��reg_opcode��r_m���������д�����ڡ�
		gen_byte(mod | reg_opcode | (r_m & SC_VALMASK));
	}
	// 2. ȫ�ֱ�����һ�����ڴ�ƫ����ָ�����ڴ��ַ������disp32��
	else if ((r_m & SC_VALMASK) == SC_GLOBAL)
	{
		// mod=00 r=101 disp32  8b 05 50 30 40 00  MOV EAX,DWORD PTR DS:[403050]
		// 8B /r MOV r32,r/m32 Move r/m32 to r32
		// mod=00 r=101 disp32  89 05 14 30 40 00  MOV DWORD PTR DS:[403014],EAX
		// 89 /r MOV r/m32,r32 Move r32 to r/m32
		gen_byte(MODRM_EFFECTIVE_ADDRESS_DISP32 // 0x05
			| reg_opcode); //ֱ��Ѱַ
		// "disp 32"��ʾSIB�ֽں����һ��32λ��ƫ������
		// ��ƫ������������Ч��ַ��
		gen_addr32(r_m, sym, value);
	}
	// 3. �ֲ�������ջ�ϣ���ַʹ��EBPѰַ�� 
	else if ((r_m & SC_VALMASK) == SC_LOCAL)
	{
		// �����8λѰַ����disp 8[EBP]
		// mod=01 r=101 disp8[EBP] 89 45 fc MOV DWORD PTR SS:[EBP-4],EAX
		if (value == (char)value)
		{
			// ������ char a = g_char; ����Ӧ�Ļ�����Ϊ����
			// 	0FBE 0500204000
			//  8845FF
			// ������������ĵڶ��䡣�����45����Դ��
			// 0x45ת�ɶ����ƾ���0100 0101
			gen_byte(MODRM_EFFECTIVE_ADDRESS_DISP8_EBP  // 0x45 
					| reg_opcode);
			// "disp  8"��ʾSIB�ֽں����һ��8λ��ƫ������
			// ��ƫ��������������չ��Ȼ�󱻼�����Ч��ַ��
			gen_byte(value);
		}
		// �����32λѰַ����disp 32[EBP]
		else
		{
			// mod=10 r=101 disp32[EBP]
			// 0x85ת�ɶ����ƾ���1000 0101
			gen_byte(MODRM_EFFECTIVE_ADDRESS_DISP32_EBP // 0x85
					| reg_opcode);
			// "disp 32"��ʾSIB�ֽں����һ��32λ��ƫ������
			// ��ƫ������������Ч��ַ��
			gen_dword(value);
		}
	}
	// 4. ����������������ǣ�˵���ǼĴ������Ѱַ��
	else 
	{
		// mod=00 89 01(00 reg_opcode=000 EAX r=001ECX) MOV DWORD PTR DS:[ECX],EAX
		gen_byte(MODRM_EFFECTIVE_ADDRESS_REG_ADD_MOD // 0x00
			| reg_opcode | (r_m & SC_VALMASK));
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
	int opd_storage_class_val;
	int opd_type, opd_operand_value, opd_storage_class;
	opd_storage_class = opd->storage_class;
	opd_type = opd->type.type;
	opd_operand_value = opd->operand_value;

	opd_storage_class_val = opd_storage_class & SC_VALMASK;
	if (opd_storage_class & SC_LVAL)    // ��ֵ
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
		if ((opd_type & T_BTYPE) == T_CHAR)
		{
			// �ο�MOVSX�������ʽ��Intel��Ƥ��1250ҳ���Է��֣�
			//     0F BE��ʾ��" Move byte to doubleword with sign-extension."��
			// movsx -- move with sign-extention	
			// 0F BE /r	movsx r32,r/m8	move byte to doubleword,sign-extention
			// 0F BE -> movsx r32, r/m8
			gen_opcodeTwo(OPCODE_MOVSX_BYTE_TO_DOUBLEWORD_HIGH_BYTE, // 0x0f, 
				OPCODE_MOVSX_BYTE_TO_DOUBLEWORD_LOW_BYTE); // 0xbe); 
		}
		else if ((opd_type & T_BTYPE) == T_SHORT)
		{
			// �ο�MOVSX�������ʽ��Intel��Ƥ��1250ҳ���Է��֣�
			//     0F BF��ʾ��"Move word to doubleword, with sign-extension."��
			// movsx -- move with sign-extention	
			// 0F BF /r	movsx r32,r/m16	move word to doubleword,sign-extention
			// 0F BF -> movsx r32, r/m16
			gen_opcodeTwo(OPCODE_MOVSX_WORD_TO_DOUBLEWORD_HIGH_BYTE, // 0x0f, 
				OPCODE_MOVSX_WORD_TO_DOUBLEWORD_LOW_BYTE); // 0xbf); 
		}
		else 
		{
			// �ο�MOV�������ʽ��Intel��Ƥ��1161ҳ���Է��֣�
			//     8B��ʾ��"Move r/m32 to r32."��
			// 8B /r	mov r32,r/m32	mov r/m32 to r32
			gen_opcodeOne(OPCODE_MOV_RM32_TO_R32); // 0x8b); // 8B -> mov r32, r/m32
		}
		gen_modrm(ADDR_OTHER, storage_class, 
			opd_storage_class, opd->sym, opd_operand_value);
	}
	else    // ��ֵ
	{
		// �����ֵ��ȫ�ֱ��������磺��������ע�⣺
		// �ܶ�ֲ�������ʵ�Ƿ��ڼĴ����еġ���ʱ������Ϊ��ֵ����ָ����磺
		//     int  c = g_a ; // g_aΪȫ�ֱ�����
		if (opd_storage_class_val == SC_GLOBAL)
		{
			// B8+ rd	mov r32,imm32		mov imm32 to r32
			gen_opcodeOne(OPCODE_MOVE_IMM32_TO_R32 + storage_class);
			gen_addr32(opd_storage_class, opd->sym, opd_operand_value);
		}
		// �����ֵ�Ǿֲ�������ע�⣺
		// �ܶ�ֲ�������ʵ�Ƿ��ڼĴ����еġ���ʱ������Ϊ��ֵ����ָ�
		// ֻ����Ҫ����ֵѰַ������£��Ż���������֧�����磺
		//    int  * pa = &a;
		//    int x = pt.x;
		else if (opd_storage_class_val == SC_LOCAL)
		{
			// LEA--Load Effective Address
			// 8D /r	LEA r32,m	Store effective address for m in register r32
			gen_opcodeOne(OPCODE_LEA_EFFECTIVE_ADDRESS_IN_R32);
			gen_modrm(ADDR_OTHER, storage_class, SC_LOCAL, opd->sym, opd_operand_value);
		}
		else if (opd_storage_class_val == SC_CMP) // ������c=a>b���
		{
			/************************************************************************/
			/*  ����������ɵ���������                                              */
			/*  00401384   39C8             CMP EAX,ECX                             */
			/*  00401386   B8 00000000      MOV EAX,0                               */
			/*  0040138B   0F9FC0           SETG AL                                 */
			/*  0040138E   8945 FC          MOV DWORD PTR SS:[EBP-4],EAX            */
			/************************************************************************/
			/*B8+ rd	mov r32,imm32		mov imm32 to r32*/
			gen_opcodeOne(OPCODE_MOVE_IMM32_TO_R32 + storage_class); /* mov r, 0*/
			gen_dword(0);
			
			// �ο�SETcc�������ʽ��Intel��Ƥ��1718ҳ���Է��֣�
			//     0F 9F��ʾ��"Set byte if greater (ZF=0 and SF=OF)."��
			// SETcc--Set Byte on Condition
			// OF 9F			SETG r/m8	Set byte if greater(ZF=0 and SF=OF)
			// 0F 8F cw/cd		JG rel16/32	jump near if greater(ZF=0 and SF=OF)
			gen_opcodeTwo(OPCODE_SET_BYTE_IF_GREATER_HIGH_BYTE // 0x0f
						, opd_operand_value + 16);
			gen_modrm(ADDR_REG, 0, storage_class, NULL, 0);
		}
		else if (opd_storage_class_val != storage_class)
		{
			// 89 /r	MOV r/m32,r32	Move r32 to r/m32
			gen_opcodeOne(OPCODE_STORE_SHORT_INT_OPCODE);
			gen_modrm(ADDR_REG, opd_storage_class_val, storage_class, NULL, 0);
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
void store(int reg_index, Operand * opd)
{
	int opd_storage_class, opd_type;
	opd_storage_class = opd->storage_class & SC_VALMASK;
	opd_type          = opd->type.type & T_BTYPE;
	// 1. ���short��ֵ����ǰ׺0x66��
	// �� short b = g_short; ����Ӧ�Ļ�����Ϊ����
	// 	0FBF 0502204000
	//  66: 8945FE
	// ������������ĵڶ����е�ǰ׺�������66����Դ��
	if (opd_type == T_SHORT)
	{
		gen_prefix(OPCODE_STORE_T_SHORT_PREFIX); //Operand-size override, 66H
	}
	
	// 2. ����char��ֵ���ɻ�����0x88��
	// �� char a = g_char; ����Ӧ�Ļ�����Ϊ����
	// 	0FBE05 00204000
	//  8845 FF
	// ������������ĵڶ��䡣�����88����Դ��
	if (opd_type == T_CHAR)
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
	if (opd_storage_class == SC_GLOBAL ||  // ������ȫ�ֱ�������������
		opd_storage_class == SC_LOCAL  ||  // �ֲ�������Ҳ����ջ�б���
		(opd->storage_class & SC_LVAL))    // ��ֵ
	{
		gen_modrm(ADDR_OTHER, reg_index, opd->storage_class, opd->sym, opd->operand_value);
	}

}

/************************************************************************/
/* ���ܣ������������㡣�ú�����gen_op���á�                             */
/* op����������͡���������Ϊe_TokenCode��������ϵ�������ѧ���㡣      */
/************************************************************************/
void gen_opInteger(int op)
{
	int dst_storage_class, src_storage_class;
	int reg_code = REG_ANY;  // Set error code as default value
	switch(op) {
	// ��ѧ���㡣
	case TK_PLUS:
		// �鿴Intel��Ƥ��2517ҳ��One-byte Opcode Map����
		// ���Կ���ADD��ȡֵΪ0x00 ~ 0x05��
		// ����ȡ��5λ��Ҳ���ǳ���8��Ϊ0��
		reg_code = ONE_BYTE_OPCODE_ADD_GIGH_FIVE_BYTES;
		gen_opTwoInteger(reg_code, op);
		break;
	case TK_MINUS:
		// �鿴Intel��Ƥ��2517ҳ��One-byte Opcode Map����
		// ���Կ���ADD��ȡֵΪ0x28 ~ 0x2D��
		// ����ȡ��5λ��Ҳ���ǳ���8��Ϊ5��
		reg_code = ONE_BYTE_OPCODE_SUB_GIGH_FIVE_BYTES;
		gen_opTwoInteger(reg_code, op);
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
		dst_storage_class  = operand_stack_last_top->storage_class;
		src_storage_class = operand_stack_top->storage_class;
		operand_pop();
		// ִ��IMUL��ɳ˷���
		gen_opcodeTwo(OPCODE_IMUL_HIGH_BYTE, OPCODE_IMUL_LOW_BYTE);
		// ����IMUL��Ҫ��ָ��Ѱַ��ʽ��
		gen_modrm(ADDR_REG, dst_storage_class, src_storage_class, NULL, 0);
		break;
	case TK_DIVIDE:
	case TK_MOD:
		/* ������Intel��Ƥ��509ҳ��Table 2-1.�в���F9�����Է��֣�               */
		/*     ��Ϊ������ֱ��ʹ��ECX�������Mod=0b11Ҳ����3��R/MΪ001           */
		/*     ��reg_code = 0b111��Ҳ����REG_EDI��                              */
		reg_code = REG_EDI; // 7;
		// ��ΪIDIV ECX�ᵼ��EAX����ECX��
		// �ѳ����ͱ��������ص�EAX��ECX�С�
		load_two(REG_EAX, REG_ECX);
		dst_storage_class  = operand_stack_last_top->storage_class;
		src_storage_class = operand_stack_top->storage_class;
		operand_pop();
		// ��ΪIDIV ECX�Ľ���У��̷���EAX��������EDX��
		// ����ϣ�������ҪԤ��REG_EDX��
		spill_reg(REG_EDX);

		// CWD/CDQ--Convert Word to Doubleword/Convert Doubleword to Qradword
		// 99	CWQ	EDX:EAX<--sign_entended EAX
		gen_opcodeOne(OPCODE_CDQ);

		/* һ������ IDIV ECX  ��Ӧ�Ļ�����Ϊ��F7F9��������д��һ���ַ�F7        */
		/************************************************************************/
		/* ָ��μ�Intel��Ƥ��1015ҳ��ʹ��IDIV r/m32��Ҳ����F7���������£�      */
		/*  Signed divide EDX:EAX by r/m32, with result                         */
		/*  stored in EAX <- Quotient, EDX <- Remainder.                        */
		/************************************************************************/
		/* IDIV--Signed Divide                                                  */
		/* F7 /7	IDIV r/m32	Signed divide EDX:EAX                           */
		/* (where EDX must contain signed extension of EAX)                     */
		/* by r/m doubleword.(Result:EAX=Quotient, EDX=Remainder)               */
		/* EDX:EAX������ r/m32����                                              */
		/************************************************************************/
		gen_opcodeOne(OPCODE_IDIV);
		/* д�ڶ����ַ�F9                                                       */
		/* ������Intel��Ƥ��509ҳ��Table 2-1.�в���F9�����Է��֣�               */
		/*     ��Ϊ������ֱ��ʹ��ECX�������Mod=0b11Ҳ����3��R/MΪ001           */
		/*     ��reg_code = 0b111��Ҳ����REG_EDI��                              */
		gen_modrm(ADDR_REG, reg_code, src_storage_class, NULL, 0);
		// ��ΪIDIV ECX�Ľ���У��̷���EAX��������EDX��
		if (op == TK_MOD)
		{
			dst_storage_class = REG_EDX;
		}
		else
		{
			dst_storage_class = REG_EAX;
		}
		operand_stack_top->storage_class = dst_storage_class;
		break;
	// ��ϵ���㡣
	default:
		// �鿴Intel��Ƥ��2517ҳ��One-byte Opcode Map����
		// ���Կ���ADD��ȡֵΪ0x38 ~ 0x3D��
		// ����ȡ��5λ��Ҳ���ǳ���8��Ϊ7��
		reg_code = ONE_BYTE_OPCODE_CMP_GIGH_FIVE_BYTES;
		gen_opTwoInteger(reg_code, op);
		break;
	}
}

/************************************************************************/
/* ���ܣ�     ����������Ԫ���㡣�ú�����gen_opInteger���á�             */
/*            ֻ�����ӷ��������͹�ϵ������                              */
/* reg_code�� ModR/M[5��3]                                              */
/* op��       ��������͡���������Ϊe_TokenCode��                       */
/*            ֻ�����ӷ��������͹�ϵ���㡣                              */
/************************************************************************/
/* ��������߼����£�                                                   */
/* 1. ���ȼ���������������                                            */
/*   1.1. ���Ŀ���������8λ������������a + 8; 6 + 8; a - 8;           */
/*        �ǵ�һ���ֽھ���0x83��ʹ�ô����reg_code����ָ��Ѱַ��ʽ�ֽڡ�*/
/*   1.2. ���Ŀ���������32λ���������ǵ�һ���ֽھ���0x81��            */
/*        ʹ�ô����reg_code����ָ��Ѱַ��ʽ�ֽڡ�                      */
/*   1.3. ���Ŀ�������������������                                    */
/*        ���ݲ������洢���ͺ�Ŀ��������洢��������ָ��Ѱַ��ʽ�ֽڡ�  */
/* 2. ���ڹ�ϵ����������CMPӰ��λ��                                    */
/************************************************************************/
/* ���0x83�Ľ������£�                                                 */
/* �ο�Add/Cmp/Sub�������ʽ��Intel��Ƥ��604/726/1776ҳ���Է��֣�       */
/*     0x83��ʾ��"Add sign-extended imm8 to r/m32."��                   */
/*     0x83��ʾ��"Compare imm8 with r/m32."��                           */
/*     0x83��ʾ��"Subtract sign-extended imm8 from r/m32."��            */
/* ���0x83�Ľ��͹������£�                                             */
/*     ���Ȳ鿴Intel��Ƥ��2517ҳ��One-byte Opcode Map����               */
/*     �������Immediate Grp 1����ʽ��Ev,lb��˵����������˶���ָ�   */
/*     Ҫ���ݺ����һ���ֽڵ�ModR/Mȷ����Ӧ��ָ��                       */
/*     �������к���Ev���ţ���ô���������һ���ֽھ���MODR/M��           */
/*     ͨ�����ModR/M�õ�ModR/M[5��3]�Ϳ����ƶϳ�ָ�                 */
/************************************************************************/
/* ���0x81�Ľ������£�                                                 */
/* �ο�Add/Cmp/Sub�������ʽ��Intel��Ƥ��604/726/1776ҳ���Է��֣�       */
/*     0x81��ʾ��"Add imm32 to r/m32."��                                */
/*     0x81��ʾ��"Compare imm32 with r/m32."��                          */
/*     0x81��ʾ��"Subtract imm32 from r/m32."��                         */
/* ���0x81�Ľ��͹������£�                                             */
/*     ���Ȳ鿴Intel��Ƥ��2517ҳ��One-byte Opcode Map����               */
/*     �������Immediate Grp 1����ʽ��Ev,lz��˵����������˶���ָ�   */
/*     Ҫ���ݺ����һ���ֽڵ�ModR/Mȷ����Ӧ��ָ��                       */
/*     �������к���Ev���ţ���ô���������һ���ֽھ���MODR/M��           */
/*     ͨ�����ModR/M�õ�ModR/M[5��3]�Ϳ����ƶϳ�ָ�                 */
/************************************************************************/
void gen_opTwoInteger(int reg_code, int op)
{
	int dst_storage_class, src_storage_class, value;
	// ���ջ��Ԫ�ز��Ƿ��ţ�Ҳ�Ͳ���ȫ�ֱ����ͺ������壬
	// �������Ҳ������ֵ���Ǿ�ֻ���ǳ�����
	if ((operand_stack_top->storage_class & 
		(SC_VALMASK | SC_LVAL | SC_SYM)) == SC_GLOBAL)
	{
		dst_storage_class = load_one(REG_ANY, operand_stack_last_top);
		// ���c��һ��8λ��������
		value = operand_stack_top->operand_value;
		if (value == (char)value)
		{
			// ADC--Add with Carry			83 /2 ib	ADC r/m32,imm8	Add with CF sign-extended imm8 to r/m32
			// ADD--Add						83 /0 ib	ADD r/m32,imm8	Add sign-extended imm8 from r/m32
			// SUB--Subtract				83 /5 ib	SUB r/m32,imm8	Subtract sign-extended imm8 to r/m32
			// CMP--Compare Two Operands	83 /7 ib	CMP r/m32,imm8	Compare imm8 with r/m32
			// �����ǼӼ������ǱȽϲ�����ֻҪ�ǵ�һ���ֽڶ���0x83��
			gen_opcodeOne(ONE_BYTE_OPCODE_IMME_GRP_TO_IMM8); // (0x83);
			// ֻ�еڶ����ֽڲ�ͬ��
			gen_modrm(ADDR_REG, reg_code, dst_storage_class, NULL, 0);
			// �������������ͺ��ˡ�
			gen_byte(value);
		}
		// �������32λ����������
		else
		{
			// ADD--Add					    81 /0 id	ADD r/m32,imm32	Add sign-extended imm32 to r/m32
			// SUB--Subtract				81 /5 id	SUB r/m32,imm32	Subtract sign-extended imm32 from r/m32
			// CMP--Compare Two Operands	81 /7 id	CMP r/m32,imm32	Compare imm32 with r/m32
			gen_opcodeOne(ONE_BYTE_OPCODE_IMME_GRP_TO_IMM32); //(0x81);
			gen_modrm(ADDR_REG, reg_code, dst_storage_class, NULL, 0);
			gen_byte(value);
		}
	}
	// ���ջ��Ԫ�ز��ǳ�������һ���ֽھͺ�opc�й�ϵ�ˡ�
	else
	{
		// More clear
		int one_byte_opcode = reg_code;
		load_two(REG_ANY, REG_ANY);
		dst_storage_class  = operand_stack_last_top->storage_class;
		src_storage_class  = operand_stack_top->storage_class;
		// ������ӷ�ʱ��opc��0������ʱ��opc��5���Ƚ�ʱ��opc��7��
		// ����ϣ����������ǣ��ӷ�ʱΪ0x01������ʱΪ0x29���Ƚ�ʱΪ0x39��
		// �鿴Intel��Ƥ��2517ҳ��One-byte Opcode Map����
		// �����ҵ���Ӧ��ָ��ֱ�ΪADD��SUB��CMP
		gen_opcodeOne((one_byte_opcode << 3)
				| ONE_BYTE_OPCODE_LOW_THREE_BYTE_EV_GV); // 0x01);
		// ���ݲ������洢���ͺ�Ŀ��������洢��������ָ��Ѱַ��ʽ�ֽڡ�
		gen_modrm(ADDR_REG, src_storage_class, dst_storage_class, NULL, 0);
	}
	// ������ɡ�����ջ��Ԫ�ء�
	operand_pop();
	// CMPӰ��λ��Intel��Ƥ��417ҳ��
	// Ҳ����Table B-1. EFLAGS Condition Codes
	if (op >= TK_EQ && op <= TK_GEQ)   // >,<,>=.<=...
	{
		operand_stack_top->storage_class = SC_CMP;
		switch(op) {
		case TK_EQ:
			operand_stack_top->operand_value
				= OPCODE_CONDITION_CODES_EQUAL;           // 0x84;
			break;
		case TK_NEQ:
			operand_stack_top->operand_value
				= OPCODE_CONDITION_CODES_NOT_EQUAL;       // 0x85;
			break;
		case TK_LT:
			operand_stack_top->operand_value
				= OPCODE_CONDITION_CODES_LESS;            // 0x8c;
			break;
		case TK_LEQ:
			operand_stack_top->operand_value
				= OPCODE_CONDITION_CODES_LESS_OR_EQUAL;   // 0x8e;
			break;
		case TK_GT:
			operand_stack_top->operand_value
				= OPCODE_CONDITION_CODES_GREATER;          // 0x8f;
			break;
		case TK_GEQ:
			operand_stack_top->operand_value
				= OPCODE_CONDITION_CODES_GREATER_OR_EQUAL; // 0x8d;
			break;
		}
	}
}

/************************************************************************/
/* ���ܣ�          ������ߵ�ַ��תָ���ת��ַ����                   */
/* target_address��ǰһ��תָ���ַ                                     */
/* makelist������������ĺ���                                           */
/************************************************************************/
int gen_jmpforward(int target_address)
{
	// �ο�JMP�������ʽ��Intel��Ƥ��1064ҳ���Է��֣�
	//     E9 cd��ʾ��"Jump near, relative, RIP = RIP + 32-bit 
    //                 displacement sign extended to 64-bits."��
	// JMP--Jump		
	// E9 cd	JMP rel32	
	// Jump near,relative,displacement relative to next instruction
	gen_opcodeOne(OPCODE_JUMP_NEAR); // (0xe9);
	return make_jmpaddr_list(target_address);
}

/************************************************************************/
/* ���ܣ�������͵�ַ��תָ���ת��ַ��ȷ��                           */
/* target_address����ת����Ŀ���ַ                                     */
/************************************************************************/
#define  OPCODE_SIZE_JMP         2
void gen_jmpbackward(int target_address)
{
	int displacement;
	// displacement - ƫ�����������������Ϊ��Ե�ַ��
	// ���ƫ�����ļ��㷽�����£�
	// ���ȼ���ָ���ڴ���ڵ�ƫ����Ϊ51��Ҳ����sec_text_opcode_ind = 51��
	// ������ת��ƫ����28���棬Ҳ����target_address = 28��
	// ������Ҫ������ֵ�����ͬʱ���ǻ���Ҫ��ȥJMPָ��Ļ����뱾���Ĵ�С��
	displacement = target_address - sec_text_opcode_offset - OPCODE_SIZE_JMP;
	// 8λת���Ƕ�ת�ơ�����ת��Χ��-128-127��
	if (displacement = (char)displacement)
	{
		// �ο�JMP�������ʽ��Intel��Ƥ��1064ҳ���Է��֣�
		//     EB cb��ʾ��"Jump short, RIP = RIP + 8-bit displacement sign 
	    //                 extended to 64-bits."��
		// EB cb	JMP rel8	
		// Jump short,relative,displacement relative to next instruction
		gen_opcodeOne(OPCODE_JUMP_SHORT); //(0xeb);
		// 8-bit displacement
		gen_byte(displacement);
	}
	// ������32λת�� - Զת��
	else
	{
		// �ο�JMP�������ʽ��Intel��Ƥ��1064ҳ���Է��֣�
		//     E9 cd��ʾ��"Jump near, relative, RIP = RIP + 32-bit 
	    //                 displacement sign extended to 64-bits."��
		// E9 cd	JMP rel32	
		// Jump short,relative,displacement relative to next instruction
		gen_opcodeOne(OPCODE_JUMP_NEAR); //(0xe9);
		// ƫ���� - 32-bit displacement
		gen_dword(target_address - sec_text_opcode_offset - 4);
	}
}

/************************************************************************/
/* ���ܣ�    ����������תָ��                                           */
/* jmp_addr��ǰһ��תָ���ַ                                           */
/* ����ֵ��  ��make_jmpaddr_list���ɵ�����תָ���ַ��                  */
/*           gen_jmpforwardҲ������make_jmpaddr_list�����ء�            */
/************************************************************************/
int gen_jcc(int jmp_addr)
{
	int storage_class;
	// �ο�Jcc�������ʽ��Intel��Ƥ��1060ҳ���Է��֣�
	// ��ZF=1ʱ�����λ��ı�
	int inv = 1;  // if 0 (ZF=1)
	storage_class = operand_stack_top->storage_class & SC_VALMASK;
	// ���ǰһ����CMP������Ҫ����JLE����JGE��
	// ���ﴦ��������������ʽ������д����
	//     if (a > b)
	if (storage_class == SC_CMP)
	{
		// ��storage_classΪSC_CMP��ʱ��operand_valueΪ��Ӧ��CMPӰ��λ��
		// �μ���Intel��Ƥ��417ҳ��Ҳ����Table B-1. EFLAGS Condition Codes
		// �ο�Jcc�������ʽ��Intel��Ƥ��1060ҳ���Է��֣�
		//     0F 8F cw/cd ��ʾ��"Jump near if greater (ZF=0 and SF=OF)."��
		// Jcc--Jump if Condition Is Met
		// .....
		// 0F 8F cw/cd		JG rel16/32	jump near if greater(ZF=0 and SF=OF)
		// .....
		gen_opcodeTwo(OPCODE_JCC_JUMP_NEAR_IF_GREATER // 0x0f
				, operand_stack_top->operand_value ^ inv);
		jmp_addr = make_jmpaddr_list(jmp_addr);
	}
	else
	{
		// ���ջ��Ԫ�ز��Ƿ��ţ�Ҳ�Ͳ���ȫ�ֱ����ͺ������壬
		// �������Ҳ������ֵ���Ǿ�ֻ���ǳ�����
		// Ҳ���������������������ǻ�������ôд���룺
		//     if (1)
		// ���ֻ��Ҫֱ��JMP���ɡ�
		if (operand_stack_top->storage_class & 
			(SC_VALMASK | SC_LVAL | SC_SYM) == SC_GLOBAL)
		{
			jmp_addr = gen_jmpforward(jmp_addr);
		}
		// ���ﴦ������һ���������������ǻ�������ôд���룺
		//     if (a = 7)
		// Ҳ������������£�if����ı���ʽ����һ���Ƚϲ���������һ�����������
		else
		{
			storage_class = load_one(REG_ANY, operand_stack_top);
			// �ο�TEST�������ʽ��Intel��Ƥ��1801ҳ���Է��֣�
			//     85 /r��ʾ��"AND r32 with r/m32; set SF, ZF, PF according to result."��
			// TESTָ���CMPָ�����ƣ�������ֻ�ı���Ӧ�ı�־�Ĵ�����λ������ֵ���浽��һ���������С�
			// ���磺test ecx��ecx��ָ������ָ��ֻ��ecx�е�ֵΪ0ʱ����־�Ĵ���ZFλ����Ϊ0��
			// TEST--Logical Compare
			// 85 /r	TEST r/m32,r32	AND r32 with r/m32,set SF,ZF,PF according to result		
			gen_opcodeOne(OPCODE_TEST_AND_R32_WITH_RM32); // 0x85);
			gen_modrm(ADDR_REG, storage_class, storage_class, NULL, 0);
			
			// �ο�Jcc�������ʽ��Intel��Ƥ��1060ҳ���Է��֣�
			//	   0F 8F cw/cd ��ʾ��"Jump near if greater (ZF=0 and SF=OF)."��
			// Ҳ���Ǹ��ݱ�־�Ĵ���ZFλ������ת��
			//  Jcc--Jump if Condition Is Met
			// .....
			// 0F 8F cw/cd		JG rel16/32	jump near if greater(ZF=0 and SF=OF)
			// .....
			gen_opcodeTwo(OPCODE_JCC_JUMP_NEAR_IF_GREATER // 0x0f
							, 0x85 ^ inv);
			jmp_addr = make_jmpaddr_list(jmp_addr);
		}
	}
	operand_pop();
	return jmp_addr;
}

/***********************************************************
 * ����:		���ɺ�����ͷ����
 * func_type:	��������
 * ����������߼��������ġ��������ﲻ������ɺ�����ͷ���롣
 * ֻ�Ǽ�¼�����忪ʼ��λ�ã��ȵ����������ʱ���ں���gen_epilog����䡣
 **********************************************************/
void gen_prologue(Type *func_type)
{
	int addr, align, size, func_call;
	int param_addr;
	Symbol * sym;
	Type * type;
	// �������Ͷ����Ӧ�ķ��ű���
	sym = func_type->ref;
	// ��ú����ĵ��÷�ʽ��Ĭ��ΪCDECL
	func_call = sym->storage_class;
	addr = 8;
	function_stack_loc  = 0;
	// ��¼�˺����忪ʼ���Ա㺯�������ʱ��亯��ͷ����Ϊ��ʱ����ȷ�����ٵ�ջ��С��
	func_begin_ind = sec_text_opcode_offset;
	sec_text_opcode_offset += FUNC_PROLOG_SIZE;
	// ��֧�ַ��ؽṹ�壬���Է��ؽṹ��ָ��
	if (sym->typeSymbol.type == T_STRUCT)
	{
		print_error("Can not return T_STRUCT");
	}
	//  ��������в�����������������
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
		// ����ջ��ƫ�ơ���Ϊ���ŵĹ���ֵ��
		param_addr = addr;
		addr += size;
		// �Ѳ������ŷ��ڷ���ջ
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
void gen_epilogue()
{
	int reserved_stack_size, saved_ind, opc;
	// ���ɺ�����βָ�����������
	//    �ο�MOV�������ʽ��Intel��Ƥ��1161ҳ���Է��֣�
	//        8B��ʾ��"Move r/m32 to r32."��
	//    8B /r	mov r32,r/m32	mov r/m32 to r32
	gen_opcodeOne((char)OPCODE_MOV_RM32_TO_R32); // 0x8B);
	/* 4. MOV ESP, EBP*/
	gen_modrm(ADDR_REG, REG_ESP, REG_EBP, NULL, 0);

	// �ο�POP�������ʽ��Intel��Ƥ��1510ҳ���Է��֣�
	//     58 + rd��ʾ��"Pop top of stack into r32; increment stack pointer."��
	// POP--Pop a Value from the Stack
	// 58+	rd		POP r32		
	//    POP top of stack into r32; increment stack pointer
	/* 5. POP EBP */
	gen_opcodeOne(OPCODE_POP_STACK_TOP_INTO_R32 // 0x58
					+ REG_EBP);
	// KW_CDECL
	if (func_ret_sub == 0)
	{
		// �ο�RET�������ʽ��Intel��Ƥ��1675ҳ���Է��֣�
		//     C3��ʾ��"Near return to calling procedure."��
		// 6. RET--Return from Procedure
		// C3	RET	near return to calling procedure
		gen_opcodeOne(OPCODE_NEAR_RETURN); // 0xC3);
	}
	// KW_STDCALL
	else
	{
		// �ο�RET�������ʽ��Intel��Ƥ��1675ҳ���Է��֣�
		//     C2 iw��ʾ��"Near return to calling procedure and pop imm16 bytes from stack."��
		// 6. RET--Return from Procedure
		//    C2 iw	RET imm16	near return to calling procedure 
		//                      and pop imm16 bytes form stack
		gen_opcodeOne(OPCODE_NEAR_RETURN_AND_POP_IMM16_BYTES); // 0xC2);
		gen_byte(func_ret_sub);
		gen_byte(func_ret_sub >> 8);
	}
	reserved_stack_size = calc_align(-function_stack_loc, 4);
	// ��ind����Ϊ֮ǰ��¼�����忪ʼ��λ�á�
	saved_ind = sec_text_opcode_offset;
	sec_text_opcode_offset = func_begin_ind;

	// �ο�PUSH�������ʽ��Intel��Ƥ��1633ҳ���Է��֣�
	//     50+rd��ʾ��"Push r32."��
	// 1. PUSH--Push Word or Doubleword Onto the Stack
	//    50+rd	PUSH r32	Push r32
	gen_opcodeOne(OPCODE_PUSH_R32 // 0x50
						+ REG_EBP);

	// �ο�MOV�������ʽ��Intel��Ƥ��1161ҳ���Է��֣�
	//     89 /r��ʾ��"Move r32 to r/m32."��
	// 89 /r	MOV r/m32,r32	Move r32 to r/m32
	// 2. MOV EBP, ESP
	gen_opcodeOne(OPCODE_MOVE_R32_TO_RM32); // 0x89);
	gen_modrm(ADDR_REG, REG_ESP, REG_EBP, NULL, 0);

	// �ο�SUB�������ʽ��Intel��Ƥ��1776ҳ���Է��֣�
	//     81 /5 id��ʾ��"Subtract imm32 from r/m32."��
	//SUB--Subtract		81 /5 id	SUB r/m32,imm32	
	//         Subtract sign-extended imm32 from r/m32
	// 3. SUB ESP, stacksize
	gen_opcodeOne(OPCODE_SUBTRACT_IMM32_FROM_RM32); // 0x81);
	opc = 5;
	gen_modrm(ADDR_REG, opc, REG_ESP, NULL, 0);
	gen_dword(reserved_stack_size);

	// �ָ�ind��ֵ��
	sec_text_opcode_offset = saved_ind;
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
		gen_opcodeOne(ONE_BYTE_OPCODE_IMME_GRP_TO_IMM8);	// ADD esp,val
		gen_modrm(ADDR_REG,opc,REG_ESP,NULL,0);
        gen_byte(val);
    } 
	else 
	{
		// ADD--Add	81 /0 id	ADD r/m32,imm32	Add sign-extended imm32 to r/m32
		gen_opcodeOne(ONE_BYTE_OPCODE_IMME_GRP_TO_IMM32);	// add esp, val
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
	int size, reg_idx, args_size, idx, func_call;
	args_size = 0;
	// ����������ջ��
	// ������һ�����⡣����������ʵֻ���ĸ��Ĵ�����
	// ����������������ĸ���ô�죿��
	for (idx = 0; idx < nb_args; idx++)
	{
		reg_idx = load_one(REG_ANY, operand_stack_top);
		size =4;
		// �ο�PUSH�������ʽ��Intel��Ƥ��1633ҳ���Է��֣�
		//     50+rd��ʾ��"Push r32."��
		// PUSH--Push Word or Doubleword Onto the Stack
		// 50+rd	PUSH r32	Push r32
		gen_opcodeOne(OPCODE_PUSH_R32 // 0x50
						+ reg_idx);
		args_size += size;
		operand_pop();
	}
	// ��ռ�õļĴ���ȫ�������ջ�С�
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
	int reg_idx;
	if (operand_stack_top->storage_class & (SC_VALMASK | SC_LVAL) == SC_GLOBAL)
	{
		// ��¼�ض�λ��Ϣ
		coffreloc_add(sec_text, operand_stack_top->sym, 
			sec_text_opcode_offset + 1, IMAGE_REL_I386_REL32);
			
		// �ο�CALL�������ʽ��Intel��Ƥ��695ҳ���Է��֣�
		//     E8 cd��ʾ��"Call near, relative, displacement relative to next instruction. "��
		//	CALL--Call Procedure E8 cd   
		//	CALL rel32    call near,relative,displacement relative to next instrution
		gen_opcodeOne(OPCODE_CALL_NEAR_RELATIVE_32_BIT_DISPLACE); // 0xe8);
		gen_dword(operand_stack_top->operand_value - 4);
	}
	else
	{
		reg_idx = load_one(REG_ANY, operand_stack_top);
		// �ο�CALL�������ʽ��Intel��Ƥ��695ҳ���Է��֣�
		//     FF /2��ʾ��" Call near, absolute indirect, address given in r/m32."��
        // FF /2 CALL r/m32 Call near, absolute indirect, address given in r/m32
		gen_opcodeOne(OPCODE_CALL_NEAR_ABSOLUTE); // 0xff);
		gen_opcodeOne(OPCODE_CALL_NEAR_ABSOLUTE_32_RM32_ADDRESS // 0xd0
						+ reg_idx);
	}
}

/************************************************************************/
/* һ������ char str1[]  = "str1"; �����������µ�ָ�               */
/*    MOV ECX,  5                                                       */
/*    MOV ESI,  scc_anal.00403015; ASCII"strl"                          */
/*    LEA EDI,  DWORD PTR SS: [EBP-9]                                   */
/*    REP MOVS  BYTE PTR ES: [EDI], BYTE PTR DS: [ESI]                  */
/* ��Ӧ�Ļ��������£�                                                   */
/*    B9 05000000                                                       */
/*    BE 15304000                                                       */
/*    8D7D F7                                                           */
/*    F3: A4                                                            */
/************************************************************************/
void array_initialize()
{
	// �ճ�REG_ECX��Ϊ��ֵ������
	spill_reg(REG_ECX);

	/*    MOV ECX,  5 ��Ӧ�Ļ��������£�        */
	/*    B9 05000000                           */
	// �ο�MOV�������ʽ��Intel��Ƥ��1161ҳ���Է��֣�
	//     0x0xB8��ʾ��"Move imm32 to r32."�� 
	gen_opcodeOne(OPCODE_MOVE_IMM32_TO_R32 + REG_ECX);
	gen_dword(operand_stack_top->type.ref->related_value);
	
	/*    MOV ESI,  scc_anal.00403015; ASCII"strl"                          */
	/*    BE 15304000                                                       */
	gen_opcodeOne(OPCODE_MOVE_IMM32_TO_R32 + REG_ESI);
	gen_addr32(operand_stack_top->storage_class, operand_stack_top->sym, operand_stack_top->operand_value);
	operand_swap();
	
	/*    LEA EDI,  DWORD PTR SS: [EBP-9] ��Ӧ�Ļ��������£�   */
	/*    8D7D F7                                              */
	// �ο�LEA�������ʽ��Intel��Ƥ��1101ҳ���Է��֣�
	//     0xB8��ʾ��"Store effective address for m in register r32."��
	gen_opcodeOne(OPCODE_LEA_EFFECTIVE_ADDRESS_IN_R32);
	gen_modrm(ADDR_OTHER, REG_EDI, SC_LOCAL, operand_stack_top->sym, operand_stack_top->operand_value);
	/* REP MOVS  BYTE PTR ES: [EDI], BYTE PTR DS: [ESI]��Ӧ�Ļ��������£�   */
	/*    F3: A4                                                            */
	// �ο�REP MOVS�������ʽ��Intel��Ƥ��1671ҳ���Է��֣�
	//     F3 A4��ʾ��"Move (E)CX bytes from DS:[(E)SI] to ES:[(E)DI]."��
	gen_prefix((char)OPCODE_REP_PREFIX);
	gen_opcodeOne((char)OPCODE_MOVE_ECX_BYTES_DS_ESI_TO_ES_EDI);
	// ����ECX��ESI
	operand_stack_top -= 2;
}

