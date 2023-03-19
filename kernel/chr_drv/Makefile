include ../../Makefile.header

CFLAGS	+= -I../../include
CPP	+= -I../../include

.c.s:
	@$(CC) $(CFLAGS) -S -o $*.s $<
.s.o:
	@$(AS) -o $*.o $<
.c.o:
	@$(CC) $(CFLAGS) -c -o $*.o $<

OBJS = tty_io.o console.o keyboard.o serial.o rs_io.o

chr_drv.a: $(OBJS)
	$(AR) rcs chr_drv.a $(OBJS)
	sync

keyboard.s: kb.S
	$(CPP) kb.S -o keyboard.s

serial.s serial.o: serial.c

clean:
	rm -f *.a *o

