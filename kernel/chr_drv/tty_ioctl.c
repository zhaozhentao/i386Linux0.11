#include <errno.h>
#include <termios.h>

#include <linux/tty.h>
#include <linux/sched.h>

#include <asm/segment.h>

int tty_ioctl(int dev, int cmd, int arg)
{
	struct tty_struct * tty;
	if (MAJOR(dev) == 5) {
		dev=current->tty;
		if (dev<0)
			panic("tty_ioctl: dev<0");
	} else
		dev=MINOR(dev);
	tty = dev + tty_table;
}

