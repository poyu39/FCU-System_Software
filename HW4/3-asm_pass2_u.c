#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "2-optable.c"

/* Public variables and functions */
#define ADDR_SIMPLE 0x01
#define ADDR_IMMEDIATE 0x02
#define ADDR_INDIRECT 0x04
#define ADDR_INDEX 0x08

#define LINE_EOF (-1)
#define LINE_COMMENT (-2)
#define LINE_ERROR (0)
#define LINE_CORRECT (1)

typedef struct
{
    char symbol[LEN_SYMBOL];
    char op[LEN_SYMBOL];
    char operand1[LEN_SYMBOL];
    char operand2[LEN_SYMBOL];
    unsigned code;
    unsigned fmt;
    unsigned loc;
    unsigned addressing;
} LINE;

typedef struct 
{
    int addr;
    unsigned useBASE;
} DISP;


LINE line_arr[100];
LINE m_arr[100];
int m_count = 0;
int BASE = 0;
int line_len = 0;

int process_line(LINE *line);
/* return LINE_EOF, LINE_COMMENT, LINE_ERROR, LINE_CORRECT and Instruction information in *line*/

/* Private variable and function */

void init_LINE(LINE *line) {
    line->symbol[0] = '\0';
    line->op[0] = '\0';
    line->operand1[0] = '\0';
    line->operand2[0] = '\0';
    line->code = 0x0;
    line->fmt = 0x0;
    line->loc = 0x0;
    line->addressing = ADDR_SIMPLE;
}

int process_line(LINE *line)
/* return LINE_EOF, LINE_COMMENT, LINE_ERROR, LINE_CORRECT */
{
    char buf[LEN_SYMBOL];
    int c;
    int state;
    int ret;
    Instruction *op;

    c = ASM_token(buf); /* get the first token of a line */
    if (c == EOF)
        return LINE_EOF;
    else if ((c == 1) && (buf[0] == '\n')) /* blank line */
        return LINE_COMMENT;
    else if ((c == 1) && (buf[0] == '.')) /* a comment line */
    {
        do {
            c = ASM_token(buf);
        } while ((c != EOF) && (buf[0] != '\n'));
        return LINE_COMMENT;
    } else {
        init_LINE(line);
        ret = LINE_ERROR;
        state = 0;
        while (state < 8) {
            switch (state) {
            case 0:
            case 1:
            case 2:
                op = is_opcode(buf);
                if ((state < 2) && (buf[0] == '+')) /* + */
                {
                    line->fmt = FMT4;
                    state = 2;
                } else if (op != NULL) /* INSTRUCTION */
                {
                    strcpy(line->op, op->op);
                    line->code = op->code;
                    state = 3;
                    if (line->fmt != FMT4) {
                        line->fmt = op->fmt & (FMT1 | FMT2 | FMT3);
                    } else if ((line->fmt == FMT4) && ((op->fmt & FMT4) == 0)) /* INSTRUCTION is FMT1 or FMT 2*/
                    {                                                          /* ERROR 20210326 added */
                        printf("ERROR at token %s, %s cannot use format 4 \n", buf, buf);
                        ret = LINE_ERROR;
                        state = 7; /* skip following tokens in the line */
                    }
                } else if (state == 0) /* SYMBOL */
                {
                    strcpy(line->symbol, buf);
                    state = 1;
                } else /* ERROR */
                {
                    printf("ERROR at token %s\n", buf);
                    ret = LINE_ERROR;
                    state = 7; /* skip following tokens in the line */
                }
                break;
            case 3:
                if (line->fmt == FMT1 || line->code == 0x4C) /* no operand needed */
                {
                    if (c == EOF || buf[0] == '\n') {
                        ret = LINE_CORRECT;
                        state = 8;
                    } else /* COMMENT */
                    {
                        ret = LINE_CORRECT;
                        state = 7;
                    }
                } else {
                    if (c == EOF || buf[0] == '\n') {
                        ret = LINE_ERROR;
                        state = 8;
                    } else if (buf[0] == '@' || buf[0] == '#') {
                        line->addressing = (buf[0] == '#') ? ADDR_IMMEDIATE : ADDR_INDIRECT;
                        state = 4;
                    } else /* get a symbol */
                    {
                        op = is_opcode(buf);
                        if (op != NULL) {
                            printf("Operand1 cannot be a reserved word\n");
                            ret = LINE_ERROR;
                            state = 7; /* skip following tokens in the line */
                        } else {
                            strcpy(line->operand1, buf);
                            state = 5;
                        }
                    }
                }
                break;
            case 4:
                op = is_opcode(buf);
                if (op != NULL) {
                    printf("Operand1 cannot be a reserved word\n");
                    ret = LINE_ERROR;
                    state = 7; /* skip following tokens in the line */
                } else {
                    strcpy(line->operand1, buf);
                    state = 5;
                }
                break;
            case 5:
                if (c == EOF || buf[0] == '\n') {
                    ret = LINE_CORRECT;
                    state = 8;
                } else if (buf[0] == ',') {
                    state = 6;
                } else /* COMMENT */
                {
                    ret = LINE_CORRECT;
                    state = 7; /* skip following tokens in the line */
                }
                break;
            case 6:
                if (c == EOF || buf[0] == '\n') {
                    ret = LINE_ERROR;
                    state = 8;
                } else /* get a symbol */
                {
                    op = is_opcode(buf);
                    if (op != NULL) {
                        printf("Operand2 cannot be a reserved word\n");
                        ret = LINE_ERROR;
                        state = 7; /* skip following tokens in the line */
                    } else {
                        if (line->fmt == FMT2) {
                            strcpy(line->operand2, buf);
                            ret = LINE_CORRECT;
                            state = 7;
                        } else if ((c == 1) && (buf[0] == 'x' || buf[0] == 'X')) {
                            // line->addressing = line->addressing | ADDR_INDEX;
                            line->addressing = ADDR_INDEX;
                            ret = LINE_CORRECT;
                            state = 7; /* skip following tokens in the line */
                        } else {
                            printf("Operand2 exists only if format 2  is used\n");
                            ret = LINE_ERROR;
                            state = 7; /* skip following tokens in the line */
                        }
                    }
                }
                break;
            case 7: /* skip tokens until '\n' || EOF */
                if (c == EOF || buf[0] == '\n')
                    state = 8;
                break;
            }
            if (state < 8)
                c = ASM_token(buf); /* get the next token */
        }
        return ret;
    }
}

