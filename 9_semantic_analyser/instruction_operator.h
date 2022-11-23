#ifndef INSTRUCTION_OPERATOR_H
#define INSTRUCTION_OPERATOR_H

#include "operand_stack.h"

int load_one(int rc, Operand * opd);
void load_two(int rc1, int rc2);
void store_zero_to_one();

#endif