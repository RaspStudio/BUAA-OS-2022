# Common includes in Makefile
#
# Copyright (C) 2007 Beihang University
# Written By Zhu Like ( zlike@cse.buaa.edu.cn )



# Exercise 1.1. Please modify the CROSS_COMPILE path.

CROSS_COMPILE := /OSLAB/compiler/usr/bin/mips_4KC-
CC            := $(CROSS_COMPILE)gcc
CFLAGS        := -O -G 0 -mno-abicalls -fno-builtin -Wa,-xgot -Wall -fPIC -march=r3000
LD            := $(CROSS_COMPILE)ld
DUMP		  := $(CROSS_COMPILE)objdump

WORK_DIR	  := /home/git/xxxxxxxx
USER_DIR 	  := $(WORK_DIR)/user
USERLIB := \
		$(USER_DIR)/lib/printf.o \
		$(USER_DIR)/lib/print.o \
		$(USER_DIR)/lib/libos.o \
		$(USER_DIR)/lib/fork.o \
		$(USER_DIR)/lib/pgfault.o \
		$(USER_DIR)/lib/syscall_lib.o \
		$(USER_DIR)/lib/ipc.o \
		$(USER_DIR)/lib/fd.o \
		$(USER_DIR)/lib/pageref.o \
		$(USER_DIR)/lib/file.o \
		$(USER_DIR)/lib/pipe.o \
		$(USER_DIR)/lib/fsipc.o \
		$(USER_DIR)/lib/wait.o \
		$(USER_DIR)/lib/spawn.o \
		$(USER_DIR)/lib/console.o \
		$(USER_DIR)/lib/fprintf.o \
		$(USER_DIR)/lib/loader.o \
		$(USER_DIR)/shell/mysh_ui.o \
		$(USER_DIR)/shell/mysh_io.o	 \
		$(USER_DIR)/shell/mysh_history.o\
		$(USER_DIR)/shell/mysh_var.o	 \
		$(USER_DIR)/shell/string.o 

USERAPP := \
		$(USER_DIR)/echo.b  \
		$(USER_DIR)/num.b   \
		$(USER_DIR)/init.b  \
		$(USER_DIR)/cat.b  \
		$(USER_DIR)/ls.b  \
		$(USER_DIR)/history.b \
		$(USER_DIR)/mkdir.b \
		$(USER_DIR)/touch.b \
		$(USER_DIR)/mysh.b \
		$(USER_DIR)/clear.b \
		$(USER_DIR)/tree.b \
		$(USER_DIR)/mtwt.b 
		
		
		
		
		