#include "lib.h"

void
umain(int argc, char **argv)
{
    char *color; int index = 0;
    if (argv[1]) {
        if (argv[1][0] == '1') color = B_BLUE, index = 1;
        else if (argv[1][0] == '2') color = B_CYAN, index = 2;
        else if (argv[1][0] == '3') color = B_GREEN, index = 3;
        else if (argv[1][0] == '4') color = B_MAGENTA, index = 4;
        else if (argv[1][0] == '5') color = B_YELLOW, index = 5;
        else if (argv[1][0] == '6') color = B_RED, index = 6;
        else color = C_RESET;
    } else color = C_RESET;


    int i;
    for (i = 0; i < 500; i++) {
        if (i % 100 == 0) writef_line("%sMyTestWait"C_RESET"[%d]: Wake Up %d/5 (%d)\n", color, env->env_id, i/100, index);
        syscall_yield();
    }
        

    writef_line("%sMyTestWait"C_RESET"[%d]: I'm Killed\n", color, env->env_id, i/100);
    exit(0);
}


