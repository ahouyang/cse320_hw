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

#include "hw1.h"
#include "debug.h"

int main(int argc, char **argv)
{
    if(!validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    debug("Options: 0x%X", global_options);
    if(global_options & 0x1) {
        USAGE(*argv, EXIT_SUCCESS);

    }
    //get base address
    //printf("hello\n");
    int bigE = 0;
    if(global_options & 0x4){
        bigE = 1;
        //printf("bigE\n");
    }
    int currentAddress = global_options;
    currentAddress = currentAddress >> 12;
    currentAddress = currentAddress << 12;
    if(global_options & 0x2){
        //disassemble
        int index = 0;
        while(1){
            if(feof(stdin)){
                break;
            }
            unsigned int val = 0;
            //fread(&val, sizeof(int), 1, stdin);
            fread(&val, sizeof(int), 1,stdin);
            Instruction ip;
            if(bigE){
                //printf("bigE\n");
                ip.value = flipEnd(val);
            }
            else{
                ip.value = val;
                //printf("%d\n", val[index]);
            }
            if(!decode(&ip, currentAddress)){
                return EXIT_FAILURE;
            }
            char mnemonic[120];
            if(!feof(stdin)){
                printf(ip.info->format, ip.args[0], ip.args[1], ip.args[2]);
                printf("\n");
            }
            index++;
            currentAddress += 4;
            //printf(mnemonic);
        }
    }
    else{
        //assemble
        //printf("hello\n");
        char line[120];
        Instruction ip;
        int index = 0;
        while(1){
            if(feof(stdin)){
                break;
            }
            //printf("%d\n", index);
            fgets(line, 120, stdin);
            //printf("got input\n");
            //printf("%s\n", line);
            //printf("%d\n", matches(line,"syscall\n"));
            int foundInstr = 0;
            int arguments[3];
            for(int j = 0; j < 64; j++){
                //printf("infoloop?\n");
                arguments[0] = 0;
                arguments[1] = 0;
                arguments[2] = 0;
                int result = sscanf(line, instrTable[j].format, &arguments[0]
                    , &arguments[1], &arguments[2]);
                if(result == 0){
                    //printf("not found\n");
                    if(matches(line, "syscall\n")){
                        ip.info = &instrTable[61];
                        foundInstr = 1;
                        break;
                    }
                    else if(matches(line, "ILLEGAL\n")){
                        ip.info = &instrTable[0];
                        foundInstr = 1;
                        break;
                    }
                    else if(matches(line, "Unimplemented\n")){
                        ip.info = &instrTable[63];
                        foundInstr = 1;
                        break;
                    }
                    else if(matches(line, "rfe\n")){
                        ip.info = &instrTable[41];
                        foundInstr = 1;
                        break;
                    }
                    else{
                        index++;
                        continue;
                    }
//                    if(!(matches(line,"syscall\n") || matches(line,"ILLEGAL\n")
//                        || matches(line,"Unimplemented\n") || matches(line,"rfe\n"))){
                        //printf("not%d\n", index);

//                        index++;
//                        continue;
//                    }
                    //printf("found\n");
                }
                //printf("found\n");
                ip.info = &instrTable[j];
                foundInstr = 1;
                break;
            }
            if(!foundInstr){
                return EXIT_FAILURE;
            }
            //printf("building instruction\n");
            ip.regs[0]= 0;
            ip.regs[1]= 0;
            ip.regs[2]= 0;
            ip.args[0]= 0;
            ip.args[1]= 0;
            ip.args[2]= 0;
            for(int k = 0; k < 3; k++){
                //printf("srcs loop?\n");
                ip.args[k] = arguments[k];
                Instr_info currentInfo = *(ip.info);
                if(currentInfo.srcs[k] == RS){
                    ip.regs[0] = arguments[k];
                }
                else if(currentInfo.srcs[k] == RT){
                    ip.regs[1] = arguments[k];
                }
                else if(currentInfo.srcs[k] == RD){
                    ip.regs[2] = arguments[k];
                }
                else if(currentInfo.srcs[k] == EXTRA){
                    ip.extra = arguments[k];
                }

            }
            if(!encode(&ip, currentAddress)){
                return EXIT_FAILURE;
            }
            else{
                if(!feof(stdin)){
                if(bigE){
                    int flipped = flipEnd(ip.value);
                    fwrite(&flipped,sizeof(int), 1, stdout);
                }
                else{
                    //printf("%d\n", ip.value);
                    fwrite(&(ip.value),sizeof(int), 1, stdout);
                }
            }
            }
            currentAddress += 4;
            index++;

        }
        //printf("gone through all instructions\n");
        //for(int i = 0; i < index; i++){
            //check through instruction table
            //printf("arrayLoop?\n");


        //}

    }

}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
