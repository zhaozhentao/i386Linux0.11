ROOT_DEV= #FLOPPY

all: Image	

Image: boot/bootsect
	tools/build.sh boot/bootsect Image $(ROOT_DEV)

boot/bootsect: boot/bootsect.s
	make bootsect -C boot

start: Image
	qemu-system-i386 -m 16M -boot a -fda Image -hda hdc-0.11.img -curses

debug: Image
	qemu-system-i386 -s -S -m 16M -boot a -fda Image -hda hdc-0.11.img -curses

stop:
	@kill -9 $$(ps -ef | grep qemu-system-i386 | awk '{print $$2}')

clean:
	rm -f Image
	for i in boot; do make clean -C $$i; done

