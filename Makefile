ROOT_DEV= #FLOPPY

all: Image	

Image: boot/bootsect
	tools/build.sh boot/bootsect Image $(ROOT_DEV)

boot/bootsect: boot/bootsect.s
	make bootsect -C boot

clean:
	rm -f Image
	for i in boot; do make clean -C $$i; done

