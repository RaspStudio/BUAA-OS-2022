#include "lib.h"
#include <args.h>

#define FAKE_RUN 0
#define CHECK 0

#define MAXARGS 15

#define SHR_FAIL 0
#define SHR_EOF 1
#define SHR_SYM 2
#define SHR_WD  3

#define PARSE_NOTINLINE 2
#define PARSE_INLINE 1

// SUDO ///////
int SuperShellFlag = 0;
int history_fd = -1;
// PARSER BUFFER /////////////////////////
char line[MAXLINE];

// RUNNER BUFFER /////////////////////////
int argc;
char *argv[MAXARGS + 1];

// FOR STEP BUFFER ///////////////////////
char *segments_step[MAXLINE];
int segments_type_step[MAXLINE];

// FOR CUT BUFFER ////////////////////////
char *segments[MAXLINE];
int segments_type[MAXLINE];
char segments_buffer[MAXLINE + MAXLINE];

// HELPER DECLARE ////////////////////////
int mys_op(char *word);
int mys_backup_fd(int fd[]);
void _move(int start, char *seg[], int seg_type[]);
void _copy(int end, char *seg[], int seg_type[]);
int _syntax_err(int errcode, const char *fmt, ...);
inline int _cut_word(char c);
void check(char buf[], char *pbuf[], int typ[]);
void _clean(int start, int end, char *seg[MAXLINE], int seg_type[MAXLINE]);
// INLINE RUNNER ///////////////////////////////////////////////////////////

void mys_runi() {
    if (strcmp(argv[0], "declare") == 0) {
        if (!FAKE_RUN) mys_declare(argc, argv);
    } else if (strcmp(argv[0], "unset") == 0) {
        if (!FAKE_RUN) mys_unset(argc, argv);
    } else if (strcmp(argv[0], "cd") == 0) {
        if (!FAKE_RUN) mys_cd(argc, argv);
    } else if (strcmp(argv[0], "quit") == 0) {
        if (SuperShellFlag) writef(B_MAGENTA "Shell" C_RESET ": Restart.\n");
        if (!FAKE_RUN) exit(0);
    } else {
        writef(B_RED"Shell"C_RESET": Cannot Recognize Inline Op!\n");
        return;
    }
    if (FAKE_RUN) {
        int j;
        writef(B_MAGENTA "Shell" C_RESET ": Ready To Run Non-Inline Op(");
        for (j = 0; j < argc; j++) {
            if (j > 0) writef(", ");
            writef("%s", argv[j]);
        } writef(")\n");
        return;
    }
}

// PARSER ///////////////////////////////////////////////////////////

void mys_parseVar() {
    int lineIndex = 0;
    int inVar = 0;
    char templine[MAXLINE] = {0};
    char word[MAXLINE] = {0};
    char *templp = templine;

    while (inVar || (line[lineIndex] != 0 && strchr(SH_EOF, line[lineIndex]) == 0)) {
        //while (syscall_cgetc() != '0');
        //writef("%s<-%s-%s\n", templine, inVar ? "$" : "", line);
        if (line[lineIndex] == '$') {
            inVar = 1;
            lineIndex++;
        } else if (inVar) {
            if (_cut_word(line[lineIndex])) {
                word[inVar-1] = line[lineIndex++];
                inVar++;
            } else {
                word[inVar-1] = 0;
                char tempVal[MAXVARVALUE] = {0};
                var_get(word, tempVal);
                strcpy(templp, tempVal);
                templp += strlen(tempVal);
                inVar = 0;
                //writef("Out\n");
                *(templp++) = line[lineIndex++];
            }
        } else {
            *(templp++) = line[lineIndex++];
        }
    }
    //writef("NewLine=%s\n", templine);
    strcpy(line, templine);
}

// GENERAL RUNNER /////////////////////////////////////////////////////////

