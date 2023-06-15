include ../Makefile.header

LDFLAGS += -r
CFLAGS += -I../include

.c.s:
	$(CC) $(CFLAGS) -S -o $*.s $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<
.s.o:
	$(AS) -o $*.o $<

OBJS = file_table.o buffer.o super.o

fs.o: $(OBJS)
	$(LD) $(LDFLAGS) -o fs.o $(OBJS)

clean:
	rm -f *.o

buffer.o: buffer.c

file_table.o: file_table.c

super.o: super.c

