include ../Makefile.header

LDFLAGS	+= -Ttext 0

bootsect: bootsect.S
	$(AS) -o bootsect.o bootsect.S
	$(LD) $(LDFLAGS) bootsect.o -o bootsect_elf
	$(OBJCOPY) -R .pdr -R .comment -R .note -O binary -S bootsect_elf bootsect
	$(OBJDUMP) -D -m i386 bootsect_elf > bootsect.dis

clean:
	rm -f *.o bootsect bootsect.s *.dis *_elf

