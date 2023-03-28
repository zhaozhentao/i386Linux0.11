include ../Makefile.header

FLAGS += -Ttext 0
LDFLAGS += -Ttext 0

bootsect: bootsect.s
	$(AS) -o bootsect.o bootsect.s
	$(LD) $(LDFLAGS) bootsect.o -o bootsect_elf
	$(OBJCOPY) -R .pdr -R .comment -R .note -O binary -S bootsect_elf bootsect
	$(OBJDUMP) -D -m i386 bootsect_elf > bootsect.dis

setup: setup.s
	$(AS) -o setup.o setup.s
	$(LD) $(LDFLAGS) setup.o -o setup_elf
	$(OBJCOPY) -R .pdr -R .comment -R .note -O binary -S setup_elf setup

clean:
	rm -f *.o *_elf *.dis bootsect setup

