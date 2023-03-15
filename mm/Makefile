include ../Makefile.header

LDFLAGS += -r
CFLAGS	+= -I../include

.c.o:
	@$(CC) $(CFLAGS) -c -o $*.o $<
.s.o:
	@$(AS) -o $*.o $<
.c.s:
	@$(CC) $(CFLAGS) -S -o $*.s $<

OBJS = memory.o 

all: mm.o

mm.o: $(OBJS)
	@$(LD) $(LDFLAGS) -o mm.o $(OBJS)

clean:
	@rm -f *.o 

memory.o: memory.c

