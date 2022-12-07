#ifndef REG_MANAGER_H
#define REG_MANAGER_H

int allocate_reg(int rc);

void spill_reg(char r);
void spill_regs();

#endif
