INCLUDES := -I./ -I../ -I../include/
%.o: %.c
	@$(CC) $(CFLAGS) $(INCLUDES) -c $<

%.o: %.S
	@$(CC) $(CFLAGS) $(INCLUDES) -c $<

.PHONY: clean

all: kernel_elfloader.o env.o print.o printf.o sched.o env_asm.o kclock.o traps.o genex.o kclock_asm.o syscall.o syscall_all.o getc.o
	@echo "	\033[36m[RASP-MAKE]:	/lib finished compile.\033[0m"

clean:
	@rm -rf *~ *.o


include ../include.mk
