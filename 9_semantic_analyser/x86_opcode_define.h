#ifndef X86_OPCODE_DEFINE_H
#define X86_OPCODE_DEFINE_H

#define OPCODE_STORE_T_SHORT_PREFIX			0x66
#define OPCODE_STORE_CHAR_OPCODE			0x88
#define OPCODE_STORE_SHORT_INT_OPCODE       0x89

#define OPCODE_IMUL_HIGH_BYTE  				0x0f
#define OPCODE_IMUL_LOW_BYTE   				0xaf
#define OPCODE_CDQ             				0x99
#define OPCODE_IDIV            				0xf7

// Used by function gen_modrm
#define MODRM_MOD_OFFSET                    0x06
#define MODRM_REG_OPCODE_OFFSET             0x03

#define MODRM_EFFECTIVE_ADDRESS_REG         0xC0
#define MODRM_EFFECTIVE_ADDRESS_DISP32      0x05
#define MODRM_EFFECTIVE_ADDRESS_DISP8_EBP   0x45
#define MODRM_EFFECTIVE_ADDRESS_DISP32_EBP  0x85

#define MODRM_EFFECTIVE_ADDRESS_REG_ADD_MOD     0x00

// 参见Intel白皮书2517页的One-byte Opcode Map表。
#define ONE_BYTE_OPCODE_ADD_GIGH_FIVE_BYTES     0x00
#define ONE_BYTE_OPCODE_SUB_GIGH_FIVE_BYTES     0x05
#define ONE_BYTE_OPCODE_CMP_GIGH_FIVE_BYTES     0x07

#define ONE_BYTE_OPCODE_IMME_GRP_TO_IMM8        0x83
#define ONE_BYTE_OPCODE_IMME_GRP_TO_IMM32       0x81



#endif