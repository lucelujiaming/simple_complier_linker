#ifndef OPCODE_GENERATOR_H
#define OPCODE_GENERATOR_H

#include "symbol_table.h"

// Operand functions
void gen_byte(char write_byte);
void gen_prefix(char opcode);
void gen_opcodeOne(unsigned char opcode);
void gen_opcodeTwo(unsigned char first, unsigned char second);
void gen_dword(unsigned int c);
void gen_addr32(int storage_class, Symbol * sym, int value);

#endif

