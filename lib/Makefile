include ../Makefile.header

CFLAGS	+= -I../include

OBJS = ctype.o string.o

.c.s:
	@$(CC) $(CFLAGS) -S -o $*.s $<
.s.o:
	@$(AS) -o $*.o $<
.c.o:
	@$(CC) $(CFLAGS) -c -o $*.o $<

lib.a: $(OBJS)
	$(AR) rcs lib.a $(OBJS)
	sync

string.s string.o : string.c ../include/string.h

clean:
	@rm -f core *.o *.a tmp_make

