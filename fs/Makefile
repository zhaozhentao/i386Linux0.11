include ../Makefile.header

LDFLAGS += -r
CFLAGS += -I../include

.c.s:
	$(CC) $(CFLAGS) -S -o $*.s $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<
.s.o:
	$(AS) -o $*.o $<

OBJS = inode.o file_table.o buffer.o super.o namei.o \
	   bitmap.o

fs.o: $(OBJS)
	$(LD) $(LDFLAGS) -o fs.o $(OBJS)

clean:
	rm -f *.o

bitmap.o: bitmap.c

buffer.o: buffer.c

file_table.o: file_table.c

inode.o: inode.c

super.o: super.c

namei.o: namei.c

