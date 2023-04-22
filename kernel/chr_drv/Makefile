include ../../Makefile.header

CFLAGS	+= -I../../include
CPP += -I../../include

.c.s:
	$(CC) $(CFLAGS) -S -o $*.s $<
.s.o:
	$(AS) -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

OBJS = tty_io.o console.o keyboard.o

chr_drv.a: $(OBJS)
	$(AR) rcs chr_drv.a $(OBJS)
	sync

keyboard.s: kb.S
	$(CPP) kb.S -o keyboard.s

clean:
	rm *.a *.o

