#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

static const char* alt_path = "data/listing_0049_conditional_jumps";

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

typedef uint8_t u8;
typedef uint16_t u16;

enum Flag_
{
    Flag_None = 0x00,
    Flag_Zero = 0x01,
    Flag_Sign = 0x02,
};

#define HIGHEST_BIT_8   0x0080
#define HIGHEST_BIT_16  0x8000

struct Registry
{
    union { struct { u8 al, ah; }; u16 ax; };
    union { struct { u8 bl, bh; }; u16 bx; };
    union { struct { u8 cl, ch; }; u16 cx; };
    union { struct { u8 dl, dh; }; u16 dx; };

    u16 sp;
    u16 bp;
    u16 si;
    u16 di;

    u8 flags;

    int ip;
};
typedef struct Registry Registry;
static Registry *global_reg = NULL;

static size_t buffer_size = 24;
static size_t max_buffers = 2;
static char *buffer;

byte_t get_byte(byte_t* instructions)
{
    byte_t result = instructions[global_reg->ip];
    global_reg->ip += 1;
    //byte_t result = fgetc(file);
    // PRINT_BYTE("", result, "\n");
    return result;
}

static Registry create_reg()
{
    Registry reg;

    reg.ax = 0;
    reg.bx = 0;
    reg.cx = 0;
    reg.dx = 0;

    reg.sp = 0;
    reg.bp = 0;
    reg.si = 0;
    reg.di = 0;

    reg.flags = Flag_None;

    reg.ip = 0;

    return reg;
}

static int get_from_reg(Registry *reg, const char *reg_name)
{
    if (strcmp(reg_name, "ax") == 0)    return reg->ax;
    if (strcmp(reg_name, "bx") == 0)    return reg->bx;
    if (strcmp(reg_name, "cx") == 0)    return reg->cx;
    if (strcmp(reg_name, "dx") == 0)    return reg->dx;

    if (strcmp(reg_name, "sp") == 0)    return reg->sp;
    if (strcmp(reg_name, "bp") == 0)    return reg->bp;
    if (strcmp(reg_name, "si") == 0)    return reg->si;
    if (strcmp(reg_name, "di") == 0)    return reg->di;

    if (strcmp(reg_name, "al") == 0)    return reg->al;
    if (strcmp(reg_name, "ah") == 0)    return reg->ah;

    if (strcmp(reg_name, "bl") == 0)    return reg->bl;
    if (strcmp(reg_name, "bh") == 0)    return reg->bh;

    if (strcmp(reg_name, "cl") == 0)    return reg->cl;
    if (strcmp(reg_name, "ch") == 0)    return reg->ch;

    if (strcmp(reg_name, "dl") == 0)    return reg->dl;
    if (strcmp(reg_name, "dh") == 0)    return reg->dh;

    return 0;
}

#define SET_REG(reg_part, value) { int old = reg->reg_part; reg->reg_part = value; printf("%s: %d -> %d\n", #reg_part, old, value); }

static void set_reg(Registry* reg, const char* reg_name, int value)
{
    if (strcmp(reg_name, "ax") == 0)         SET_REG(ax, value)
    else if (strcmp(reg_name, "bx") == 0)    SET_REG(bx, value)
    else if (strcmp(reg_name, "cx") == 0)    SET_REG(cx, value)
    else if (strcmp(reg_name, "dx") == 0)    SET_REG(dx, value)

    else if (strcmp(reg_name, "sp") == 0)    SET_REG(sp, value)
    else if (strcmp(reg_name, "bp") == 0)    SET_REG(bp, value)
    else if (strcmp(reg_name, "si") == 0)    SET_REG(si, value)
    else if (strcmp(reg_name, "di") == 0)    SET_REG(di, value)

    else if (strcmp(reg_name, "al") == 0)    SET_REG(al, value)
    else if (strcmp(reg_name, "ah") == 0)    SET_REG(ah, value)

    else if (strcmp(reg_name, "bl") == 0)    SET_REG(bl, value)
    else if (strcmp(reg_name, "bh") == 0)    SET_REG(bh, value)

    else if (strcmp(reg_name, "cl") == 0)    SET_REG(cl, value)
    else if (strcmp(reg_name, "ch") == 0)    SET_REG(ch, value)

    else if (strcmp(reg_name, "dl") == 0)    SET_REG(dl, value)
    else if (strcmp(reg_name, "dh") == 0)    SET_REG(dh, value)
}

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

