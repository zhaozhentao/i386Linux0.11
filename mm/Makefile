include ../Makefile.header

LDFLAGS += -r
CFLAGS	+= -I../include
CPP	+= -I../include

.c.o:
	@$(CC) $(CFLAGS) \
	-c -o $*.o $<
.s.o:
	@$(AS) -o $*.o $<
.c.s:
	@$(CC) $(CFLAGS) \
	-S -o $*.s $<

OBJS = memory.o page.o

all: mm.o

mm.o: $(OBJS)
	$(LD) $(LDFLAGS) -o mm.o $(OBJS)

clean:
	rm -f core *.o *.a tmp_make
	for i in *.c;do rm -f `basename $$i .c`.s;done

memory.o: memory.c ../include/signal.h ../include/sys/types.h \
  ../include/asm/system.h ../include/linux/sched.h \
  ../include/linux/head.h ../include/linux/fs.h ../include/linux/mm.h \
  ../include/linux/kernel.h

