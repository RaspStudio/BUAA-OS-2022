# Main makefile
#
# Copyright (C) 2007 Beihang University
# Written by Zhu Like ( zlike@cse.buaa.edu.cn )
#

# ========= Configuration =========

INCLUDES	  := -I../include/

# ======== End of configuration ===

%.o: %.S
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< 

.PHONY: clean

all: start.o
	@echo "	\033[36m[RASP-MAKE]:	/boot finished compile.\033[0m"

clean:
	@rm -rf *~ *.o

include ../include.mk