static void handle_reg_mem_ops(byte_t byte, byte_t *instructions, char **out_src, char **out_dst, byte_t *reg_to_reg)
{
    byte_t D = byte_span(byte, 1, 2);
    byte_t W = byte_span(byte, 0, 1);

    byte_t next_byte = get_byte(instructions);
    byte_t MOD = byte_span(next_byte, 6, 8);
    byte_t REG = byte_span(next_byte, 3, 6);
    byte_t RM = byte_span(next_byte, 0, 3);

    strcpy(*out_src, reg_field_encoding[W][REG]);

    switch (MOD)
    {
        /* Register Mode, no displacement follows* */
        case 0b11:
        {
            strcpy(*out_dst, reg_field_encoding[W][RM]);

            if (reg_to_reg)
            {
                *reg_to_reg = 1;
            }
        }
        break;

        /* Register Mode, 8-bit displacement follows */
        case 0b01:
        {
            byte_t disp_lo = get_byte(instructions);
            int disp = disp_lo + (disp_lo & 0x80 ? 0xff00 : 0);
            sprintf(*out_dst, "[%s + %d]", effective_address_calculation[RM], disp);
        }
        break;

        /* Register Mode, 16-bit displacement follows */
        case 0b10:
        {
            byte_t disp_lo = get_byte(instructions);
            byte_t disp_hi = get_byte(instructions);
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
                byte_t disp_lo = get_byte(instructions);
                byte_t disp_hi = get_byte(instructions);
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

static int handle_imm_to_reg_mem_ops(byte_t byte, byte_t *instructions, int use_s_bit, char *out_dst, char *out_type, byte_t *out_reg, byte_t *imm_to_reg)
{
    byte_t W = byte_span(byte, 0, 1);
    byte_t S = byte_span(byte, 1, 2);

    byte_t next_byte = get_byte(instructions);
    byte_t MOD = byte_span(next_byte, 6, 8);
    byte_t RM = byte_span(next_byte, 0, 3);

    switch (MOD)
    {
        /* Register Mode, no displacement follows* */
        case 0b11:
        {
            strcpy(out_dst, reg_field_encoding[W][RM]);

            if (imm_to_reg)
            {
                *imm_to_reg = 1;
            }
        }
        break;

        /* Register Mode, 8-bit displacement follows */
        case 0b01:
        {
            byte_t disp = get_byte(instructions);
            sprintf(out_dst, "[%s + %d]", effective_address_calculation[RM], disp);
        }
        break;

        /* Register Mode, 16-bit displacement follows */
        case 0b10:
        {
            byte_t disp_lo = get_byte(instructions);
            byte_t disp_hi = get_byte(instructions);
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
                byte_t disp_lo = get_byte(instructions);
                byte_t disp_hi = get_byte(instructions);
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

    int data = get_byte(instructions);
    if (W)
    {
        if (use_s_bit && S)
        {
            data = data + (data & 0x80 ? 0xff00 : 0);
        }
        else
        {
            byte_t data_w = get_byte(instructions);
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

static void sim_ops_to_reg(int op_id, int src_value, const char* dst)
{
    int dst_value = get_from_reg(global_reg, dst);

    int skip = 0;
    int result;

    switch (op_id)
    {
        case 0b000: // add
        {
            result = dst_value + src_value;
            set_reg(global_reg, dst, result);
        }
        break;

        case 0b101: // sub
        {
            result = dst_value - src_value;
            set_reg(global_reg, dst, result);
        }
        break;

        case 0b111: // cmp
        {
            result = dst_value - src_value;
        }
        break;

        default: skip = 1;
    }

    if (skip == 0)
    {
        global_reg->flags = Flag_None;
        if (result == 0)
        {
            global_reg->flags |= Flag_Zero;
        }
        else if (result & HIGHEST_BIT_16)
        {
            global_reg->flags |= Flag_Sign;
        }
        printf("flags: %d\n", global_reg->flags);
    }
}

int main(int argc, char** argv)
{
    const char* in_path;
    if (argc == 2)
    {
        in_path = argv[1];
    }
    else
    {
        in_path = alt_path;
    }

    FILE* in_file = fopen(in_path, "rb");
    if (!in_file)
    {
        printf("Failed to open %s!", in_path);
        return 1;
    }

    fseek(in_file, 0, SEEK_END);
    int in_length = ftell(in_file);
    fseek(in_file, 0, SEEK_SET);

    byte_t* instructions = (byte_t*)malloc(in_length);
    fread(instructions, in_length, 1, in_file);

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

    Registry reg = create_reg();
    global_reg = &reg;

    while (global_reg->ip < in_length)
    {
        byte_t byte = get_byte(instructions);
        if (feof(in_file)) break;

        /* MOV: Register/memory to/from register */
        if (byte_span(byte, 2, 8) == OPCODE_MOV_REGMEM_TOFROM_REG)
        {
            char *src, *dst;
            get_two_buffers(&src, &dst);

            byte_t reg_to_reg = 0;

            handle_reg_mem_ops(byte, instructions, &src, &dst, &reg_to_reg);

            if (reg_to_reg)
            {
                int data = get_from_reg(global_reg, src);
                set_reg(global_reg, dst, data);
            }

            fprintf(out_file, "mov %s, %s\n", dst, src);
        }
        /* MOV: Immediate to register/memory */
        else if (byte_span(byte, 1, 8) == OPCODE_MOV_IMM_TO_REGMEM)
        {
            char *dst, *type;
            get_two_buffers(&dst, &type);

            byte_t imm_to_reg = 0;

            int data = handle_imm_to_reg_mem_ops(byte, instructions, 0, dst, type, NULL, &imm_to_reg);

            if (imm_to_reg)
            {
                set_reg(global_reg, dst, data);
            }

            fprintf(out_file, "mov %s, %s%d\n", dst, type, data);
        }
        /* MOV: Immediate to register */
        else if (byte_span(byte, 4, 8) == OPCODE_MOV_IMM_TO_REG)
        {
            byte_t W = byte_span(byte, 3, 4);
            byte_t REG = byte_span(byte, 0, 3);

            int data = get_byte(instructions);
            if (W)
            {
                byte_t data_w = get_byte(instructions);
                data = data + (data_w << 8);
            }

            set_reg(global_reg, reg_field_encoding[W][REG], data);

            fprintf(out_file, "mov %s, %d\n", reg_field_encoding[W][REG], data);
        }
        /* MOV: Register to accmulator */
        else if (byte_span(byte, 1, 8) == OPCODE_MOV_MEM_TO_ACC)
        {
            byte_t W = byte_span(byte, 0, 1);
            byte_t addr_lo = get_byte(instructions);
            byte_t addr_hi = get_byte(instructions);
            int addr = addr_lo + (addr_hi << 8);

            fprintf(out_file, "mov %s, [%d]\n", W ? "ax" : "al", addr);
        }
        /* MOV: Accumulator to memory */
        else if (byte_span(byte, 1, 8) == OPCODE_MOV_ACC_TO_MEM)
        {
            byte_t W = byte_span(byte, 0, 1);
            byte_t addr_lo = get_byte(instructions);
            byte_t addr_hi = get_byte(instructions);
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

                int data = get_byte(instructions);
                if (W)
                {
                    byte_t data_w = get_byte(instructions);
                    data = data + (data_w << 8);
                }

                fprintf(out_file, "%s %s, %d\n", op_from_identifier[op_id], W ? "ax" : "al", data);
            }
            /* OP: Reg/memory with register to either */
            else
            {
                char* src, * dst;
                get_two_buffers(&src, &dst);

                byte_t reg_to_reg = 0;

                handle_reg_mem_ops(byte, instructions, &src, &dst, &reg_to_reg);

                if (reg_to_reg)
                {
                    int src_value = get_from_reg(global_reg, src);
                    sim_ops_to_reg(op_id, src_value, dst);
                }

                fprintf(out_file, "%s %s, %s\n", op_from_identifier[op_id], dst, src);
            }
        }
        /* OP: Immediate to register/memory */
        else if (byte_span(byte, 2, 8) == OPCODE_OP_IMM_TO_REGMEM)
        {
            char* dst, * type;
            byte_t* reg;
            get_three_buffers(&dst, &type, &reg);

            byte_t imm_to_reg = 0;

            int data = handle_imm_to_reg_mem_ops(byte, instructions, 1, dst, type, reg, &imm_to_reg);

            if (imm_to_reg)
            {
                sim_ops_to_reg(*reg, data, dst);
            }

            fprintf(out_file, "%s %s, %s%d\n", op_from_identifier[*reg], dst, type, data);
        }
        /* Conditional jumps */
        else if (byte_span(byte, 4, 8) == OPCODE_CONDITIONAL_JUMPS)
        {
            byte_t jump_type = byte_span(byte, 0, 4);
            s8 ip_inc8 = get_byte(instructions) + 2;

            switch (jump_type)
            {
                case 0b0101: // jne
                {
                    if ((global_reg->flags & Flag_Zero) == 0)
                    {
                        global_reg->ip += ip_inc8 - 2;
                    }
                }
                break;
            }

            fprintf(out_file, "%s $%s%d\n", jump_from_identifier[jump_type], ip_inc8 < 0 ? "" : "+", ip_inc8);
        }
        /* Loops */
        else if (byte_span(byte, 2, 8) == OPCODE_LOOPS)
        {
            byte_t loop_type = byte_span(byte, 0, 2);
            s8 ip_inc8 = get_byte(instructions) + 2;

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
