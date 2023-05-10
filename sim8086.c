#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define SWAP_REG(x, y) do { char* t = x; x = y; y = t; } while (0)

#define BYTE_TO_BINARY(byte) \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')
#define PRINT_BYTE(prefix, byte, suffix) printf(prefix "%c%c%c%c%c%c%c%c" suffix, BYTE_TO_BINARY(byte))
// #define PRINT_BYTE

typedef unsigned char byte_t;
typedef char s8;

byte_t byte_span(byte_t byte, int begin, int end)
{
    return (byte - ((byte >> end) << end)) >> begin;
}

byte_t get_byte(FILE *file)
{
    byte_t result = fgetc(file);
    // PRINT_BYTE("", result, "\n");
    return result;
}

#define OPCODE_MOV_REGMEM_TOFROM_REG            0b100010
#define OPCODE_MOV_IMM_TO_REGMEM                0b1100011
#define OPCODE_MOV_IMM_TO_REG                   0b1011
#define OPCODE_MOV_MEM_TO_ACC                   0b1010000
#define OPCODE_MOV_ACC_TO_MEM                   0b1010001

#define OPCODE_OP_REGMEM_WITH_REG_TO_EITHER     0b00
#define OPCODE_OP_IMM_TO_REGMEM                 0b100000
#define OPCODE_OP_IMM_TO_ACC                    0b10
#define OPCODE_ADD                              0b000
#define OPCODE_SUB                              0b101
#define OPCODE_CMP                              0b111

#define OPCODE_CONDITIONAL_JUMPS                0b0111
#define OPCODE_JE_JZ                            0b0100
#define OPCODE_JL_JNGE                          0b1100
#define OPCODE_JLE_JNG                          0b1110
#define OPCODE_JB_JNAE                          0b0010
#define OPCODE_JBE_JNA                          0b0110
#define OPCODE_JP_JPE                           0b1010
#define OPCODE_JO                               0b0000
#define OPCODE_JS                               0b1000
#define OPCODE_JNE_JNZ                          0b0101
#define OPCODE_JNL_JGE                          0b1101
#define OPCODE_JNLE_JG                          0b1111
#define OPCODE_JNB_JAE                          0b0011
#define OPCODE_JNBE_JA                          0b0111
#define OPCODE_JNP_JPO                          0b1011
#define OPCODE_JNO                              0b0001
#define OPCODE_JNS                              0b1001

#define OPCODE_LOOPS                            0b111000

const char *reg_field_encoding[2][8] = {
    /* W = 0 */ {
        /* 000 = */ "al",
        /* 001 = */ "cl",
        /* 010 = */ "dl",
        /* 011 = */ "bl",
        /* 100 = */ "ah",
        /* 101 = */ "ch",
        /* 110 = */ "dh",
        /* 111 = */ "bh"
    },
    /* W = 1 */ {
        /* 000 = */ "ax",
        /* 001 = */ "cx",
        /* 010 = */ "dx",
        /* 011 = */ "bx",
        /* 100 = */ "sp",
        /* 101 = */ "bp",
        /* 110 = */ "si",
        /* 111 = */ "di"
    }
};

const char *effective_address_calculation[8] = {
    /* 000 = */ "bx + si",
    /* 001 = */ "bx + di",
    /* 010 = */ "bp + si",
    /* 011 = */ "bp + di",
    /* 100 = */ "si",
    /* 101 = */ "di",
    /* 110 = */ "bp",
    /* 111 = */ "bx"
};

const char *op_from_identifier[8] = {
    /* 000 = */ "add",
    /* 001 = */ "???",
    /* 010 = */ "???",
    /* 011 = */ "???",
    /* 100 = */ "???",
    /* 101 = */ "sub",
    /* 110 = */ "???",
    /* 111 = */ "cmp"
};

