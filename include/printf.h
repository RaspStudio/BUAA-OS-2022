/*
 * Copyright (C) 2001 MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef _printf_h_
#define _printf_h_

#include <stdarg.h>
void printf(char *fmt, ...);

void _panic(const char *, int, const char *, ...) 
	__attribute__((noreturn));

#define panic(...) _panic(__FILE__, __LINE__, __VA_ARGS__)


#endif /* _printf_h_ */
