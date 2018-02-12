#ifndef HW_H
#define HW_H

#include "const.h"
#include "instruction.h"

int validateBaseAddress(char * base);
int matches(char * str1, char * str2);
Opcode decodeSpecial(int val);
int decodeBCOND(int val, Opcode *opcode);
int decodeExtra(int val, Type type, Opcode opcode, unsigned int addr, int *error);
int flipEnd(int val);

#endif
