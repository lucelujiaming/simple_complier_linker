#ifndef X86_OPCODE_DEFINE_H
#define X86_OPCODE_DEFINE_H

#define OPCODE_STORE_T_SHORT_PREFIX			        0x66
#define OPCODE_STORE_CHAR_OPCODE			        0x88
#define OPCODE_STORE_SHORT_INT_OPCODE               0x89

#define OPCODE_IMUL_HIGH_BYTE  				        0x0f
#define OPCODE_IMUL_LOW_BYTE   				        0xaf
#define OPCODE_CDQ             				        0x99
#define OPCODE_IDIV            				        0xf7

// Used by function gen_modrm
#define MODRM_MOD_OFFSET                            0x06
#define MODRM_REG_OPCODE_OFFSET                     0x03

#define MODRM_EFFECTIVE_ADDRESS_REG                 0xC0
#define MODRM_EFFECTIVE_ADDRESS_DISP32              0x05
#define MODRM_EFFECTIVE_ADDRESS_DISP8_EBP           0x45
#define MODRM_EFFECTIVE_ADDRESS_DISP32_EBP          0x85

#define MODRM_EFFECTIVE_ADDRESS_REG_ADD_MOD         0x00

// �μ�Intel��Ƥ��2517ҳ��One-byte Opcode Map��
#define ONE_BYTE_OPCODE_ADD_GIGH_FIVE_BYTES         0x00
#define ONE_BYTE_OPCODE_SUB_GIGH_FIVE_BYTES         0x05
#define ONE_BYTE_OPCODE_CMP_GIGH_FIVE_BYTES         0x07

#define ONE_BYTE_OPCODE_IMME_GRP_TO_IMM8            0x83
#define ONE_BYTE_OPCODE_IMME_GRP_TO_IMM32           0x81

#define ONE_BYTE_OPCODE_IMME_GRP_TO_IMM32           0x81

// ������ӷ�ʱ��opc��0������ʱ��opc��5���Ƚ�ʱ��opc��7��
// ����ϣ����������ǣ��ӷ�ʱΪ0x01������ʱΪ0x29���Ƚ�ʱΪ0x39��
// �鿴Intel��Ƥ��2517ҳ��One-byte Opcode Map��
// �����ҵ���Ӧ��ָ��ֱ�ΪADD��SUB��CMP
#define ONE_BYTE_OPCODE_LOW_THREE_BYTE_EV_GV        0x01
 
 
// �μ�Intel��Ƥ��1161ҳ��MOV�������ʽ��
//     0xB8��ʾ��"Move imm32 to r32."��
#define OPCODE_MOVE_IMM32_TO_R32  				    0xB8
// �μ�Intel��Ƥ��1101ҳ��LEA�������ʽ��
//     0x8D��ʾ��"Store effective address for m in register r32."��
#define OPCODE_LEA_EFFECTIVE_ADDRESS_IN_R32  	    0x8D
// �ο�REP MOVS�������ʽ��Intel��Ƥ��1671ҳ���Է��֣�
//     F3 A4��ʾ��"Move (E)CX bytes from DS:[(E)SI] to ES:[(E)DI]."��
#define OPCODE_REP_PREFIX                           0xF3
#define OPCODE_MOVE_ECX_BYTES_DS_ESI_TO_ES_EDI      0xA4
// �ο�MOVSX�������ʽ��Intel��Ƥ��1250ҳ���Է��֣�
//     0F BE��ʾ��" Move byte to doubleword with sign-extension."��
#define OPCODE_MOVSX_BYTE_TO_DOUBLEWORD_HIGH_BYTE   0x0F
#define OPCODE_MOVSX_BYTE_TO_DOUBLEWORD_LOW_BYTE    0xBE
//     0F BF��ʾ��"Move word to doubleword, with sign-extension."��
#define OPCODE_MOVSX_WORD_TO_DOUBLEWORD_HIGH_BYTE   0x0F
#define OPCODE_MOVSX_WORD_TO_DOUBLEWORD_LOW_BYTE    0xBF
// �ο�MOV�������ʽ��Intel��Ƥ��1161ҳ���Է��֣�
//     8B��ʾ��"Move r/m32 to r32."��
#define OPCODE_MOV_RM32_TO_R32                      0x8B
// �ο�MOV�������ʽ��Intel��Ƥ��1161ҳ���Է��֣�
//     89 /r��ʾ��"Move r32 to r/m32."��
#define OPCODE_MOVE_R32_TO_RM32                     0x89
// �ο�SETcc�������ʽ��Intel��Ƥ��1718ҳ���Է��֣�
//     0F 9F��ʾ��"Set byte if greater (ZF=0 and SF=OF)."��
#define OPCODE_SET_BYTE_IF_GREATER_HIGH_BYTE        0x0F

