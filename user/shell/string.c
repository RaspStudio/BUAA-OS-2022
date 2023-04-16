#include <types.h>
#include <color.h>
#include "../lib/lib.h"

int
strlen(const char *s)
{
	int n;

	for (n = 0; *s; s++) {
		n++;
	}

	return n;
}

char *
strcpy(char *dst, const char *src)
{
	const char *p = src;
	while(*p != 0) {
		//writef("%c", *p);
		*(dst++) = *(p++);
	}
	*dst = 0;
	//writef("<END\n", *p);
	return (char*)p;
}

const char *
strchr(const char *s, char c)
{
	for (; *s; s++)
		if (*s == c) {
			return s;
		}

	return 0;
}

void *
memcpy(void *destaddr, void const *srcaddr, u_int len)
{
	char *dest = destaddr;
	char const *src = srcaddr;

	while (len-- > 0) {
		*dest++ = *src++;
	}

	return destaddr;
}

int
strcmp(const char *p, const char *q)
{
	if (p == NULL || q == NULL) {
		return p == q;
	}

	while (*p && *p == *q) {
		p++, q++;
	}

	if ((u_int)*p < (u_int)*q) {
		return -1;
	}

	if ((u_int)*p > (u_int)*q) {
		return 1;
	}

	return 0;
}

char *
strlcat(char *front, const char *back, unsigned long max_len) {
	int i = 0;
	/* Go To End Of Front */
	for (i = 0; i < max_len && front[i] != '\0'; i++);

	int j = 0;
	for (; i < max_len && back[j] !='\0'; i++, j++) {
		front[i] = back[j];
	}

	front[i] = '\0';

	if (i == max_len) {
		return NULL;
	}
	return front;
}

static char mysh_buf[CONSOLE_WIDTH + 1];
static char mysh_textbuf[CONSOLE_TEXT_WIDTH + 1];

char *my_mid_ltext(const char *text, u_long length) {
	int len = strlen(text);
	if (len > length) return NULL;

	int space = length - len;
	int lspace = (space & 1) ? (space >> 1) + 1 : (space >> 1);

	int i, j;
	for (i = 0; i < lspace; i++) mysh_textbuf[i] = ' ';
	for (j = 0; j < len; i++, j++) mysh_textbuf[i] = text[j];
	for (; i < length; i++) mysh_textbuf[i] = ' ';
	mysh_textbuf[length] = '\0';
	return mysh_textbuf;
}

char *my_mid_text(const char *text) {
	return my_mid_ltext(text, CONSOLE_TEXT_WIDTH);
}

char *my_console_line(const char *text, const char *line_color, const char *text_color) {
	/* Initialize */
	mysh_buf[0] = '\0';
	strlcat(mysh_buf, line_color, 		CONSOLE_WIDTH + 1);
	strlcat(mysh_buf, "│", 				CONSOLE_WIDTH + 1);
	strlcat(mysh_buf, text_color, 		CONSOLE_WIDTH + 1);
	strlcat(mysh_buf, my_mid_text(text), 	CONSOLE_WIDTH + 1);
	strlcat(mysh_buf, line_color, 		CONSOLE_WIDTH + 1);
	strlcat(mysh_buf, "│", 				CONSOLE_WIDTH + 1);
	strlcat(mysh_buf, NEXT_LINE, 			CONSOLE_WIDTH + 1);
	return mysh_buf;
}







