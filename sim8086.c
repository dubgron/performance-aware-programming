#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef unsigned char byte_t;

byte_t byte_span(byte_t byte, int begin, int end)
{
    return (byte - ((byte >> end) << end)) >> begin;
}

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

#define OPCODE_MOV 0b100010

const char* reg_field_encoding[2][8] = {
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


int main(int argc, char** argv)
{
    // byte_t b = 0b11011010;
    // byte_t n = byte_span(b, 1, 2);
    // PRINT_BYTE("0b", b, "\n");
    // PRINT_BYTE("0b", n, "\n");
    // return 0;

    if (argc < 2)
    {
        printf("One argument required, none provided!\n");
        return 1;
    }

    // char *in_path = "data/listing_0037_single_register_mov";
    char *in_path = argv[1];

    FILE *in_file = fopen(in_path, "rb");
    if (!in_file)
    {
        printf("Failed to open %s!", in_path);
        return 1;
    }

    char *out_path = (char*)malloc(strlen(in_path) + 4);
    sprintf(out_path, "%s.asm", in_path);

    FILE *out_file = fopen(out_path, "w");
    if (!out_file)
    {
        printf("Failed to open %s!", out_path);
        return 1;
    }
    free(out_path);

    fprintf(out_file, "; %s dissasembly\n", strrchr(in_path, '/') + 1);
    fprintf(out_file, "bits 16\n");

    for (;;)
    {
        byte_t byte = fgetc(in_file);
        if (feof(in_file)) break;

        PRINT_BYTE("OP: ", byte, "\n");

        char instruction[3];

        /* MOV */
        if ((byte >> 2) == OPCODE_MOV)
        {
            memcpy(instruction, "mov", 3);

            byte_t D = byte_span(byte, 1, 2);
            byte_t W = byte_span(byte, 0, 1);

            byte_t next_byte = fgetc(in_file);
            byte_t MOD = byte_span(next_byte, 6, 8);
            byte_t REG = byte_span(next_byte, 3, 6);
            byte_t RM = byte_span(next_byte, 0, 3);

            PRINT_BYTE("PAR: ", next_byte, "\n");

            if (MOD == 0b11)
            {
                if (D)
                {
                    fprintf(out_file, "%s %s, %s\n", instruction, reg_field_encoding[W][REG], reg_field_encoding[W][RM]);
                }
                else
                {
                    fprintf(out_file, "%s %s, %s\n", instruction, reg_field_encoding[W][RM], reg_field_encoding[W][REG]);
                }
            }
            else
            {
                assert(0 && "Unsupported MOD!");
            }
        }
        else
        {
            assert(0 && "Unsupported operation! Sorry :(");
        }
    }

    fclose(out_file);
    fclose(in_file);
    return 0;
}
