#include "hw1.h"
#include <stdlib.h>

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif



/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 */

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 1 if validation succeeds and 0 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 1 if validation succeeds and 0 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validateBaseAddress(char * base){
    int i = 0;
    int allZero = 1;
    while(base[i] != '\0'){
        if(i > 7){
//            printf("too long");
            return 0;
        }
        if(base[i] != '0'){
//            printf("non zero detected");
            allZero = 0;
        }
//        if(base[i] != '0' || base[i] != '1'|| base[i] != '2'|| base[i] != '3'|| base[i] != '4'
//            || base[i] != '5'|| base[i] != '6'|| base[i] != '7'|| base[i] != '8'|| base[i] != '9'
//            || base[i] != 'A'|| base[i] != 'B'|| base[i] != 'C'|| base[i] != 'D'|| base[i] != 'E'
//            || base[i] != 'F'|| base[i] != 'a'|| base[i] != 'b'|| base[i] != 'c'|| base[i] != 'd'
//            || base[i] != 'e'|| base[i] != 'f'){
        if((base[i] >= '0' && base[i] <= '9') || (base[i] >= 'a' && base[i] <= 'f') ||
            (base[i] >= 'A' && base[i] <= 'F')){
            i++;
            continue;
        }
        else{
//            printf("%dinvalid char found" , base[i]);
            return 0;
        }
        i++;
    }
    i--;
    if(allZero){
        return 1;
    }
    if(i < 3){
//        printf("too short");
        return 0;
    }
    int length = i;
    while(i > length - 3){
        if(base[i] != '0'){
//            printf("not divisible by 4096");
            return 0;
        }
        else{
//            printf("%dchar" , base[i]);
        }
        i--;
    }

    return 1;
}
int matches(char * str1, char * str2){
    int i = 0;
    while(str1[i] != '\0' && str2[i] != '\0'){
        //printf("%c\n", str1[i]);
        //printf("%c\n", str2[i]);
        if(str1[i] != str2[i]){
            return 0;
        }
        i++;

    }
    if(str1[i] == str2[i]){
        //printf("match!\n");
        return 1;
    }
    else{
        return 0;
    }
}
int validargs(int argc, char **argv)
{

    if(argc == 0 || argc == 1){
        return 0;
    }
    int i;
    int a_or_d = 0;
    //int hasA = 0;
    int hasD = 0;
    int hasB = 0;
    int hasH = 0;
    int hasE = 0;
    int lastB = 0;
    int lastE = 0;
    int bigE = 0;
    //int littleE = 0;
    char * baseAddress;
    for(i = 1; i < argc; i++){
        if(i == 1){
            if(argv[i][0] == '-'){
                if(matches("-h", argv[i])){
//                    printf("h flag detected");
                  //fix this later for index out of bounds
                  //  hasH = 1;
                  //  continue;
                    global_options = 1;
                    return 1;
                }
                else if(matches("-a", argv[i]) || matches("-d", argv[i])){
                    a_or_d = 1;
                    if(matches("-d", argv[i])){
//                        printf("d flag detected");
                        //hasA = 1;
                        hasD = 1;
                    }
                    else{
//                        printf("a flag detected");
                    }

                }
                else{
                    return 0;
                }
            }
            else{
                return 0;
            }
        }
        else{
            if(lastB){
//                printf("checking base address");
                if(validateBaseAddress(argv[i])){
//                    printf("base address valid");
                    lastB = 0;
                    baseAddress = argv[i];
                }
                else{
//                    printf("invalid base address");
                    return 0;
                }
            }
            else if(lastE){
//                printf("checking endian");
                if(matches("b", argv[i])){
                    bigE = 1;
                    lastE = 0;
//                    printf("big endian");
                }
                else if(matches("l", argv[i])){
                    //littleE = 1;
                    lastE = 0;
//                    printf("little endian");
                }
                else{
                    return 0;
                }
            }
            else if(matches("-a", argv[i]) || matches("-d", argv[i])){
                if(a_or_d || hasB || hasE){
                    return 0;

                }
                else{
                    a_or_d = 1;
//                    printf("a/d flag detected");
                }
            }
            else if(matches("-h", argv[i])){
                return 0;
            }
            else if(matches("-b", argv[i])){
                if(!a_or_d || hasE){
                    return 0;
                }
                else{
//                    printf("b flag detected");
                    hasB = 1;
                    lastB = 1;
                }
            }
            else if (matches("-e", argv[i])){
                if(!a_or_d){
                    return 0;
                }
                else{
//                    printf("e flag detected");
                    hasE = 1;
                    lastE = 1;
                }
            }
            else{
                return 0;
            }


        }
    }
    if(lastE || lastB){
        return 0;
    }
    //determine what global options should be set to
    global_options = 0;
    if(hasD){
        global_options = global_options | 2;
    }
    if(bigE){
            global_options = global_options | 4;
    }
    if(hasB){
        int decValue = strtol(baseAddress, NULL, 16);
        global_options = global_options | decValue;
    }
    return 1;



}

