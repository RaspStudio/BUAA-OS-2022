include ../include.mk

INCLUDES = -I../include/ -I./lib/
CFLAGS += -nostdlib -static

all: icode.x  $(USERAPP)\
	$(USERLIB) lib/entry.o lib/syscall_wrap.o fs
	@echo "	\033[36m[RASP-MAKE]	/user finished compile.\033[0m"

%.x: %.b.c 
	@$(CC) $(CFLAGS) -c -o $@ $<

%.b.c: %.b
	@chmod +x ./bintoc
	@./bintoc $* $< > $@~ && mv -f $@~ $@
	
%.b: lib/entry.o lib/syscall_wrap.o %.o $(USERLIB)
	@$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib -T ./lib/user.lds $^

%.o: %.c
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.S
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: lib.h

fs:	
	$(MAKE) --directory=$(USER_DIR)/fs

.PHONY: clean fs

clean:
	@rm -rf *~ *.o *.b.c *.x *.b
	@rm -rf ./lib/*~ ./lib/*.o ./lib/*.b.c ./lib/*.x ./lib/*.b
	@rm -rf ./shell/*~ ./shell/*.o ./shell/*.b.c ./shell/*.x ./shell/*.b
	$(MAKE) clean --directory=$(USER_DIR)/fs


