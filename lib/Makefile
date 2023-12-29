include ../Makefile.header

CFLAGS	+= -I../include

.c.s:
	@$(CC) $(CFLAGS) -S -o $*.s $<
.s.o:
	@$(AS) -o $*.o $<
.c.o:
	@$(CC) $(CFLAGS) -c -o $*.o $<

OBJS  = ctype.o _exit.o open.o close.o errno.o write.o dup.o setsid.o string.o \
		execve.o wait.o sys_call.o

lib.a: $(OBJS)
	$(AR) rcs lib.a $(OBJS)
	sync

_exit.s _exit.o : _exit.c

string.o : string.c

wait.s wait.o : wait.c

close.s close.o : close.c

ctype.o : ctype.c ../include/ctype.h 

open.s open.o : open.c

setsid.s setsid.o : setsid.c

dup.s dup.o : dup.c

errno.s errno.o : errno.c

execve.s execve.o : execve.c

write.s write.o : write.c

sys_call.o : sys_call.c

clean:
	rm -rf *.o *.a