const char *jump_from_identifier[16] = {
    /* 0000 = */ "jo",
    /* 0001 = */ "jno",
    /* 0010 = */ "jb",
    /* 0011 = */ "jnb",
    /* 0100 = */ "je",
    /* 0101 = */ "jne",
    /* 0110 = */ "jbe",
    /* 0111 = */ "jnbe",
    /* 1000 = */ "js",
    /* 1001 = */ "jns",
    /* 1010 = */ "jp",
    /* 1011 = */ "jnp",
    /* 1100 = */ "jl",
    /* 1101 = */ "jnl",
    /* 1110 = */ "jle",
    /* 1111 = */ "jnle"
};

const char *loop_from_idnetifier[4] = {
    /* 00 = */ "loopnz",
    /* 01 = */ "loopz",
    /* 10 = */ "loop",
    /* 11 = */ "jcxz"
};

static size_t buffer_size = 24;
static size_t max_buffers = 2;
static char *buffer;

static void get_buffer(char **out_buffer)
{
    *out_buffer = buffer;
    memset(*out_buffer, 0, buffer_size);
}

static void get_two_buffers(char **out_buffer_1, char **out_buffer_2)
{
    *out_buffer_1 = buffer;
    memset(*out_buffer_1, 0, buffer_size);

    *out_buffer_2 = buffer + buffer_size;
    memset(*out_buffer_2, 0, buffer_size);
}

static void get_three_buffers(char** out_buffer_1, char** out_buffer_2, char** out_buffer_3)
{
    *out_buffer_1 = buffer;
    memset(*out_buffer_1, 0, buffer_size);

    *out_buffer_2 = buffer + buffer_size;
    memset(*out_buffer_2, 0, buffer_size);

    *out_buffer_3 = buffer + buffer_size + buffer_size;
    memset(*out_buffer_3, 0, buffer_size);
}

static void handle_reg_mem_ops(byte_t byte, FILE *file, char **out_src, char **out_dst)
{
    byte_t D = byte_span(byte, 1, 2);
    byte_t W = byte_span(byte, 0, 1);

    byte_t next_byte = get_byte(file);
    byte_t MOD = byte_span(next_byte, 6, 8);
    byte_t REG = byte_span(next_byte, 3, 6);
    byte_t RM = byte_span(next_byte, 0, 3);

    strcpy(*out_src, reg_field_encoding[W][REG]);

    switch (MOD)
    {
        /* Memory Mode, no displacement follows* */
        case 0b11:
        {
            strcpy(*out_dst, reg_field_encoding[W][RM]);
        }
        break;

        /* Memory Mode, 8-bit displacement follows */
        case 0b01:
        {
            byte_t disp_lo = get_byte(file);
            int disp = disp_lo + (disp_lo & 0x80 ? 0xff00 : 0);
            sprintf(*out_dst, "[%s + %d]", effective_address_calculation[RM], disp);
        }
        break;

        /* Memory Mode, 16-bit displacement follows */
        case 0b10:
        {
            byte_t disp_lo = get_byte(file);
            byte_t disp_hi = get_byte(file);
            int disp = disp_lo + (disp_hi << 8);
            sprintf(*out_dst, "[%s + %d]", effective_address_calculation[RM], disp);
        }
        break;

        /* Register Mode (no displacement) */
        case 0b00:
        {
            /* Direct Address */
            if (RM == 0b110)
            {
                byte_t disp_lo = get_byte(file);
                byte_t disp_hi = get_byte(file);
                int direct_address = disp_lo + (disp_hi << 8);

                sprintf(*out_dst, "[%d]", direct_address);
            }
            else
            {
                sprintf(*out_dst, "[%s]", effective_address_calculation[RM]);
            }
        }
        break;
    }

    if (D)
    {
        SWAP_REG(*out_src, *out_dst);
    }
}

