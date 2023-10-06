include ../Makefile.header

CFLAGS	+= -I../include

.c.s:
	@$(CC) $(CFLAGS) -S -o $*.s $<
.s.o:
	@$(AS) -o $*.o $<
.c.o:
	@$(CC) $(CFLAGS) -c -o $*.o $<

OBJS  = ctype.o errno.o string.o

lib.a: $(OBJS)
	$(AR) rcs lib.a $(OBJS)
	sync

string.o : string.c

ctype.o : ctype.c ../include/ctype.h 

errno.s errno.o : errno.c

clean:
	rm -rf *.o *.a

