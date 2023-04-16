# Makefile for gxconsole module
#
# Copyright (C) 2007 Beihang Unversity.
# Written by Zhu Like, zlike@cse.buaa.edu.cn

%.o: %.c %.h
	@$(CC) $(CFLAGS) -c $< -o $*.o

.PHONY: clean
all: console.o
	@echo "	\033[36m[RASP-MAKE]:	/drivers finished compile.\033[0m"

clean:
	@rm -rf *.o *~



include ../../include.mk
