
#ifndef _MY_COLOR_
#define _MY_COLOR_ 1
#include <types.h>

#define C_RESET       "\033[0m"
#define C_BLACK       "\033[0;30m"             /* Black */
#define C_RED         "\033[0;31m"             /* Red */
#define C_GREEN       "\033[0;32m"             /* Green */
#define C_YELLOW      "\033[0;33m"             /* Yellow */
#define C_BLUE        "\033[0;34m"             /* Blue */
#define C_MAGENTA     "\033[0;35m"             /* Magenta */
#define C_CYAN        "\033[0;36m"             /* Cyan */
#define C_WHITE       "\033[0;37m"             /* White */

#define B_BLACK       "\033[1;30m"             /* Black */
#define B_RED         "\033[1;31m"             /* Red */
#define B_GREEN       "\033[1;32m"             /* Green */
#define B_YELLOW      "\033[1;33m"             /* Yellow */
#define B_BLUE        "\033[1;34m"             /* Blue */
#define B_MAGENTA     "\033[1;35m"             /* Magenta */
#define B_CYAN        "\033[1;36m"             /* Cyan */
#define B_WHITE       "\033[1;37m"             /* White */

#define VERT_LINE       "│"
#define UPHAT_LINE      "┌──────────────────────────────────────────────────────────┐"
#define DOWNHAT_LINE    "└──────────────────────────────────────────────────────────┘"
#define NEXT_LINE       "\n"

#define KEY_ESC 	"\x1b"
#define KEY_UP 		KEY_ESC"[A"
#define KEY_DN 		KEY_ESC"[B"
#define KEY_LT 		KEY_ESC"[D"
#define KEY_RT 		KEY_ESC"[C"

#define KEY_SV 		KEY_ESC"7"
#define KEY_LD 		KEY_ESC"8"
#define KEY_CLEAN 	KEY_ESC"[0J"
#define KEY_HOME	KEY_ESC"[H"
#define KEY_CLR		KEY_ESC"[100M"

#define CONSOLE_WIDTH 255
#define CONSOLE_TEXT_WIDTH 58
#define MAXLINE 128

// My Shell ///////////////////////////////
#define CUT_SYM "<|>&;="
#define CUT_STR "\'\""
#define CUT_NAM "_-*?/."


#define SH_BLANK "\t "
#define SH_EOF "\r\n"
#define SH_SYM "<|>&;()"
#define SH_STR "\"\'="
#define SH_NAME CUT_NAM


#define SYM_NEXTLINE SH_EOF

#define VAR_NOWRITE  0x01
#define VAR_GLOBAL   0x10
#define VAR_SYS      0x02

#define VAR_STR_NOW  (C_RED"R"C_RESET)
#define VAR_STR_GLO  (C_GREEN"G"C_RESET)
#define VAR_STR_SYS  (C_CYAN"S"C_RESET)

#define VAR_WORKDIR "WorkSpace"

#define VARSTART 0x70000000
#define MAXVAR 16
#define MAXVARKEY 26
#define MAXVARVALUE (128 - sizeof(int) - sizeof(char))

typedef struct {
	u_int envid;
	char val[MAXVARVALUE];
	char mode;
} Var;

typedef struct {
    char keys[MAXVAR][MAXVARKEY];
    Var values[MAXVAR];
} VarMap;

#define VARMAP2PG (ROUND(sizeof(VarMap), BY2PG) >> PGSHIFT)

#endif /* _MY_COLOR_*/






