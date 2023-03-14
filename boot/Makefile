include ../Makefile.header

LDFLAGS	+= -Ttext 0

bootsect: bootsect.s
	$(AS) -o bootsect.o bootsect.s
	$(LD) $(LDFLAGS) bootsect.o -o bootsect_elf
	$(OBJCOPY) -R .pdr -R .comment -R .note -O binary -S bootsect_elf bootsect
	$(OBJDUMP) -D -m i386 bootsect_elf > bootsect.dis

setup: setup.s
	$(AS) -o setup.o setup.s
	$(LD) $(LDFLAGS) setup.o -o setup_elf
	$(OBJCOPY) -R .pdr -R .comment -R .note -O binary -S setup_elf setup
	$(OBJDUMP) -D -m i386 setup_elf > setup.dis

head.o: head.s
	@$(AS) -o head.o head.s

clean:
	rm -f bootsect setup *.o *.dis *_elf