int mys_prepare() {
    int i;
    //writef("Prepare Start!!\n");
    argc = 0;
    user_bzero(argv,sizeof(argv));
    for (i = 0; segments_step[i] != NULL; i++) {
        if (strchr("ws", (char)segments_type_step[i])) {
            int j = 0; argv[argc++] = segments_step[i];
            //writef("w:%s.(%d)\t",argv[argc-1], i);
        } 
        else if (i > 0 && segments_type_step[i] == '>' && strchr("ws", (char)segments_type_step[i+1])) {
            redirect_stdout(segments_step[i+1]); i++; 
        } 
        else if (i > 0 && segments_type_step[i] == '<' && strchr("ws", (char)segments_type_step[i+1])) {
            redirect_stdin(segments_step[i+1]); i++; 
        } 
        else if (i > 0 && segments_type_step[i] == '=' && strchr("ws", (char)segments_type_step[i+1])) {
            int j = 0; argv[argc++] = segments_step[i];
            //writef("s:%s.(%d)\t",argv[argc-1], i);
        }
        else {
            _syntax_err(0, "Prepare Failed \'%c\'", (char)segments_type_step[i]); exit(0);
        }
    }

    argv[argc] = 0;

    // writef("%d>-%d-", i, argc);
    // for (i = 0; i < argc; i++) {
    //     writef("%s,",argv[i]);
    // }
    return 0;
}

void mys_finish(int son, int quit) {

    if (argc == 0) {
        //writef(B_RED "ERROR:" C_RESET "Empty Command!\n");
        if (quit) exit(0); else return;
    }

    // EXECUTE //////////////////////////////////////////////////////////////////
    if (FAKE_RUN) {
        int j;
        writef(B_MAGENTA "Shell" C_RESET "[%d]: Ready To Run Op(", env->env_id);
        for (j = 0; j < argc; j++) {
            if (j > 0) writef(", ");
            writef("%s", argv[j]);
        } writef(")\n");
    } else     
    // Real Run
    if (mys_op(argv[0])) { // Inline Op
        mys_runi();
    } else {    // Outside Op
        int spawned;
        if ((spawned = spawn(argv[0], argv)) < 0) {
            writef(B_RED "ERROR:" C_RESET "Failed To Spawn!\n");
        } else {
            OUT writef(B_BLUE"New"C_RESET"[%d]: %s\n", spawned, argv ? (argv[0] ? argv[0] : "No Arg0") : "No Arg");
            close_all();
            if (SuperShellFlag)  {
                if (opencons() == 0) {
                    dup(0,1); history_fd = history_open();
                } else writef(B_RED "ERROR:" C_RESET "Failed To Restore!\n");
            }
            // Wait Own Spawn
            if (spawned > 0) wait(spawned);
        }
    }
    
    // FINISH //////////////////////////////////////////////////////////////////
    if (son) wait(son);
    // If Not Father, Exit.
    if (quit) exit(0);
}

// SHELLS STEP ///////////////////////////////////////////////////////////

// Move Segments_step (Every Step In)
int mys_run() {
    //check(segments_buffer, segments_step, segments_type_step);

    int i = 0;
    int top = 1;
    int forkid = -1;
    
    while (segments_step[i] != NULL) {
        if (segments_type_step[i] == '|') {
            if (top) {
                /* First Pipe, Fork And Wait Till End */
                if ((forkid = fork()) != 0) {
                    wait(forkid);
                    return 0;
                } else {
                    SuperShellFlag = 0;
                    top = 0;
                }
            } else {
                /* Son Shell Do Normal Pipe */
                int pipeFd[2];
                pipe(pipeFd);
                if ((forkid = fork()) != 0) {
                    /* Son Shell : Run Left Wait Right */
                    OUT writef(B_MAGENTA"Shell"C_RESET"[%d]: Pipe To [%d]\n", env->env_id, forkid);
                    segments_step[i] = NULL;
                    segments_type_step[i] = 0;
                    mys_prepare();
                    dup(pipeFd[1], 1);
                    close(pipeFd[1]);
                    close(pipeFd[0]);
                    mys_finish(forkid, 1);
                } else {
                    dup(pipeFd[0], 0);
                    close(pipeFd[0]);
                    close(pipeFd[1]);
                    _move(i+1, segments_step, segments_type_step);
                    i = 0;
                }
            }
        } else {
            i++;
        }
    }

    // No Pipe, Step/SonSonSon Shell's Instruction.
    mys_prepare();
    mys_finish(0, !top);
    /* Able To Return, Only Step Shell */
    return 0;
}