// �ο�JMP�������ʽ��Intel��Ƥ��1064ҳ���Է��֣�
//     E9 cd��ʾ��"Jump near, relative, RIP = RIP + 32-bit 
//                 displacement sign extended to 64-bits."��
#define OPCODE_JUMP_NEAR                            0xE9
// �ο�JMP�������ʽ��Intel��Ƥ��1064ҳ���Է��֣�
//     EB cb��ʾ��"Jump short, RIP = RIP + 8-bit displacement sign 
//                 extended to 64-bits."��
#define OPCODE_JUMP_SHORT                           0xEB
// �ο�Jcc�������ʽ��Intel��Ƥ��1060ҳ���Է��֣�
//     0F 8F cw/cd ��ʾ��"Jump near if greater (ZF=0 and SF=OF)."��
#define OPCODE_JCC_JUMP_NEAR_IF_GREATER             0x0f


// �ο�POP�������ʽ��Intel��Ƥ��1510ҳ���Է��֣�
//     58 + rd��ʾ��"Pop top of stack into r32; increment stack pointer."��
#define OPCODE_POP_STACK_TOP_INTO_R32               0x58
	
// �ο�RET�������ʽ��Intel��Ƥ��1675ҳ���Է��֣�
//     C3��ʾ��"Near return to calling procedure."��
#define OPCODE_NEAR_RETURN                          0xC3
#define OPCODE_NEAR_RETURN_AND_POP_IMM16_BYTES      0xC2
// �ο�PUSH�������ʽ��Intel��Ƥ��1633ҳ���Է��֣�
//     50+rd��ʾ��"Push r32."��
#define OPCODE_PUSH_R32                             0x50

// �ο�SUB�������ʽ��Intel��Ƥ��1776ҳ���Է��֣�
//     81 /5 id��ʾ��"Subtract imm32 from r/m32."��
#define OPCODE_SUBTRACT_IMM32_FROM_RM32             0x81

// �ο�CALL�������ʽ��Intel��Ƥ��695ҳ���Է��֣�
//     E8 cd��ʾ��"Call near, relative, displacement relative to next instruction. "��
#define OPCODE_CALL_NEAR_RELATIVE_32_BIT_DISPLACE   0xE8
#define OPCODE_CALL_NEAR_ABSOLUTE                   0XFF
#define OPCODE_CALL_NEAR_ABSOLUTE_32_RM32_ADDRESS   0xd0

// �ο�TEST�������ʽ��Intel��Ƥ��1801ҳ���Է��֣�
//     85 /r��ʾ��" AND r32 with r/m32; set SF, ZF, PF according to result."��
#define OPCODE_TEST_AND_R32_WITH_RM32               0x85

// CMPӰ��λ��Intel��Ƥ��417ҳ��
// Ҳ����Table B-1. EFLAGS Condition Codes
#define OPCODE_CONDITION_CODES_EQUAL                0x84
#define OPCODE_CONDITION_CODES_NOT_EQUAL            0x85
#define OPCODE_CONDITION_CODES_LESS                 0X8C
#define OPCODE_CONDITION_CODES_LESS_OR_EQUAL        0X8E
#define OPCODE_CONDITION_CODES_GREATER              0X8F
#define OPCODE_CONDITION_CODES_GREATER_OR_EQUAL     0x8d

#endif