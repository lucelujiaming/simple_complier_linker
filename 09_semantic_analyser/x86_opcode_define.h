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

// 参见Intel白皮书2517页的One-byte Opcode Map表。
#define ONE_BYTE_OPCODE_ADD_GIGH_FIVE_BYTES         0x00
#define ONE_BYTE_OPCODE_SUB_GIGH_FIVE_BYTES         0x05
#define ONE_BYTE_OPCODE_CMP_GIGH_FIVE_BYTES         0x07

#define ONE_BYTE_OPCODE_IMME_GRP_TO_IMM8            0x83
#define ONE_BYTE_OPCODE_IMME_GRP_TO_IMM32           0x81

#define ONE_BYTE_OPCODE_IMME_GRP_TO_IMM32           0x81

// 在这里加法时，opc是0。减法时，opc是5。比较时，opc是7。
// 因此上，计算结果就是：加法时为0x01。减法时为0x29。比较时为0x39。
// 查看Intel白皮书2517页的One-byte Opcode Map表。
// 可以找到对应的指令，分别为ADD，SUB，CMP
#define ONE_BYTE_OPCODE_LOW_THREE_BYTE_EV_GV        0x01
 
 
// 参见Intel白皮书1161页的MOV的命令格式。
//     0xB8表示是"Move imm32 to r32."。
#define OPCODE_MOVE_IMM32_TO_R32  				    0xB8
// 参见Intel白皮书1101页的LEA的命令格式。
//     0x8D表示是"Store effective address for m in register r32."。
#define OPCODE_LEA_EFFECTIVE_ADDRESS_IN_R32  	    0x8D
// 参考REP MOVS的命令格式在Intel白皮书1671页可以发现：
//     F3 A4表示是"Move (E)CX bytes from DS:[(E)SI] to ES:[(E)DI]."。
#define OPCODE_REP_PREFIX                           0xF3
#define OPCODE_MOVE_ECX_BYTES_DS_ESI_TO_ES_EDI      0xA4
// 参考MOVSX的命令格式在Intel白皮书1250页可以发现：
//     0F BE表示是" Move byte to doubleword with sign-extension."。
#define OPCODE_MOVSX_BYTE_TO_DOUBLEWORD_HIGH_BYTE   0x0F
#define OPCODE_MOVSX_BYTE_TO_DOUBLEWORD_LOW_BYTE    0xBE
//     0F BF表示是"Move word to doubleword, with sign-extension."。
#define OPCODE_MOVSX_WORD_TO_DOUBLEWORD_HIGH_BYTE   0x0F
#define OPCODE_MOVSX_WORD_TO_DOUBLEWORD_LOW_BYTE    0xBF
// 参考MOV的命令格式在Intel白皮书1161页可以发现：
//     8B表示是"Move r/m32 to r32."。
#define OPCODE_MOV_RM32_TO_R32                      0x8B
// 参考MOV的命令格式在Intel白皮书1161页可以发现：
//     89 /r表示是"Move r32 to r/m32."。
#define OPCODE_MOVE_R32_TO_RM32                     0x89
// 参考SETcc的命令格式在Intel白皮书1718页可以发现：
//     0F 9F表示是"Set byte if greater (ZF=0 and SF=OF)."。
#define OPCODE_SET_BYTE_IF_GREATER_HIGH_BYTE        0x0F

// 参考JMP的命令格式在Intel白皮书1064页可以发现：
//     E9 cd表示是"Jump near, relative, RIP = RIP + 32-bit 
//                 displacement sign extended to 64-bits."。
#define OPCODE_JUMP_NEAR                            0xE9
// 参考JMP的命令格式在Intel白皮书1064页可以发现：
//     EB cb表示是"Jump short, RIP = RIP + 8-bit displacement sign 
//                 extended to 64-bits."。
#define OPCODE_JUMP_SHORT                           0xEB
// 参考Jcc的命令格式在Intel白皮书1060页可以发现：
//     0F 8F cw/cd 表示是"Jump near if greater (ZF=0 and SF=OF)."。
#define OPCODE_JCC_JUMP_NEAR_IF_GREATER             0x0f


// 参考POP的命令格式在Intel白皮书1510页可以发现：
//     58 + rd表示是"Pop top of stack into r32; increment stack pointer."。
#define OPCODE_POP_STACK_TOP_INTO_R32               0x58
	
// 参考RET的命令格式在Intel白皮书1675页可以发现：
//     C3表示是"Near return to calling procedure."。
#define OPCODE_NEAR_RETURN                          0xC3
#define OPCODE_NEAR_RETURN_AND_POP_IMM16_BYTES      0xC2
// 参考PUSH的命令格式在Intel白皮书1633页可以发现：
//     50+rd表示是"Push r32."。
#define OPCODE_PUSH_R32                             0x50

// 参考SUB的命令格式在Intel白皮书1776页可以发现：
//     81 /5 id表示是"Subtract imm32 from r/m32."。
#define OPCODE_SUBTRACT_IMM32_FROM_RM32             0x81

// 参考CALL的命令格式在Intel白皮书695页可以发现：
//     E8 cd表示是"Call near, relative, displacement relative to next instruction. "。
#define OPCODE_CALL_NEAR_RELATIVE_32_BIT_DISPLACE   0xE8
#define OPCODE_CALL_NEAR_ABSOLUTE                   0XFF
#define OPCODE_CALL_NEAR_ABSOLUTE_32_RM32_ADDRESS   0xd0

// 参考TEST的命令格式在Intel白皮书1801页可以发现：
//     85 /r表示是" AND r32 with r/m32; set SF, ZF, PF according to result."。
#define OPCODE_TEST_AND_R32_WITH_RM32               0x85

// CMP影响位在Intel白皮书417页。
// 也就是Table B-1. EFLAGS Condition Codes
#define OPCODE_CONDITION_CODES_EQUAL                0x84
#define OPCODE_CONDITION_CODES_NOT_EQUAL            0x85
#define OPCODE_CONDITION_CODES_LESS                 0X8C
#define OPCODE_CONDITION_CODES_LESS_OR_EQUAL        0X8E
#define OPCODE_CONDITION_CODES_GREATER              0X8F
#define OPCODE_CONDITION_CODES_GREATER_OR_EQUAL     0x8d

#endif