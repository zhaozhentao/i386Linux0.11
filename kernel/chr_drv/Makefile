include ../../Makefile.header

CFLAGS	+= -I../../include

.c.s:
	@$(CC) $(CFLAGS) -S -o $*.s $<
.s.o:
	@$(AS) -o $*.o $<
.c.o:
	@$(CC) $(CFLAGS) -c -o $*.o $<

OBJS = tty_io.o serial.o rs_io.o

chr_drv.a: $(OBJS)
	$(AR) rcs chr_drv.a $(OBJS)
	sync

serial.s serial.o: serial.c

clean:
	rm -f *.a *o