int addloc(LINE *line) {
    int next_loc = 0;
    char buf_op[LEN_SYMBOL];
    if (line->fmt == FMT4) {
        next_loc = 4;
    } else if (line->code == OPTAB[5].code) {
        if (line->operand1[0] == 'C') {
            next_loc = strlen(line->operand1) - 3;
        } else {
            next_loc = (strlen(line->operand1) - 3) / 2;
        }
    } else if (line->code == OPTAB[38].code) {
        next_loc = atoi(line->operand1);
    } else if (line->code == OPTAB[39].code) {
        next_loc = atoi(line->operand1) * 3;
    } else if (line->code == OPTAB[4].code) {
        next_loc = 0;
    } else {
        if (line->fmt == FMT2) {
            next_loc = 2;
        } else {
            next_loc = 3;
        }
    }
    return next_loc;
}

int find_symtab(LINE input_line, int line_count) {
    for (int i = 0; i < 100; i++) {
        if (strcmp(input_line.operand1, line_arr[i].symbol) == 0 && line_count != i) {
            return line_arr[i].loc;
        }
    }
    return -1;
}

unsigned is_symbol(int line_count) {
    for (int i = 0; i < line_len; i++) {
        if (strcmp(line_arr[line_count].operand1, line_arr[i].symbol) == 0 && line_count != i) {
            return 1;
        }
    }
    return 0;
}

DISP find_disp(LINE line, int line_count) {
    DISP disp;
    disp.useBASE = 0;
    disp.addr = 0;
    int symtab_loc = find_symtab(line, line_count);
    if (line.fmt == FMT4) {
        if (symtab_loc == -1) {
            disp.addr = atoi(line.operand1);
        } else {
            disp.addr = symtab_loc;
            m_arr[m_count++] = line;
        }
    } else {
        if (line.addressing == ADDR_SIMPLE) {
            if (symtab_loc == -1) {
                disp.addr = 0;
            }
        }
        if (line.addressing == ADDR_IMMEDIATE) {
            if (symtab_loc == -1) {
                disp.addr = atoi(line.operand1);
            }
        }
        if (line.addressing == ADDR_INDIRECT) {
            if (symtab_loc == -1) {
                disp.addr = 0;
            }
        }
        if (symtab_loc != -1) {
            disp.addr = symtab_loc - line_arr[line_count + 1].loc;
            if (disp.addr < -2048 || disp.addr > 2047) {
                disp.addr = symtab_loc - BASE;
                disp.useBASE = TRUE;
            }
        }

    }
    if (strcmp(line.op, "LDB") == 0) {
        BASE = symtab_loc;
    }
    return disp;
}

