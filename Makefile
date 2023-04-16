# Main makefile
#
# Copyright (C) 2007 Beihang University
# Written by Zhu Like ( zlike@cse.buaa.edu.cn )
#

drivers_dir	  := drivers
boot_dir	  := boot
user_dir	  := user
init_dir	  := init
lib_dir		  := lib
fs_dir		  := user/fs
mm_dir		  := mm
tools_dir	  := tools
vmlinux_elf	  := gxemul/vmlinux
user_disk     := gxemul/fs.img

link_script   := $(tools_dir)/scse0_3.lds

modules		  := boot drivers init lib mm user
objects		  := $(boot_dir)/start.o			  \
				 $(init_dir)/*.o			  \
			   	 $(drivers_dir)/gxconsole/console.o \
				 $(lib_dir)/*.o				  \
				 $(user_dir)/*.x \
				 $(fs_dir)/*.x \
				 $(mm_dir)/*.o

.PHONY: all $(modules) clean

all: $(modules) vmlinux
	@echo "	\033[32m[RASP-MAKE]:	GLOBAL COMPILE FINISHED.\033[0m"

vmlinux: $(modules)
	@$(LD) -o $(vmlinux_elf) -N -T $(link_script) $(objects)

$(modules): 
	@$(MAKE) --directory=$@

clean: 
	@for d in $(modules);	\
		do					\
			$(MAKE) --directory=$$d clean; \
		done; \
	rm -rf *.o *~ $(vmlinux_elf)  $(user_disk)
	@echo "	\033[32m[RASP-MAKE]:	GLOBAL CLEAN FINISHED.\033[0m"

run:
	@echo "	\033[32m[RASP-MAKE]:	START ON NORMAL MODE.\033[0m"
	@echo "\033[32m------------------------------------------------------------\033[0m"
	@/OSLAB/gxemul -q -E testmips -C R3000 -M 64 -d gxemul/fs.img $(vmlinux_elf)
	

runv:
	@echo "	\033[32m[RASP-MAKE]:	START ON DEBUG MODE.\033[0m"
	@echo "\033[32m------------------------------------------------------------\033[0m"
	@/OSLAB/gxemul -q -E testmips -C R3000 -M 64 -d gxemul/fs.img $(vmlinux_elf) -V

o:
	@make clean
	
	@make
	@make run

v:
	@make clean
	@make 
	@make runv


include include.mk
