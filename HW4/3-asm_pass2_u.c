#include <string.h>
#include <stdio.h>

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
    unsigned addressing;
} LINE;

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
                            line->addressing = line->addressing | ADDR_INDEX;
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
    // printf("next_loc : %d ", next_loc);
    return next_loc;
}

void print_line(int c, LINE line, int line_count, int line_loc) {
    if (c == LINE_ERROR) {
        printf("%-03d : Error\n", line_count);
    } else if (c == LINE_COMMENT) {
        printf("%-03d : Comment line\n", line_count);
    } else {
        char output_op[LEN_SYMBOL];
        if (line.fmt == FMT4) {
            strcpy(output_op, "+");
            strcat(output_op, line.op);
        } else {
            strcpy(output_op, line.op);
        }
        char output_operand[LEN_SYMBOL];
        if (line.addressing == ADDR_IMMEDIATE)
            strcpy(output_operand, "#");
        else if (line.addressing == ADDR_INDIRECT)
            strcpy(output_operand, "@");
        else
            strcpy(output_operand, "");
        if (line.operand2[0] == '\0') {
            strcat(output_operand, line.operand1);
        } else {
            strcpy(output_operand, line.operand1);
            strcat(output_operand, ",");
            strcat(output_operand, line.operand2);
        }
        printf("%-03d : %06X      %-12s %-12s %-12s \n", line_count, line_loc, line.symbol, output_op, output_operand);
    }
}

int objcode(LINE line) {
    char objcode_bin[32];
    char TA_hex[9];
    char opcode[3];
    char nixbpe_bin[6];
    char disp_bin[20];

    itoa(line.code, opcode, 16);
    opcode[3] = '\0';

    // if (line.fmt == FMT4) {
    //     nixbpe_bin[5] = '1';
    // } else {
    //     nixbpe_bin[0] = '0';
    //     nixbpe_bin[1] = '0';
    //     nixbpe_bin[2] = '0';
    //     nixbpe_bin[3] = '0';
    //     nixbpe_bin[4] = '0';
    //     nixbpe_bin[5] = '0';
    //     nixbpe_bin[6] = '\0';
    // }

    printf("%02s\n", &opcode);

    // switch (line.addressing) {
    // case ADDR_IMMEDIATE:
    //     break;
    // case ADDR_INDEX:
    //     break;
    // case ADDR_INDIRECT:
    //     break;
    // case ADDR_SIMPLE:
    //     break;
    // default:
    //     break;
    // }
    
}

int main(int argc, char *argv[]) {
    int i, c, line_count, start_loc, line_loc, last_line_loc = 0;
    char buf[LEN_SYMBOL];
    LINE line;
    if (argc < 2) {
        printf("Usage: %s fname.asm\n", argv[0]);
    } else {
        if (ASM_open(argv[1]) == NULL)
            printf("File not found!!\n");
        else {
            for (line_count = 1; (c = process_line(&line)) != LINE_EOF; line_count++) {
                if (line_count == 1) {
                    if (line.code == OPTAB[47].code) {
                        start_loc = strtol(line.operand1, NULL, 16);
                    } else {
                        line_loc = 0;
                    }
                    objcode(line);
                    // print_line(c, line, line_count, line_loc);
                    start_loc = line_loc;
                    last_line_loc = line_loc;
                } else if (line.code == OPTAB[13].code) {
                    line_loc = last_line_loc;
                    objcode(line);
                    // print_line(c, line, line_count, line_loc);
                } else if (c != LINE_ERROR && c != LINE_COMMENT) {
                    objcode(line);
                    // print_line(c, line, line_count, line_loc);
                    last_line_loc = line_loc;
                    line_loc += addloc(&line);
                } else {
                    objcode(line);
                    // print_line(c, line, line_count, line_loc);
                }
            }
            ASM_close();
        }
    }
}