void objcode(LINE line, int line_count) {
    unsigned opni_hex;
    unsigned xbpe_hex;
    char opni_char[2];
    char xbpe_char[2];
    char disp_char[2];

    switch (line.fmt) {
    case FMT0:
        if (strcmp(line.op, "BYTE") == 0) {
            if (line.operand1[0] == 'C') {
                for (int i = 2; i < strlen(line.operand1) - 1; i++) {
                    printf("%02X", line.operand1[i]);
                }
            } else if (line.operand1[0] == 'X') {
                for (int i = 2; i < strlen(line.operand1) - 1; i++) {
                    printf("%c", line.operand1[i]);
                }
            }
        } else if (strcmp(line.op, "WORD") == 0) {
            printf("%06X", atoi(line.operand1));
        } else if (strcmp(line.op, "RSUB") == 0) {
            printf("4F0000");
        }
        break;
    case FMT2:
        if (strcmp(line.op, "CLEAR") == 0) {
            printf("B4");
        } else if (strcmp(line.op, "TIXR") == 0) {
            printf("B8");
        } else if (strcmp(line.op, "COMPR") == 0) {
            printf("A0");
        }
        if (strcmp(line.operand1, "A") == 0) {
            printf("0");
        } else if (strcmp(line.operand1, "X") == 0) {
            printf("1");
        } else if (strcmp(line.operand1, "L") == 0) {
            printf("2");
        } else if (strcmp(line.operand1, "B") == 0) {
            printf("3");
        } else if (strcmp(line.operand1, "S") == 0) {
            printf("4");
        } else if (strcmp(line.operand1, "T") == 0) {
            printf("5");
        } else if (strcmp(line.operand1, "F") == 0) {
            printf("6");
        } else {
            printf("0");
        }

        if (strcmp(line.operand2, "A") == 0) {
            printf("0");
        } else if (strcmp(line.operand2, "X") == 0) {
            printf("1");
        } else if (strcmp(line.operand2, "L") == 0) {
            printf("2");
        } else if (strcmp(line.operand2, "B") == 0) {
            printf("3");
        } else if (strcmp(line.operand2, "S") == 0) {
            printf("4");
        } else if (strcmp(line.operand2, "T") == 0) {
            printf("5");
        } else if (strcmp(line.operand2, "F") == 0) {
            printf("6");
        } else {
            printf("0");
        }
        break;
    case FMT3:
    case FMT4:
        if (strcmp(line.op, "RSUB") == 0) {
            printf("4F0000");
            break;
        }
        unsigned usePC = TRUE;
        opni_hex = line.code;
        xbpe_hex = 2;

        if (line.addressing == ADDR_INDIRECT)
            opni_hex += 2;
        if (line.addressing == ADDR_INDEX)
            opni_hex += 3;
        if (line.addressing == ADDR_SIMPLE)
            opni_hex += 3;
        if (line.addressing == ADDR_IMMEDIATE) {
            opni_hex += 1;
            if (is_symbol(line_count) == 0) {
                usePC = FALSE;
            }
        }
        
        /*
                bin     hex
            x   1000    8
            b   0100    4
            p   0010    2
            e   0001    1
        */
        
        // addr
        if (line.addressing == ADDR_INDEX) {
            xbpe_hex += 8;
        }
            

        // fmt 
        if (line.fmt == FMT4) {
            xbpe_hex += 1;
            usePC = FALSE;
        }
        
        DISP disp = find_disp(line, line_count);
        if (disp.useBASE) {
            xbpe_hex += 4;
            usePC = FALSE;
        }
        if (!usePC) {
            xbpe_hex -= 2;
        }
        if (disp.addr >= -2048 && disp.addr < 0) {
            disp.addr += 4096;
        }
        if (line.fmt == FMT4) {
            printf("%02X%1X%05X", opni_hex, xbpe_hex, disp.addr);
        } else if (line.fmt == FMT3) {
            printf("%02X%1X%03X", opni_hex, xbpe_hex, disp.addr);
        }

        break;
    default:
        break;
    }

    // printf(" ");
}

