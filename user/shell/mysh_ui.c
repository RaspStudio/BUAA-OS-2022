#include "../lib/lib.h"
#include <color.h>

void mys_title() {
    writef_line(NEXT_LINE);
	writef_line(C_YELLOW UPHAT_LINE C_RESET NEXT_LINE);
	writef_line("%s", my_console_line("", C_YELLOW, B_YELLOW));
	writef_line("%s", my_console_line("MOS Shell - By RaspStudio", C_YELLOW, B_YELLOW));
	writef_line("%s", my_console_line("", C_YELLOW, B_YELLOW));
	writef_line(C_YELLOW DOWNHAT_LINE C_RESET NEXT_LINE);
	writef_line(NEXT_LINE);
}

void usr_title(const char *name) {
	writef_line(NEXT_LINE);
	writef_line(C_YELLOW UPHAT_LINE C_RESET NEXT_LINE);
	writef_line("%s", my_console_line(name, C_YELLOW, B_YELLOW));
	writef_line(C_YELLOW DOWNHAT_LINE C_RESET NEXT_LINE);
	writef_line(NEXT_LINE);
}

void mys_startln(const char *path, int flag) {
	writef_line(B_GREEN"MOS@xxxxxxxx"C_RESET"[%d]:"B_BLUE"~%s"C_RESET"$ ", env->env_id, path ? path : "");
}






