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
    char name[7];
} DEFREC;

void print_tab() {
    printf("Control\t\tSymbol\n");
    printf("section\t\tname\t\tAddress\t\tLength\n");
}

void print_divider() {
    printf("-------------------------------------------------------\n");
}

int header(int order, int last_address) {
    char line[100];
    fgets(line, sizeof(line), OBJ_fp);
    int start_address = 0;
    int program_length = 0;
    char buffer[6];
    char program_name[7] = "";
    if (line[0] == 'H') {
        // get start address
        for(int j = 0; j <= 5; j++) {
            buffer[j] = line[j + 7];
        }
        start_address = strtol(buffer, NULL, 16);
        if (start_address != 0) {
            printf("this is not a relocatable program\n");
        } else {
            start_address = start_address + last_address;
        }
        // get program length
        memset(buffer, 0, 6);
        for(int j = 0; j <= 5; j++) {
            buffer[j] = line[j + 13];
        }
        program_length = strtol(buffer, NULL, 16);

        // get program name
        for(int j = 0; j <= 5; j++) {
            program_name[j] = line[j + 1];
        }
    }
    // printf("%s\n", program_name);
    printf("%s\t\t\t\t%04X\t\t%04X\n", program_name, start_address, program_length);
    start_address += program_length;
    return start_address;
}

void define(int start_address) {
    char line[100];
    char symbol[7] = "";
    fgets(line, sizeof(line), OBJ_fp);
    int def_count = 0;
    if (line[0] == 'D') {
        // get definition record
        def_count = (strlen(line) - 1) / 12;
        DEFREC defrec[def_count];
        for (int j = 0; j < def_count; j++) {
            for (int k = 0; k <= 5; k++) {
                symbol[k] = line[k + 1 + j * 12];
            }
            strcpy(defrec[j].name, symbol);
            char token[7] = "";
            for (int k = 0; k <= 5; k++) {
                token[k] = line[k + 7 + j * 12];
            }
            defrec[j].address = strtol(token, NULL, 16);
            defrec[j].address += start_address;
        }
        // printf("def count: %d\n", def_count);
        for (int j = 0; j < def_count; j++) {
            printf("\t\t%s\t\t%04X\n", defrec[j].name, defrec[j].address);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4 || strcmp(argv[1], "load") != 0) {
        printf("Syntax: load <address> <file 1> <file 2> …\n");
    } else {
        int start_address = strtol(argv[2], NULL, 16);
        if (start_address == 0 && strcmp(argv[2], "0") != 0) {
            printf("invalid address\n");
            return 0;
        }
        int last_address = start_address;
        print_tab();
        print_divider();
        for (int i = 3; i < argc; i++) {
            if (OBJ_open(argv[i]) == NULL) {
                printf("file no found");
            } else {
                last_address = header(i, start_address);
                define(start_address);
                start_address = last_address;
            }
            if (i != argc - 1)
                printf("\n");
        }
        print_divider();
    }
}