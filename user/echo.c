#include "lib.h"

void
umain(int argc, char **argv)
{
    int i, nflag;

    nflag = 0;
    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        nflag = 1;
        argc--;
        argv++;
    }   
    for (i = 1; i < argc; i++) {
        if (i > 1)
            write(1, " ", 1); 
        //writef("\n"B_MAGENTA"Echo:"C_RESET" Write Start.\n");    
        write(1, argv[i], strlen(argv[i]));
        //writef("\n"B_MAGENTA"Echo:"C_RESET" Write End.\n");
    }   
    if (!nflag)
        fwritef(1, "\n");
        //fwritef(1, "\n"B_MAGENTA"Echo:"C_RESET" Finished\n"); 
}

