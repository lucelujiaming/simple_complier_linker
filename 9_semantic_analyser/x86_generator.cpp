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
int sec_text_opcode_ind = 0;    // ָ���ڴ����λ��
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
/* c��         ���Ź���ֵ                                               */
/************************************************************************/
/* �Ķ����������Ҫ��ϸ�о���8.4������������Ϊ���������                */
/*  1. mod == 0xC0˵���ǼĴ���Ѱַ���μ���8.4�ĺ�8�С�                  */
/*  2. ȫ�ֱ�����һ�����ڴ�ƫ����ָ�����ڴ��ַ���ǼĴ������Ѱַ��     */
/*     ����disp32��Ҳ����mod = 00��R/M = 101������0x05��                */
/*     �μ���8.4��disp32�С�                                            */
/*  3. �ֲ�������ջ�ϣ���ַʹ��EBPѰַ��                                */
/*     �����8λѰַ����disp 8[EBP]��Ҳ����mod = 01��R/M = 101��        */
/*     ����0x45���μ���8.4��disp 8[EBP]�С�                             */
/*     �����32λѰַ����disp 32[EBP]��Ҳ����mod = 10��R/M = 101��      */
/*     ����0x85���μ���8.4��disp 32[EBP]�С�                            */
/*  4. ����������������ǣ�˵���ǼĴ������Ѱַ��Ҳ���Ǳ���ǰ���С� */
/*     Ҳ����mod = 10��R/Mʹ�ò�����ֵ��                                */
/************************************************************************/
void gen_modrm(int mod, int reg_opcode, int r_m, Symbol * sym, int c)
{
	// ����mod��       Mod R/M[7��6]
	mod <<= MODRM_MOD_OFFSET; // 6;
	// ����reg_opcode��ModR/M[5��3]
	reg_opcode <<= MODRM_REG_OPCODE_OFFSET; // 3;

	// 1.  mod == 0xC0˵���ǼĴ���Ѱַ���μ���8.4�ĺ�8�С�
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
		gen_addr32(r_m, sym, c);
	}
	// 3. �ֲ�������ջ�ϣ���ַʹ��EBPѰַ�� 
	else if ((r_m & SC_VALMASK) == SC_LOCAL)
	{
		// �����8λѰַ����disp 8[EBP]
		// mod=01 r=101 disp8[EBP] 89 45 fc MOV DWORD PTR SS:[EBP-4],EAX
		if (c == c)
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
			gen_byte(c);
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
			gen_dword(c);
		}
	}
	// 4. ����������������ǣ�˵���ǼĴ������Ѱַ��
	else 
	{
		// mod=00 89 01(00 reg_opcode=000 EAX r=001ECX) MOV DWORD PTR DS:[ECX],EAX
		gen_byte(MODRM_EFFECTIVE_ADDRESS_REG_ADDRESS_MOD // 0x00
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
/* ���ܣ������������㡣�ú�����gen_op���á�                             */
/* op����������͡���������Ϊe_TokenCode��������ϵ�������ѧ���㡣      */
/************************************************************************/
void gen_opInteger(int op)
{
	int dst_storage_class, src_storage_class, opc;
	switch(op) {
	// ��ѧ���㡣
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
		opc = 7;
		load_two(REG_EAX, REG_ECX);
		dst_storage_class  = operand_stack_last_top->storage_class;
		src_storage_class = operand_stack_top->storage_class;
		operand_pop();
		spill_reg(REG_EDX);
		gen_opcodeOne(OPCODE_CDQ);
		gen_opcodeOne(OPCODE_IDIV);
		gen_modrm(ADDR_REG, opc, src_storage_class, NULL, 0);
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
		opc = 7;
		gen_opTwoInteger(opc, op);
		break;
	}
}

/************************************************************************/
/* ���ܣ�����������Ԫ���㡣�ú�����gen_opInteger���á�                  */
/*       ֻ����ӷ��������͹�ϵ������                                   */
/* opc�� ModR/M[5��3]                                                   */
/* op��  ��������͡���������Ϊe_TokenCode��ֻ�����ӷ��������͹�ϵ���㡣*/
/************************************************************************/
/* ���0x83�Ľ������£�                                                 */
/* �ο�Add/Cmp/Sub�������ʽ��Intel��Ƥ��604/726/1776ҳ���Է��֣�       */
/*     0x83��ʾ��"Add sign-extended imm8 to r/m32."��                   */
/*     0x83��ʾ��"Compare imm8 with r/m32."��                           */
/*     0x83��ʾ��"Subtract sign-extended imm8 from r/m32."��            */
/* ���0x83�Ľ��͹������£�                                             */
/*     ���Ȳ鿴Intel��Ƥ��2517ҳ��One-byte Opcode Map��               */
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
/*     ���Ȳ鿴Intel��Ƥ��2517ҳ��One-byte Opcode Map��               */
/*     �������Immediate Grp 1����ʽ��Ev,lz��˵����������˶���ָ�   */
/*     Ҫ���ݺ����һ���ֽڵ�ModR/Mȷ����Ӧ��ָ��                       */
/*     �������к���Ev���ţ���ô���������һ���ֽھ���MODR/M��           */
/*     ͨ�����ModR/M�õ�ModR/M[5��3]�Ϳ����ƶϳ�ָ�                 */
/************************************************************************/
void gen_opTwoInteger(int opc, int op)
{
	int dst_storage_class, src_storage_class, c;
	// ���ջ��Ԫ�ز��Ƿ��ţ�Ҳ�Ͳ���ȫ�ֱ����ͺ������壬
	// �������Ҳ����ջ�б������Ǿ�ֻ���ǳ�����
	if ((operand_stack_top->storage_class & (SC_VALMASK | SC_LOCAL | SC_SYM)) == SC_GLOBAL)
	{
		dst_storage_class = load_one(REG_ANY, operand_stack_last_top);
		// ���c��һ��8λ��������
		c = operand_stack_top->value;
		if (c == (char)c)
		{
			// ADC--Add with Carry			83 /2 ib	ADC r/m32,imm8	Add with CF sign-extended imm8 to r/m32
			// ADD--Add						83 /0 ib	ADD r/m32,imm8	Add sign-extended imm8 from r/m32
			// SUB--Subtract				83 /5 ib	SUB r/m32,imm8	Subtract sign-extended imm8 to r/m32
			// CMP--Compare Two Operands	83 /7 ib	CMP r/m32,imm8	Compare imm8 with r/m32
			// �����ǼӼ������ǱȽϲ�����ֻҪ�ǵ�һ���ֽڶ���0x83��
			gen_opcodeOne(0x83);
			// ֻ�еڶ����ֽڲ�ͬ��
			gen_modrm(ADDR_REG, opc, dst_storage_class, NULL, 0);
			// �������������ͺ��ˡ�
			gen_byte(c);
		}
		// �������32λ����������
		else
		{
			// ADD--Add					    81 /0 id	ADD r/m32,imm32	Add sign-extended imm32 to r/m32
			// SUB--Subtract				81 /5 id	SUB r/m32,imm32	Subtract sign-extended imm32 from r/m32
			// CMP--Compare Two Operands	81 /7 id	CMP r/m32,imm32	Compare imm32 with r/m32
			gen_opcodeOne(0x81);
			gen_modrm(ADDR_REG, opc, dst_storage_class, NULL, 0);
			gen_byte(c);
		}
	}
	// ���ջ��Ԫ�ز��ǳ�������һ���ֽھͺ�opc�й�ϵ�ˡ�
	else
	{
		load_two(REG_ANY, REG_ANY);
		dst_storage_class  = operand_stack_last_top->storage_class;
		src_storage_class  = operand_stack_top->storage_class;
		// ������ӷ�ʱ��opc��0������ʱ��opc��5���Ƚ�ʱ��opc��7��
		// ����ϣ����������ǣ��ӷ�ʱΪ0x01������ʱΪ0x29���Ƚ�ʱΪ0x39��
		// �鿴Intel��Ƥ��2517ҳ��One-byte Opcode Map��
		// �����ҵ���Ӧ��ָ��ֱ�ΪADD��SUB��CMP
		gen_opcodeOne((opc << 3) | 0x01);
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
		// ���ջ��Ԫ�ز��Ƿ��ţ�Ҳ�Ͳ���ȫ�ֱ����ͺ������壬
		// �������Ҳ����ջ�б������Ǿ�ֻ���ǳ�����
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



