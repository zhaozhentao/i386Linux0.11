all: Image

Image: boot/bootsect boot/setup
	tools/build.sh boot/bootsect boot/setup Image

boot/bootsect: boot/bootsect.s
	make bootsect -C boot

boot/setup: boot/setup.s
	make setup -C boot

start: Image
	qemu-system-i386 -m 16M -boot a -fda Image -curses

debug: Image
	qemu-system-i386 -s -S -m 16M -boot a -fda Image -curses

stop:
	@kill -9 $$(ps -ef | grep qemu-system-i386 | awk '{print $$2}')

clean:
	rm -f Image
	for i in boot; do make clean -C $$i; done