static int handle_imm_to_reg_mem_ops(byte_t byte, FILE *file, int use_s_bit, char *out_dst, char *out_type, byte_t *out_reg)
{
    byte_t W = byte_span(byte, 0, 1);
    byte_t S = byte_span(byte, 1, 2);

    byte_t next_byte = get_byte(file);
    byte_t MOD = byte_span(next_byte, 6, 8);
    byte_t RM = byte_span(next_byte, 0, 3);

    switch (MOD)
    {
        /* Memory Mode, no displacement follows* */
        case 0b11:
        {
            strcpy(out_dst, reg_field_encoding[W][RM]);
        }
        break;

        /* Memory Mode, 8-bit displacement follows */
        case 0b01:
        {
            byte_t disp = get_byte(file);
            sprintf(out_dst, "[%s + %d]", effective_address_calculation[RM], disp);
        }
        break;

        /* Memory Mode, 16-bit displacement follows */
        case 0b10:
        {
            byte_t disp_lo = get_byte(file);
            byte_t disp_hi = get_byte(file);
            int disp = disp_lo + (disp_hi << 8);
            sprintf(out_dst, "[%s + %d]", effective_address_calculation[RM], disp);
        }
        break;

        /* Register Mode (no displacement) */
        case 0b00:
        {
            /* Direct Address */
            if (RM == 0b110)
            {
                byte_t disp_lo = get_byte(file);
                byte_t disp_hi = get_byte(file);
                int direct_address = disp_lo + (disp_hi << 8);

                sprintf(out_dst, "[%d]", direct_address);
            }
            else
            {
                sprintf(out_dst, "[%s]", effective_address_calculation[RM]);
            }
        }
        break;
    }

    int data = get_byte(file);
    if (W)
    {
        if (use_s_bit && S)
        {
            data = data + (data & 0x80 ? 0xff00 : 0);
        }
        else
        {
            byte_t data_w = get_byte(file);
            data = data + (data_w << 8);
        }

        strcpy(out_type, "word ");
    }
    else
    {
        strcpy(out_type, "byte ");
    }

    if (out_reg)
    {
        *out_reg = byte_span(next_byte, 3, 6);
    }

    return data;
}

