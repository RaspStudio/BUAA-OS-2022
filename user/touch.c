#include "lib.h"

void
umain(int argc, char **argv)
{
    if (argc == 1) writef(B_MAGENTA"Touch"C_RESET": Missing Operand!");
    int i;
    for (i = 1; i < argc; i++) {
        if (open(argv[i], O_CREAT) < 0) {
            writef(B_RED"Touch"C_RESET": Failed To Create \'%s\'!", argv[i]);
        }
    }
    exit(0);
}


