ROOT_DEV= #FLOPPY

all: Image	

Image: boot/bootsect boot/setup
	tools/build.sh boot/bootsect boot/setup Image $(ROOT_DEV)

boot/bootsect: boot/bootsect.s
	make bootsect -C boot

boot/setup: boot/setup.s
	make setup -C boot

start: Image
	qemu-system-i386 -m 16M -boot a -fda Image -hda hdc-0.11.img -curses

debug: Image
	qemu-system-i386 -s -S -m 16M -boot a -fda Image -hda hdc-0.11.img -curses

stop:
	@kill -9 $$(ps -ef | grep qemu-system-i386 | awk '{print $$2}')

clean:
	rm -f Image
	for i in boot; do make clean -C $$i; done

