
include ../../include.mk

INCLUDES 	= 	-I$(WORK_DIR)/include/ -I$(USER_DIR)/lib/

tools_dir 	:= 	.
fsformat  	:= 	./fsformat
user_disk	:=  $(WORK_DIR)/gxemul/fs.img


FSLIB :=	\
		fs.o \
		ide.o \
		test.o


FSIMGFILES := motd newmotd mytest $(USERAPP)

CFLAGS += -nostdlib -static

all: serv.x fs.img
	@echo "	\033[36m[RASP-MAKE]:	/fs	finished compile.\033[0m"

%.x: %.b.c
	@$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<

%.b.c: %.b
	@chmod +x $(tools_dir)/bintoc
	@$(tools_dir)/bintoc $* $< > $@~ && mv -f $@~ $@
	
%.b: $(USER_DIR)/lib/entry.o $(USER_DIR)/lib/syscall_wrap.o %.o $(USERLIB) $(FSLIB)
	@$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib -T $(USER_DIR)/lib/user.lds $^

%.o: %.c
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.S
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: $(USER_DIR)/lib/lib.h

fs.img: $(FSIMGFILES)
	@dd if=/dev/zero of=$(user_disk) bs=4096 count=1024 2>/dev/null
	@gcc $(INCLUDES) fsformat.c -w -o $(tools_dir)/fsformat -m32
	@chmod +x $(tools_dir)/fsformat
	@$(tools_dir)/fsformat $(user_disk) $(FSIMGFILES)

.PHONY: clean

clean:
	@rm -rf *~ *.o *.b.c *.b *.x $(fsformat)