void header(LINE line, int start_loc, int program_len) {
    printf("H%-6s%06X%06X\n", line_arr[1].symbol, start_loc, program_len);
}

int find_nextline(int line_count) {
    int temp_RE = FALSE;
    int texter_len = 0;
    for (int i = line_count; texter_len < 29; i++) {
        if (strcmp(line_arr[i].op, "BYTE") == 0) {
            if (line_arr[i].operand1[0] == 'C') {
                texter_len += strlen(line_arr[i].operand1) - 3;
            } else if (line_arr[i].operand1[0] == 'X') {
                texter_len += (strlen(line_arr[i].operand1) - 3) / 2;
            }
        }
        if (line_arr[i].fmt == FMT2) 
            texter_len += 2;
        else if (line_arr[i].fmt == FMT3)
            texter_len += 3;
        else if (line_arr[i].fmt == FMT4)
            texter_len += 4;
        if (strcmp(line_arr[i].op, "RESB") == 0 || strcmp(line_arr[i].op, "RESW") == 0) {
            return texter_len;
        }
        if (i > line_len) {
            return texter_len;
        }
    }
    return texter_len;
}

int main(int argc, char *argv[]) {
    // pass 1
    int i, c, line_count, line_loc, last_line_loc = 0;
    int start_loc = 0;
    int program_len = 0;
    char buf[LEN_SYMBOL];
    
    line_loc = 0;
    LINE line;

    if (argc < 2) {
        printf("Usage: %s fname.asm\n", argv[0]);
    } else {
        if (ASM_open(argv[1]) == NULL)
            printf("File not found!!\n");
        else {
            for (line_count = 1; (c = process_line(&line)) != LINE_EOF; line_count++) {
                if (line_count == 1) {
                    if (strcmp(line.op, "START") == 0) {
                        start_loc = strtol(line.operand1, NULL, 16);
                    } else {
                        line_loc = 0;
                    }
                    start_loc = line_loc;
                    last_line_loc = line_loc;
                    line.loc = line_loc;
                    line_arr[line_count] = line;
                } else if (strcmp(line.op, "END") == 0) {
                    line_loc = last_line_loc;
                    line.loc = line_loc;
                    line_arr[line_count] = line;
                    line_loc += 1;
                } else if (c != LINE_ERROR && c != LINE_COMMENT) {
                    last_line_loc = line_loc;
                    line.loc = line_loc;
                    line_arr[line_count] = line;
                    line_loc += addloc(&line);
                }
            }
            program_len = line_loc - start_loc;
            ASM_close();
        }
    }
    line_len = line_count;

    // pass 2
    header(line_arr[1], start_loc, program_len);
    int texter_len = 0;
    int text_count = 0;
    for (int i = 1; i < line_count; i++) {
        if (line_arr[i].fmt == FMT0 && strcmp(line_arr[i].op, "BYTE") != 0) {
            continue;
        }
        if (texter_len == 0) {
            texter_len = find_nextline(i);
            printf("T%06X ", line_arr[i].loc);
            printf("%02X ", texter_len);
        }

        objcode(line_arr[i], i);

        if (strcmp(line_arr[i].op, "BYTE") == 0) {
            if (line_arr[i].operand1[0] == 'C') {
                text_count += strlen(line_arr[i].operand1) - 3;
            } else if (line_arr[i].operand1[0] == 'X') {
                text_count += (strlen(line_arr[i].operand1) - 3) / 2;
            }
        }
        if (line_arr[i].fmt == FMT2) 
            text_count += 2;
        else if (line_arr[i].fmt == FMT3)
            text_count += 3;
        else if (line_arr[i].fmt == FMT4)
            text_count += 4;

        if (text_count >= texter_len) {
            printf("\n");
            text_count = 0;
            texter_len = 0;
        }
    }
    
    for (int i = 0; i < m_count; i++) {
        printf("M%06X05\n", m_arr[i].loc + 1);
    }

    printf("E%06X\n", start_loc);
}
