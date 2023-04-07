include ../Makefile.header

# 创建一个可重定位的目标文件
LDFLAGS	+= -r

# 到 ../include 目录查找通过 <> 引入的头文件
CFLAGS	+= -I../include

.c.s:
	$(CC) $(CFLAGS) -S -o $*.s $<
.s.o:
	$(AS) -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

OBJS  = sched.o printk.o

kernel.o: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.o $(OBJS)
	sync

sched.o: sched.c

printk.o: printk.c

clean:
	rm -f *.o
