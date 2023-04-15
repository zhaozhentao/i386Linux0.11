include Makefile.header

LDFLAGS	+= -Ttext 0 -e startup_32
CFLAGS	+= -Iinclude

ARCHIVES=kernel/kernel.o
DRIVERS=kernel/chr_drv/chr_drv.a
LIBS	=lib/lib.a

.c.s:
	$(CC) $(CFLAGS) -S -o $*.s $<
.s.o:
	$(AS)  -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

all: Image

Image: boot/bootsect boot/setup tools/system
	cp -f tools/system system.tmp
	$(OBJCOPY) --only-keep-debug tools/system system.debug
	$(STRIP) system.tmp
	$(OBJCOPY) -O binary -R .note -R .comment system.tmp tools/kernel
	tools/build.sh boot/bootsect boot/setup tools/kernel Image
	$(OBJDUMP) -D -m i386 tools/system > system.dis

lib/lib.a:
	make -C lib

boot/bootsect: boot/bootsect.s
	make bootsect -C boot

boot/setup: boot/setup.s
	make setup -C boot

boot/head.o: boot/head.s
	make head.o -C boot

tools/system: boot/head.o init/main.o $(ARCHIVES) $(DRIVERS) $(LIBS)
	$(LD) -g $(LDFLAGS) boot/head.o init/main.o \
	$(ARCHIVES) \
	$(DRIVERS) \
	$(LIBS) \
	-o tools/system

kernel/chr_drv/chr_drv.a:
	make -C kernel/chr_drv

kernel/kernel.o: kernel/*.c
	make -C kernel

start: Image
	qemu-system-i386 -m 16M -boot a -fda Image -curses

debug: Image
	qemu-system-i386 -s -S -m 16M -boot a -fda Image -curses

stop:
	@kill -9 $$(ps -ef | grep qemu-system-i386 | awk '{print $$2}')

clean:
	rm -f Image *.dis *.tmp init/*.o system.debug System.map
	for i in boot kernel lib; do make clean -C $$i; done

init/main.o: init/main.c