int main(int argc, char** argv)
{
    char* in_path;
    if (argc == 2)
    {
        in_path = argv[1];
    }
    else
    {
        in_path = "data/listing_0041_add_sub_cmp_jnz";
    }

    FILE *in_file = fopen(in_path, "rb");
    if (!in_file)
    {
        printf("Failed to open %s!", in_path);
        return 1;
    }

    char *out_path = (char*)malloc(strlen(in_path) + 4);
    sprintf(out_path, "out/%s.asm", strrchr(in_path, '/') + 1);

    FILE *out_file = fopen(out_path, "w");
    if (!out_file)
    {
        printf("Failed to open %s!", out_path);
        return 1;
    }
    free(out_path);

    fprintf(out_file, "; %s dissasembly\n", strrchr(in_path, '/') + 1);
    fprintf(out_file, "bits 16\n");

    buffer = (char*)malloc(buffer_size * max_buffers);

    for (;;)
    {
        byte_t byte = get_byte(in_file);
        if (feof(in_file)) break;

        /* MOV: Register/memory to/from register */
        if (byte_span(byte, 2, 8) == OPCODE_MOV_REGMEM_TOFROM_REG)
        {
            char *src, *dst;
            get_two_buffers(&src, &dst);

            handle_reg_mem_ops(byte, in_file, &src, &dst);

            fprintf(out_file, "mov %s, %s\n", dst, src);
        }
        /* MOV: Immediate to register/memory */
        else if (byte_span(byte, 1, 8) == OPCODE_MOV_IMM_TO_REGMEM)
        {
            char *dst, *type;
            get_two_buffers(&dst, &type);

            int data = handle_imm_to_reg_mem_ops(byte, in_file, 0, dst, type, NULL);

            fprintf(out_file, "mov %s, %s%d\n", dst, type, data);
        }
        /* MOV: Immediate to register */
        else if (byte_span(byte, 4, 8) == OPCODE_MOV_IMM_TO_REG)
        {
            byte_t W = byte_span(byte, 3, 4);
            byte_t REG = byte_span(byte, 0, 3);

            int data = get_byte(in_file);
            if (W)
            {
                byte_t data_w = get_byte(in_file);
                data = data + (data_w << 8);
            }

            fprintf(out_file, "mov %s, %d\n", reg_field_encoding[W][REG], data);
        }
        /* MOV: Memory to accmulator */
        else if (byte_span(byte, 1, 8) == OPCODE_MOV_MEM_TO_ACC)
        {
            byte_t W = byte_span(byte, 0, 1);
            byte_t addr_lo = get_byte(in_file);
            byte_t addr_hi = get_byte(in_file);
            int addr = addr_lo + (addr_hi << 8);

            fprintf(out_file, "mov %s, [%d]\n", W ? "ax" : "al", addr);
        }
        /* MOV: Accumulator to memory */
        else if (byte_span(byte, 1, 8) == OPCODE_MOV_ACC_TO_MEM)
        {
            byte_t W = byte_span(byte, 0, 1);
            byte_t addr_lo = get_byte(in_file);
            byte_t addr_hi = get_byte(in_file);
            int addr = addr_lo + (addr_hi << 8);

            fprintf(out_file, "mov [%d], %s\n", addr, W ? "ax" : "al");
        }
        else if (byte_span(byte, 6, 8) == OPCODE_OP_REGMEM_WITH_REG_TO_EITHER)
        {
            byte_t op_id = byte_span(byte, 3, 6);

            /* OP: Immediate to accumulator */
            if (byte_span(byte, 1, 3) == OPCODE_OP_IMM_TO_ACC)
            {
                byte_t W = byte_span(byte, 0, 1);

                int data = get_byte(in_file);
                if (W)
                {
                    byte_t data_w = get_byte(in_file);
                    data = data + (data_w << 8);
                }

                fprintf(out_file, "%s %s, %d\n", op_from_identifier[op_id], W ? "ax" : "al", data);
            }
            /* OP: Reg/memory with register to either */
            else
            {
                char *src, *dst;
                get_two_buffers(&src, &dst);

                handle_reg_mem_ops(byte, in_file, &src, &dst);

                fprintf(out_file, "%s %s, %s\n", op_from_identifier[op_id], dst, src);
            }
        }
        /* OP: Immediate to register/memory */
        else if (byte_span(byte, 2, 8) == OPCODE_OP_IMM_TO_REGMEM)
        {
            char *dst, *type;
            byte_t *reg;
            get_three_buffers(&dst, &type, &reg);

            int data = handle_imm_to_reg_mem_ops(byte, in_file, 1, dst, type, reg);

            fprintf(out_file, "%s %s, %s%d\n", op_from_identifier[*reg], dst, type, data);
        }
        /* Conditional jumps */
        else if (byte_span(byte, 4, 8) == OPCODE_CONDITIONAL_JUMPS)
        {
            byte_t jump_type = byte_span(byte, 0, 4);
            s8 ip_inc8 = get_byte(in_file) + 2;

            fprintf(out_file, "%s $%s%d\n", jump_from_identifier[jump_type], ip_inc8 < 0 ? "" : "+", ip_inc8);
        }
        /* Loops */
        else if (byte_span(byte, 2, 8) == OPCODE_LOOPS)
        {
            byte_t loop_type = byte_span(byte, 0, 2);
            s8 ip_inc8 = get_byte(in_file) + 2;

            fprintf(out_file, "%s $%s%d\n", loop_from_idnetifier[loop_type], ip_inc8 < 0 ? "" : "+", ip_inc8);
        }
        else
        {
            // assert(0 && "Unsupported operation! Sorry :(");
            PRINT_BYTE("OP: ", byte, ". ");
            printf("Unsupported operation! Sorry :(\n");
        }
    }

    free(buffer);

    fclose(out_file);
    fclose(in_file);
    return 0;
}
