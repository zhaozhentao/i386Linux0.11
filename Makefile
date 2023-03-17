include Makefile.header

LDFLAGS	+= -Ttext 0 -e startup_32
CFLAGS	+= -Iinclude

ROOT_DEV= #FLOPPY

ARCHIVES=kernel/kernel.o mm/mm.o fs/fs.o
MATH    =kernel/math/math.a
LIBS	=lib/lib.a

all: Image	

Image: boot/bootsect boot/setup tools/system
	cp -f tools/system system.tmp
	$(STRIP) system.tmp
	$(OBJCOPY) -O binary -R .note -R .comment system.tmp tools/kernel
	tools/build.sh boot/bootsect boot/setup tools/kernel Image $(ROOT_DEV)
	$(OBJDUMP) -D -m i386 tools/system > system.dis
	rm system.tmp
	rm -f tools/kernel

mm/mm.o:
	make -C mm

fs/fs.o:
	make -C fs

lib/lib.a:
	make -C lib

boot/bootsect: boot/bootsect.s
	make bootsect -C boot

boot/setup: boot/setup.s
	make setup -C boot

boot/head.o: boot/head.s
	make head.o -C boot

tools/system: boot/head.o init/main.o $(ARCHIVES) $(MATH) $(LIBS)
	$(LD) $(LDFLAGS) boot/head.o init/main.o \
	$(ARCHIVES) \
	$(MATH) \
	$(LIBS) \
	-o tools/system

kernel/math/math.a:
	make -C kernel/math

kernel/kernel.o: kernel/traps.c
	make -C kernel

start: Image
	qemu-system-i386 -m 16M -boot a -fda Image -hda hdc-0.11.img -curses

debug: Image
	qemu-system-i386 -s -S -m 16M -boot a -fda Image -hda hdc-0.11.img -curses

stop:
	@kill -9 $$(ps -ef | grep qemu-system-i386 | awk '{print $$2}')

clean:
	rm -f Image system.dis init/*.o
	for i in boot kernel lib mm; do make clean -C $$i; done

init/main.o: init/main.c

