include ../Makefile.header

LDFLAGS += -r
CFLAGS += -I../include

.c.s:
	$(CC) $(CFLAGS) -S -o $*.s $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<
.s.o:
	$(AS) -o $*.o $<

OBJS = open.o read_write.o inode.o file_table.o buffer.o super.o namei.o \
	   char_dev.o file_dev.o stat.o bitmap.o fcntl.o truncate.o

fs.o: $(OBJS)
	$(LD) $(LDFLAGS) -o fs.o $(OBJS)

clean:
	rm -f *.o

bitmap.o: bitmap.c

buffer.o: buffer.c

char_dev.o: char_dev.c

fcntl.o: fcntl.c

file_dev.o: file_dev.c

file_table.o: file_table.c

inode.o: inode.c

read_write.o: read_write.c

stat.o: stat.c

super.o: super.c

namei.o: namei.c

open.o: open.c

truncate.o: truncate.c

