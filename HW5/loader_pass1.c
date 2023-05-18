#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// global var
FILE *OBJ_fp;

FILE *OBJ_open(char *fname) {
    OBJ_fp = fopen(fname, "r");
    return (OBJ_fp);
}

void OBJ_close(void) {
    fclose(OBJ_fp);
}

typedef struct
{
    int address;
    int length;
    char name[7];
} RELOC;

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Syntax: load <address> <file 1> < file 2> …\n");
    } else {
        int start_address = 0;
        int program_length = 0;
        for (int i = 3; i < argc; i++) {
            if (OBJ_open(argv[i]) == NULL) {
                printf("file no found");
            } else {
                char line[100];
                fgets(line, sizeof(line), OBJ_fp);
                if (line[0] == 'H') {
                    // get start address
                    char token[6];
                    for(int j = 0; j <= 5; j++) {
                        token[i] = line[j + 7];
                    }
                    start_address = strtol(token, NULL, 16);
                    if (start_address != 0) {
                        printf("this is not a relocatable program\n");
                    } else {
                        if (i == 3) {
                            start_address = strtol(argv[2], NULL, 16);
                        } else {
                            start_address = start_address + program_length;
                        }
                    }
                    // get program length
                    char token[6];
                    for(int j = 0; j <= 5; j++) {
                        token[i] = line[j + 13];
                    }
                    program_length = strtol(token, NULL, 16);

                    //  

                }
            }
        }
        
    }
}