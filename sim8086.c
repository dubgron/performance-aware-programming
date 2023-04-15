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
// #define PRINT_BYTE(prefix, byte, suffix) printf(prefix "%c%c%c%c%c%c%c%c" suffix, BYTE_TO_BINARY(byte))
#define PRINT_BYTE

typedef unsigned char byte_t;

byte_t byte_span(byte_t byte, int begin, int end)
{
    return (byte - ((byte >> end) << end)) >> begin;
}

byte_t get_byte(FILE *file)
{
    byte_t result = fgetc(file);
    PRINT_BYTE("", result, " ");
    return result;
}

#define OPCODE_MOV_REGMEM_TOFROM_REG 0b100010
#define OPCODE_MOV_IM_TO_REGMEM 0b1100011
#define OPCODE_MOV_IM_TO_REG 0b1011
#define OPCODE_MOV_MEM_TO_ACC 0b1010000
#define OPCODE_MOV_ACC_TO_MEM 0b1010001

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


int main(int argc, char** argv)
{
    // byte_t b = 0b11011010;
    // byte_t n = byte_span(b, 1, 2);
    // PRINT_BYTE("0b", b, "\n");
    // PRINT_BYTE("0b", n, "\n");
    // return 0;

    char* in_path;
    if (argc == 2)
    {
        in_path = argv[1];
    }
    else
    {
        in_path = "data/listing_0040_challenge_movs";
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

    size_t buffer_size = 16;

    char *buff = (char*)malloc(buffer_size * 2);
    char *buffer[2];
    buffer[0] = buff;
    buffer[1] = buff + buffer_size;

    for (;;)
    {
        byte_t byte = get_byte(in_file);
        if (feof(in_file)) break;

        /* MOV: Register/memory to/from register */
        if (byte_span(byte, 2, 8) == OPCODE_MOV_REGMEM_TOFROM_REG)
        {
            byte_t D = byte_span(byte, 1, 2);
            byte_t W = byte_span(byte, 0, 1);

            byte_t next_byte = get_byte(in_file);
            byte_t MOD = byte_span(next_byte, 6, 8);
            byte_t REG = byte_span(next_byte, 3, 6);
            byte_t RM = byte_span(next_byte, 0, 3);

            char *src = buffer[0];
            char *dst = buffer[1];
            memset(src, 0, buffer_size);
            memset(dst, 0, buffer_size);

            strcpy(src, reg_field_encoding[W][REG]);

            switch (MOD)
            {
                /* Memory Mode, no displacement follows* */
                case 0b11:
                {
                    strcpy(dst, reg_field_encoding[W][RM]);
                }
                break;

                /* Memory Mode, 8-bit displacement follows */
                case 0b01:
                {
                    byte_t disp_lo = get_byte(in_file);
                    int disp = disp_lo + (disp_lo & 0x80 ? 0xff00 : 0);
                    sprintf(dst, "[%s + %d]", effective_address_calculation[RM], disp);
                }
                break;

                /* Memory Mode, 16-bit displacement follows */
                case 0b10:
                {
                    byte_t disp_lo = get_byte(in_file);
                    byte_t disp_hi = get_byte(in_file);
                    int disp = disp_lo + (disp_hi << 8);
                    sprintf(dst, "[%s + %d]", effective_address_calculation[RM], disp);
                }
                break;

                /* Register Mode (no displacement) */
                case 0b00:
                {
                    /* Direct Address */
                    if (RM == 0b110)
                    {
                        byte_t disp_lo = get_byte(in_file);
                        byte_t disp_hi = get_byte(in_file);
                        int direct_address = disp_lo + (disp_hi << 8);

                        sprintf(dst, "[%d]", direct_address);
                    }
                    else
                    {
                        sprintf(dst, "[%s]", effective_address_calculation[RM]);
                    }
                }
                break;
            }

            if (D)
            {
                SWAP_REG(src, dst);
            }


            fprintf(out_file, "mov %s, %s\n", dst, src);
        }
        /* Immediate to register/memory */
        else if (byte_span(byte, 1, 8) == OPCODE_MOV_IM_TO_REGMEM)
        {
            byte_t W = byte_span(byte, 0, 1);

            byte_t next_byte = get_byte(in_file);
            byte_t MOD = byte_span(next_byte, 6, 8);
            byte_t RM = byte_span(next_byte, 0, 3);

            char *dst = buffer[0];
            memset(dst, 0, buffer_size);

            switch (MOD)
            {
                /* Memory Mode, no displacement follows* */
                case 0b11:
                {
                    strcpy(dst, reg_field_encoding[W][RM]);
                }
                break;

                /* Memory Mode, 8-bit displacement follows */
                case 0b01:
                {
                    byte_t disp = get_byte(in_file);
                    sprintf(dst, "[%s + %d]", effective_address_calculation[RM], disp);
                }
                break;

                /* Memory Mode, 16-bit displacement follows */
                case 0b10:
                {
                    byte_t disp_lo = get_byte(in_file);
                    byte_t disp_hi = get_byte(in_file);
                    int disp = disp_lo + (disp_hi << 8);
                    sprintf(dst, "[%s + %d]", effective_address_calculation[RM], disp);
                }
                break;

                /* Register Mode (no displacement) */
                case 0b00:
                {
                    /* Direct Address */
                    if (RM == 0b110)
                    {
                        byte_t disp_lo = get_byte(in_file);
                        byte_t disp_hi = get_byte(in_file);
                        int direct_address = disp_lo + (disp_hi << 8);

                        sprintf(dst, "[%d]", direct_address);
                    }
                    else
                    {
                        sprintf(dst, "[%s]", effective_address_calculation[RM]);
                    }
                }
                break;
            }

            char* type = buffer[1];
            memset(type, 0, buffer_size);

            int data = get_byte(in_file);
            if (W)
            {
                byte_t data_w = get_byte(in_file);
                data = data + (data_w << 8);
                strcpy(type, "word");
            }
            else
            {
                strcpy(type, "byte");
            }

            fprintf(out_file, "mov %s, %s %d\n", dst, type, data);
        }
        /* Immediate to register */
        else if (byte_span(byte, 4, 8) == OPCODE_MOV_IM_TO_REG)
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
        /* Memory to accmulator */
        else if (byte_span(byte, 1, 8) == OPCODE_MOV_MEM_TO_ACC)
        {
            byte_t W = byte_span(byte, 0, 1);
            byte_t addr_lo = get_byte(in_file);
            byte_t addr_hi = get_byte(in_file);
            int addr = addr_lo + (addr_hi << 8);

            fprintf(out_file, "mov %s, [%d]\n", W ? "ax" : "al", addr);
        }
        /* Accumoulator to memory */
        else if (byte_span(byte, 1, 8) == OPCODE_MOV_ACC_TO_MEM)
        {
            byte_t W = byte_span(byte, 0, 1);
            byte_t addr_lo = get_byte(in_file);
            byte_t addr_hi = get_byte(in_file);
            int addr = addr_lo + (addr_hi << 8);

            fprintf(out_file, "mov [%d], %s\n", addr, W ? "ax" : "al");
        }
        else
        {
            // assert(0 && "Unsupported operation! Sorry :(");
            PRINT_BYTE("OP: ", byte, ". ");
            printf("Unsupported operation! Sorry :(\n");
        }
    }

    free(buff);

    fclose(out_file);
    fclose(in_file);
    return 0;
}