/**
 * @brief Computes the binary code for a MIPS machine instruction.
 * @details This function takes a pointer to an Instruction structure
 * that contains information defining a MIPS machine instruction and
 * computes the binary code for that instruction.  The code is returne
 * in the "value" field of the Instruction structure.
 *
 * @param ip The Instruction structure containing information about the
 * instruction, except for the "value" field.
 * @param addr Address at which the instruction is to appear in memory.
 * The address is used to compute the PC-relative offsets used in branch
 * instructions.
 * @return 1 if the instruction was successfully encoded, 0 otherwise.
 * @modifies the "value" field of the Instruction structure to contain the
 * binary code for the instruction.
 */
int encode(Instruction *ip, unsigned int addr) {
    Instr_info instructionInfo = *ip->info;
    Instruction instruction = *ip;
    Opcode op = instructionInfo.opcode;
    //printf("%d\n", op);
    int binVal = 0;
    //encode opcode
    if(op == ILLEGL){
        return 0;
    }
    if(instructionInfo.type == NTYP){
        return 0;
    }
    else if(op == OP_BLTZ){
        //printf("int bltz\n");
        binVal = binVal | 0b00000100000000000000000000000000;
    }
    else if(op == OP_BGEZ){
        //printf("in bgez\n");
        binVal = binVal | 0b00000100000000010000000000000000;
    }
    else if(op == OP_BLTZAL){
        //printf("in bltzal\n");
        binVal = binVal | 0b00000100000100000000000000000000;
    }
    else if(op == OP_BGEZAL){
        //printf("in bgezal\n");
        binVal = binVal | 0b00000100000100010000000000000000;
    }
    else{
        int opIndex = 0;
        //int special = 1;

        for(int i = 0; i < 64; i++){
            if(opcodeTable[i] == op){
                opIndex = i;
                //special = 0;
                opIndex = opIndex << 26;
                binVal = binVal | opIndex;
                break;
            }
        }
        if(opIndex == 0){
            int opFound = 0;
            for(int i = 0; i < 64; i++){
                if(specialTable[i] == op){
                    opIndex = i;
                    binVal = binVal | opIndex;
                    opFound = 1;
                    break;
                }
            }
            if(!opFound){
                return 0;
            }
        }

    }

    //encode sources
    //printf("hello\n");
    for(int i = 0; i < 3; i++){
        if(instructionInfo.srcs[i] == RS){
            //printf("rs\n");
            int rs = instruction.args[i];
            rs = rs << 21;
            binVal = binVal | rs;
        }
        else if(instructionInfo.srcs[i] == RT){
            //printf("rt\n");
            int rt = instruction.args[i];
            rt = rt << 16;
            binVal = binVal | rt;
        }
        else if(instructionInfo.srcs[i] == RD){
            //printf("rd\n");
            int rd = instruction.args[i];
            rd = rd << 11;
            binVal = binVal | rd;
        }
        else if(instructionInfo.srcs[i] == EXTRA){
            //printf("extra\n");
            if(op == OP_BREAK || instructionInfo.type == RTYP){
                int extra = instruction.extra;
                extra = extra << 6;
                binVal = binVal | extra;
            }
            else if(instructionInfo.type == ITYP){

                if(op == OP_BEQ || op == OP_BGEZ || op == OP_BGEZAL ||
                    op == OP_BGTZ || op == OP_BLEZ || op == OP_BLTZ ||
                    op == OP_BLTZAL || op == OP_BNE){
                    int extra = instruction.extra;

                    extra -= 4;
                    extra -= addr;
                    extra = extra >> 2;
                    extra = extra & 0x0000FFFF;
                    //extra = extra << 16;
                    //extra = extra >> 16;
                    binVal = binVal | extra;
                }
                else{

                    int extra = instruction.extra;
                    //printf("%d\n", extra);
                    //extra = extra << 16;
                    //extra = extra >> 16;
                    extra = extra & 0x0000FFFF;
                    binVal = binVal | extra;
                }
            }
            else if(instructionInfo.type == JTYP){
                int extra = instruction.extra;
                int msbExtra = extra & 0xF0000000;
                int msbAddr = addr & 0xF0000000;
                if(msbAddr != msbExtra){
                    return 0;
                }
                extra = extra << 4;
                extra = extra >> 6;
                binVal = binVal | extra;
            }
            else{
                return 0;
            }

        }
        else if(instructionInfo.srcs[i] == NSRC){
            //printf("nsrc\n");
            continue;
        }
        else{
            return 0;
        }

    }
    (*ip).value = binVal;

    return 1;
}

