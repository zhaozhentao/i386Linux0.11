include ../Makefile.header

LDFLAGS += -r
CFLAGS	+= -I../include

.c.s:
	@$(CC) $(CFLAGS) -S -o $*.s $<
.c.o:
	@$(CC) $(CFLAGS) -c -o $*.o $<
.s.o:
	$(AS) -o $*.o $<

OBJS= super.o

fs.o: $(OBJS)
	@$(LD) $(LDFLAGS) -o fs.o $(OBJS)

super.o: super.c

clean:
	rm -f *.o

