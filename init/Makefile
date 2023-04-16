INCLUDES := -I../include

%.o: %.c
	@$(CC) $(CFLAGS) $(INCLUDES) -c $<

.PHONY: clean

all: init.o main.o code.o check_icode.o
	@echo "	\033[36m[RASP-MAKE]:	/init	finished compile.\033[0m"

clean:
	@rm -rf *~ *.o


include ../include.mk