/**
 * @brief Decodes the binary code for a MIPS machine instruction.
 * @details This function takes a pointer to an Instruction structure
 * whose "value" field has been initialized to the binary code for
 * MIPS machine instruction and it decodes the instruction to obtain
 * details about the type of instruction and its arguments.
 * The decoded information is returned by setting the other fields
 * of the Instruction structure.
 *
 * @param ip The Instruction structure containing the binary code for
 * a MIPS instruction in its "value" field.
 * @param addr Address at which the instruction appears in memory.
 * The address is used to compute absolute branch addresses from the
 * the PC-relative offsets that occur in the instruction.
 * @return 1 if the instruction was successfully decoded, 0 otherwise.
 * @modifies the fields other than the "value" field to contain the
 * decoded information about the instruction.
 */
int decode(Instruction *ip, unsigned int addr) {
    //printf("hello\n");
    Instruction instruction = *ip;
    unsigned int val = instruction.value;
    unsigned int opcodeBinary = val >> 26;
    //printf("%d\n", opcodeBinary);
    Opcode opcode = opcodeTable[opcodeBinary];
    if(opcode == ILLEGL){
            return 0;
        }
    if(opcode == SPECIAL){
        opcode = decodeSpecial(val);
        if(opcode == ILLEGL){
            return 0;
        }
    }
    if(opcode == BCOND){
        //printf("bcond\n");
        if(!decodeBCOND(val, &opcode)){
            return 0;
        }
    }
    Instr_info info = instrTable[opcode];
    (*ip).info = &info;
    Type type = info.type;
    (*ip).regs[0] = 0;
    (*ip).regs[1] = 0;
    (*ip).regs[2] = 0;
    for(int i = 0; i < 3; i++){
        Source source = info.srcs[i];
        if(source == RS){
            unsigned int rs = val << 6;
            rs = rs >> 27;
            (*ip).regs[0] = rs;
            (*ip).args[i] = rs;

        }
        else if(source == RT){
            unsigned int rt = val << 11;
            rt = rt >> 27;
            (*ip).regs[1] = rt;
            (*ip).args[i] = rt;

        }
        else if(source == RD){
            unsigned int rd = val << 16;
            rd = rd >> 27;
            (*ip).regs[2] = rd;
            (*ip).args[i] = rd;

        }
        else if(source == EXTRA){
            int error = 0;
            (*ip).extra = decodeExtra(val, type, opcode, addr, &error);
            if(error){
                return 0;
            }
            (*ip).args[i] = (*ip).extra;
        }
        else{
            (*ip).args[i] = 0;
        }
    }
    return 1;


}
Opcode decodeSpecial(int val){
    unsigned int specialIndex = val << 26;
    specialIndex = specialIndex >> 26;
    Opcode opcode = specialTable[specialIndex];
    //printf("%d\n", val);
    //printf("%d\n", specialIndex);
    return opcode;
}
int decodeBCOND(int val, Opcode *opcode){
    //printf("in bcond\n");
    unsigned int binary = val << 11;
    binary = binary >> 27;

    if(binary == 0){
        *opcode = OP_BLTZ;
    }
    else if(binary == 1){
        *opcode = OP_BGEZ;
    }
    else if(binary == 16){
        *opcode = OP_BLTZAL;
    }
    else if(binary == 17){
        //printf("bgezal\n");
        *opcode = OP_BGEZAL;
    }
    else{
        return 0;
    }
    return 1;

}
int decodeExtra(int val, Type type, Opcode opcode, unsigned int addr, int *error)
{    if(opcode == OP_BREAK){
        unsigned int arg = val << 6;
        arg = arg >> 12;
        return arg;
    }
    else if(type == RTYP){
        unsigned int arg = val << 21;
        arg = arg >> 27;
        return arg;
    }
    else if(type == ITYP){
        unsigned int arg = val << 16;
        arg = arg >> 16;
        if(0x8000 & arg){
            arg = arg | 0xFFFF0000;
        }
        if(opcode == OP_BEQ || opcode == OP_BGEZ || opcode == OP_BGEZAL || opcode == OP_BLEZ ||
            opcode == OP_BLTZ || opcode == OP_BLTZAL || opcode == OP_BNE || opcode == OP_BGTZ ){

            arg = arg << 2;

            addr += 4;

            arg += addr;

            //printf("%d\n", arg);

            return arg;
        }
        else{
            return arg;
        }
    }
    else{ // j type
        unsigned int arg = val << 6;
        arg = arg >> 6;
        arg = arg << 2;
        addr += 4;
        int absAddr = addr & 0xF0000000;
        arg += absAddr;
        int msbArg = arg & 0xF0000000;
        int msbAddr = addr & 0xF0000000;
        if(msbAddr != msbArg){
            *error = 1;
        }
        return arg;
    }

}

int flipEnd(int val){
    unsigned int newVal = 0;
    unsigned int byte1 = val << 24;
    newVal = newVal | byte1;
    unsigned int byte2 = val >> 8;
    byte2 = byte2 << 24;
    byte2 = byte2 >> 8;
    newVal = newVal | byte2;
    unsigned int byte3 = val << 8;
    byte3 = byte3 >> 24;
    byte3 = byte3 << 8;
    newVal = newVal | byte2;
    unsigned int byte4 = val >> 24;
    newVal = newVal | byte2;
    return newVal;
}
