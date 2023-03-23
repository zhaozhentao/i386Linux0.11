#include <linux/sched.h>
#include <asm/system.h>
#include <asm/io.h>

#define MAJOR_NR 2
#include "blk.h"

extern struct blk_dev_struct blk_dev[NR_BLK_DEV];

extern void floppy_interrupt(void);

void do_fd_request(void)
{

}

void floppy_init(void)
{
    blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
    set_trap_gate(0x26, &floppy_interrupt);
    outb(inb_p(0x21)&~0x40,0x21);
}