// Use ';' Step By Step Run (Splitted Enter : Only Super Back)
void mys_step() {
    check(segments_buffer, segments, segments_type);

    int i = 0;
    while (segments[i] != NULL) {
        if (segments_type[i] == ';') {
            _copy(i, segments_step, segments_type_step);
            if (mys_run() < 0 && !SuperShellFlag) exit(-1);
            _move(i + 1, segments, segments_type);
            i = 0;
        } else {
            i++;
        }
    }

    _copy(MAXLINE, segments_step, segments_type_step);

    /* When Return, No ';' */
    mys_run();
    if (!SuperShellFlag) exit(0);
}

// SHELL GENERATOR /////////////////////////////////////////
// Use '&' Split To Child Shell (Super Enter : Child & Super Out)
void mys_split() {
    int i = 0; int r;
    while (segments[i] != NULL) {
        if (segments_type[i] == '&') {
            if ((r=fork()) == 0) {
                /* Son Was Splited */
                SuperShellFlag = 0;
                _clean(i, MAXLINE, segments, segments_type);
                break;
            } else {
                OUT writef("Split: Sonshell[%d] is created.\n", r);
                _move(i + 1, segments, segments_type);
                i = 0;
            }
        } else {
            i++;
        }
    }
    /* When Return, No '&' */
}

// Super in, Super Out
void mys_process() {
    //writef("START PROCESS\n");
    mys_split();//writef(B_RED"RUN SPLIT\n"C_RESET);
    mys_step();
    if (!SuperShellFlag) user_panic("Lou Wang Le");
}

// CUT ELEMENTS ////////////////////////////////////////////
int mys_cut() {
    char *cur = line;
    
    char **dst = segments;
    int  *type = segments_type;
    char *buf = segments_buffer;

    user_bzero(segments, sizeof(segments));
    user_bzero(segments_type, sizeof(segments_type));
    user_bzero(segments_buffer, sizeof(segments_buffer));

    /* Cut Till Line End */
    while ((*cur) != 0) {
        if (strchr(SH_BLANK, *cur)) {
            /*--- Skip Blank ---*/
            cur++; continue;
        } else if (strchr(CUT_SYM, *cur)) {
            /*--- Splitter: SaveP - SetType - Copy ---*/
            *(dst++) = buf;
            *(type++) = *cur;
            *(buf++) = *(cur++); 
            *(buf++) = 0;
        } else if (strchr(CUT_STR, *cur)) {
            /*--- String: SaveP - SetType - Copy ---*/
            const char *temp;
            if ((temp = strchr(cur+1, *cur)) != NULL) {
                *(dst++) = buf;
                *(type++) = 's';
                cur++;// skip quote
                while(cur < temp) *(buf++) = *(cur++);
                cur++;
                *(buf++) = 0;
            } else {
                return _syntax_err(-E_INVAL, "Quote Not Matched!");
            }
        } else if (_cut_word(*cur)) {
            /*--- Word: SaveP - SetType - Copy ---*/
            *(dst++) = buf;
            *(type++) = 'w';
            while(_cut_word(*cur)) *(buf++) = *(cur++);
            *(buf++) = 0;
        } else if (*cur == '$') {
            char key[MAXVARKEY] = {0}; 
            char *pkey = key;
            cur++; // skip $
            while(_cut_word(*cur)) *(pkey++) = *(cur++);
            *(pkey++) = 0;
            char val[MAXVARVALUE] = {0};
            var_get(key, val);
            pkey = val;
            *(dst++) = buf;
            *(type++) = 'w';
            while (*pkey) *(buf++) = *(pkey++);
            *(buf++) = 0;
        } else {
            return _syntax_err(-E_INVAL, "Unknown Char \'%c\'", *cur);
        }
    }
    
    *(dst) = 0;
    *(type) = 0;

    int i;

    return 0;
}

