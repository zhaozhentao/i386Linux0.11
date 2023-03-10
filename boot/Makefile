include ../Makefile.header

LDFLAGS	+= -Ttext 0

bootsect: bootsect.S
	$(AS) -o bootsect.o bootsect.S
	$(LD) $(LDFLAGS) bootsect.o -o bootsect_elf
	$(OBJCOPY) -R .pdr -R .comment -R .note -O binary -S bootsect_elf bootsect
	$(OBJDUMP) -D -m i386 bootsect_elf > bootsect.dis

setup: setup.S
	$(AS) -o setup.o setup.S
	$(LD) $(LDFLAGS) setup.o -o setup_elf
	$(OBJCOPY) -R .pdr -R .comment -R .note -O binary -S setup_elf setup
	$(OBJDUMP) -D -m i386 setup_elf > setup.dis

clean:
	rm -f *.o bootsect bootsect.s setup setup.s *.dis *_elf

