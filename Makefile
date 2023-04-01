include Makefile.header

LDFLAGS	+= -Ttext 0 -e startup_32

all: Image

Image: boot/bootsect boot/setup tools/system
	cp -f tools/system system.tmp
	$(STRIP) system.tmp
	$(OBJCOPY) -O binary -R .note -R .comment system.tmp tools/kernel
	tools/build.sh boot/bootsect boot/setup tools/kernel Image

boot/bootsect: boot/bootsect.s
	make bootsect -C boot

boot/setup: boot/setup.s
	make setup -C boot

boot/head.o: boot/head.s
	make head.o -C boot

tools/system: boot/head.o
	$(LD) -g $(LDFLAGS) boot/head.o \
	-o tools/system

start: Image
	qemu-system-i386 -m 16M -boot a -fda Image -curses

debug: Image
	qemu-system-i386 -s -S -m 16M -boot a -fda Image -curses

stop:
	@kill -9 $$(ps -ef | grep qemu-system-i386 | awk '{print $$2}')

clean:
	rm -f Image
	for i in boot; do make clean -C $$i; done