// MAIN ////////////////////////////////////////////////////////////////
void umain(int argc, char **argv) {
    if (strcmp(argv[0], "mysh"))
        SuperShellFlag = 1;

    set_pgfault_handler(pgfault);
    history_fd = history_open();

    mys_title();
    for (;;) {
        // Init Shell Line
        char temp[MAXVARVALUE] = {0};
        var_get(VAR_WORKDIR, temp);
        mys_startln(temp, SuperShellFlag);

        // Read And Store Line
        mys_nextln(line, MAXLINE, history_fd);
        history_add(history_fd, line);

        // Parse Line
        mys_cut();

        // Run Line
        mys_process();
    }
}










// HELPER //////////////////////////////////////////////////////////////
int _syntax_err(int errcode, const char *fmt, ...) {
	va_list ap;
    va_start(ap, fmt);
    writef_base(fmt, ap);
    va_end(ap);
	return errcode;
}

// Move Segment Pointers
void _move(int start, char *seg[], int seg_type[]) {
    int j = 0;
    //writef("\nMove Else:");
    // for (j = start; seg[j]; j++) {
    //     writef("%s ", seg[j]);
    // }writef("\n");
    for (j = 0; j + start < MAXLINE; j++) {
        seg[j] = seg[start+j];
        seg_type[j] = seg_type[start+j];
    }
    for (; j < MAXLINE; j++) {
        seg[j] = NULL;
        seg_type[j] = 0;
    }
}

// Backup Original Fd
int mys_backup_fd(int fd[]) {
    struct Fd *p;
    if (fd_alloc(&p) < 0) {
        _syntax_err(-E_INVAL, "BackUp Current Fd Failed");
        return -E_INVAL;
    }
    fd[0] = fd2num(p);
    dup(0, fd[0]);

    if (fd_alloc(&p) < 0) {
        _syntax_err(-E_INVAL, "BackUp Current Fd Failed");
        return -E_INVAL;
    }
    

    fd[1] = fd2num(p);
    dup(1, fd[1]);
    // writef("BACKUP TWO\n");
    return 0;
}

int mys_op(char *word) {
    return strcmp(word, "declare") == 0 ||
           strcmp(word, "unset") == 0 ||
           strcmp(word, "cd") == 0 ||
           strcmp(word, "quit") == 0;
}

inline int _cut_word(char c) {
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') ||
           (strchr(CUT_NAM, c)); 
}

void check(char buf[], char *pbuf[], int typ[]) {
    if (!CHECK) return;
    int i;
    writef("Here's Buffer:\n");
    for (i = 0; i < MAXLINE; i++) {
        writef("%c", buf[i] == 0 ? ' ' : buf[i]);
    }writef("\nHere's Pointer Offset:\n");
    for (i = 0; i < MAXLINE; i++) {
        writef(" %d", pbuf[i] == 0 ? -1 : pbuf[i] - buf);
    }writef("\nHere's Type\n");
    for (i = 0; i < MAXLINE; i++) {
        writef(" %c", typ[i] == 0 ? ' ' : typ[i]);
    }writef("\nEnd Check\n");
}

void _copy(int end, char *seg[MAXLINE], int seg_type[MAXLINE]) {
    
    user_bzero(segments_step, sizeof(segments_step));
    user_bzero(segments_type_step, sizeof(segments_type_step));
    int j;
    for (j = 0; j < end; j++) {
        segments_step[j] = segments[j];
        segments_type_step[j] = segments_type[j];
    }
}

void _clean(int start, int end, char *seg[MAXLINE], int seg_type[MAXLINE]) {
    int i;
    if (end > MAXLINE) end = MAXLINE;
    if (start < 0) start = 0;
    for (i = start; i < end; i++) {
        seg[i] = NULL;
        seg_type[i] = 0;
    }
}









