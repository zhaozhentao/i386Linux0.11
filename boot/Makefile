include ../Makefile.header

LDFLAGS	+= -Ttext 0

bootsect: bootsect.S
	$(AS) -o bootsect.o bootsect.S
	$(LD) $(LDFLAGS) -o bootsect bootsect.o

clean:
	rm -f *.o bootsect

