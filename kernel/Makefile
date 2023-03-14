include ../Makefile.header

# 创建一个可重定位的目标文件
LDFLAGS	+= -r 

CFLAGS	+= -I../include

.c.s:
	$(CC) $(CFLAGS) -S -o $*.s $<
.s.o:
	$(AS) -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

OBJS  = sched.o 

kernel.o: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.o $(OBJS)

sched.s sched.o: sched.c

clean:
	rm -f *.o

