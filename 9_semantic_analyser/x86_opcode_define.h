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
// 参考SETcc的命令格式在Intel白皮书1718页可以发现：
//     0F 9F表示是"Set byte if greater (ZF=0 and SF=OF)."。
#define OPCODE_Set_byte_if_greater_HIGH_BYTE        0x0F
// 参考JMP的命令格式在Intel白皮书1064页可以发现：
//     E9 cd表示是"Jump near, relative, RIP = RIP + 32-bit 
//                 displacement sign extended to 64-bits."。
#define OPCODE_JUMP_NEAR                            0xE9
// 参考JMP的命令格式在Intel白皮书1064页可以发现：
//     EB cb表示是"Jump short, RIP = RIP + 8-bit displacement sign 
//                 extended to 64-bits."。
#define OPCODE_JUMP_SHORT                           0xEB
#endif