include ../Makefile.header

CFLAGS	+= -I../include

.c.s:
	@$(CC) $(CFLAGS) -S -o $*.s $<
.s.o:
	@$(AS) -o $*.o $<
.c.o:
	@$(CC) $(CFLAGS) -c -o $*.o $<

OBJS  = ctype.o open.o close.o errno.o write.o dup.o string.o

lib.a: $(OBJS)
	$(AR) rcs lib.a $(OBJS)
	sync

string.o : string.c

close.s close.o : close.c

ctype.o : ctype.c ../include/ctype.h 

open.s open.o : open.c

dup.s dup.o : dup.c

errno.s errno.o : errno.c

write.s write.o : write.c

clean:
	rm -rf *.o *.a

