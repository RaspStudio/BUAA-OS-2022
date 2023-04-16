#include "lib.h"
#define Stdin 0
#define Stdout 1


const char *_fdname(int targetFd) {
    switch (targetFd)
    {
        case 0: return "Stdin";
        case 1: return "Stdout";
        default: return "UserFile";
    }
}

int _redirect(const char *path, int mode, int targetFd) {
    int fdnum = open(path, mode);
    if (fdnum < 0) {
        writef(B_MAGENTA "Shell" C_RESET ": Failed To Open \'%s\', Process Terminated.\n", path);
        return -E_NOT_FOUND;
    } else {
        OUT writef(B_MAGENTA "Shell" C_RESET "[%d]: Redirecting %s To \'%s\'......\n", env->env_id, _fdname(targetFd), path);
        dup(fdnum, targetFd);
        close(fdnum);
        return 0;
    }
}

int redirect_stdin(const char *path) {
    return _redirect(path, O_RDONLY, Stdin);
}

int redirect_stdout(const char *path) {
    return _redirect(path, O_WRONLY | O_CREAT, Stdout);
}


void __mys_next(char *buf) {
    int r = read(Stdin, buf, 1);
    if (r == 0) {
        *buf = 0;
    } else if (r != 1) {
        writef(B_RED "ERROR" C_RESET ": Failed To Read. Error Code: " "%d", r);
    }
}

void clean() {
    writef(KEY_LD);
    writef(KEY_CLEAN);
}

void mys_nextln(char *buf, u_long size, int history_fd) {
    int i = 0;
    int arrow = 0;
    writef(KEY_SV);
    //usr_title("NEWLINE");
    while (i < size) {
        /*----- Get Next Char -----*/
        __mys_next(buf + i);
        if (i == 0 && buf[i] == 0) {
            strcpy(buf, "quit");
            return;
        }

        /*----- Analyze Char -----*/
        switch (buf[i]) {
            case '\x7f':
                if (i > 0) i--;
                else writef(" ");
                break;
            case '\x1b':
                arrow = 1;// Arrow's 1/3
                break;
            case '\x5b':
                if (arrow == 1) {
                    arrow = 2;// Arrow's 2/3
                    break;
                }
            default:
                if (arrow == 2) {
                    if (buf[i] == 'A') {
                        buf[i] = 0;
                        clean();
                        history_up(history_fd, buf);
                        i = strlen(buf);
                    } else if (buf[i] == 'B') {
                        buf[i] = 0;
                        clean();
                        history_down(history_fd, buf);
                        i = strlen(buf);
                    } else if (buf[i] == 'C'){
                        buf[i] = 0;
                        clean();
                    } else if (buf[i] == 'D'){
                        buf[i] = 0;
                        clean();
                    }
                    writef("%s", buf);
                    arrow = 0;
                    break;
                }
                if (strchr(SH_EOF, buf[i]) || buf[i] == 0) {
                    buf[i] = '\0';
                    return;
                } else {
                    i++;
                }
                arrow = 0;
        }
        
    }

    writef_line("\n" B_YELLOW "Warning" C_RESET ": Command Too Long!\n");
    buf[0] = '\0';
}


void mys_cd(int argc, char **argv) {
    if (argc == 2) {
        struct Stat st;
	    if ((stat(argv[1], &st)) < 0){
		    writef(B_RED "ERROR:" C_RESET "No Such Directory!\n");
	    } 
        if (st.st_isdir) {
            char path[MAXPATHLEN] = {0};
            if (argv[1][0] != '/')
                var_get(VAR_WORKDIR, path);
            strlcat(path, argv[1], MAXPATHLEN);

            writef("AFTER:%s\n", path);
            var_add(VAR_GLOBAL, VAR_WORKDIR, path);
            writef(B_MAGENTA "Shell:" C_RESET " Changed To %s!\n", argv[1]);
        }
    } else {
        writef(B_RED "ERROR:" C_RESET "Invalid Cd Argument!\n");
    }
}

void mys_declare(int argc, char **argv) {
    if (argc == 1) {
        var_add(0,0,0);
        return;
    }
    int i;
    int input = 1;
    int perm = 0;
    char *a = NULL;
    char *b = NULL;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && input) {
            switch (argv[i][1]) {
                case 'x':
                    perm |= VAR_GLOBAL;
                break;
                case 'r':
                    perm |= VAR_NOWRITE;
                break;
                case 's':
                    perm |= VAR_SYS;
                break;
                default:
                    writef(B_MAGENTA "Shell:" C_RESET " Unimplemented Option '%s'!\n", argv[i]);
                break;
            }
        } else {
            if (input == 1) {
                a = argv[i];
                input++;
            }
            if (input == 2 && argv[i][0] == '=') {
                input++;
            }
            if (input == 3) {
                b = argv[i];
            }
        }
    }
    var_add(perm, a, b);
}

void mys_unset(int argc, char **argv) {
    if (argc == 2) {
        var_del(argv[1]);
    } else {
        writef(B_RED "Shell:" C_RESET " Usage unset NAME.\n");
    }
}
















