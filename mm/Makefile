INCLUDES	:= -I./ -I../ -I../include/

%.o: %.c
	@$(CC) $(CFLAGS) $(INCLUDES) -c $<

%.o: %.S
	@$(CC) $(CFLAGS) $(INCLUDES) -c $<

.PHONY: clean

all: pmap.o tlb_asm.o
	@echo "	\033[36m[RASP-MAKE]:	/mm finished compile.\033[0m"

clean:
	@rm -rf *~ *.o


include ../include.mk
