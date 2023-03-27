
start:
	qemu-system-i386 -m 16M -boot a -fda boot/bootsect -curses

debug:
	qemu-system-i386 -s -S -m 16M -boot a -fda boot/bootsect -curses

