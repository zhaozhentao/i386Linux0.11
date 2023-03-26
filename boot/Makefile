include ../Makefile.header

LDFLAGS += -Ttext 0

bootsect: bootsect.s
    $(AS) -o bootsect.o bootsect.s
    $(LD) $(LDFLAGS) bootsect.o -o bootsect_elf
    $(OBJCOPY) -R .pdr -R .comment -R .note -O binary -S bootsect_elf bootsect
    $(OBJDUMP) -D -m i386 bootsect_elf > bootsect.dis

clean:
    rm -f *.o *_elf bootsect
