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

OBJS  = sched.o system_call.o traps.o asm.o fork.o panic.o printk.o vsprintf.o \
		sys.o mktime.o

kernel.o: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.o $(OBJS)
	sync

fork.s fork.o: fork.c

mktime.s mktime.o: mktime.c ../include/time.h

sched.o: sched.c

panic.o: panic.c ../include/linux/kernel.h ../include/linux/sched.h

printk.o: printk.c ../include/stdarg.h

vsprintf.o: vsprintf.c ../include/stdarg.h ../include/string.h

sys.o: sys.c

traps.o: traps.c

clean:
	rm -f *.o
	for i in blk_drv chr_drv; do make clean -C $$i; done

