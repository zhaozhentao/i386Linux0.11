#include <asm/system.h>
#include <linux/sched.h>

extern void rs1_interrupt(void);

void rs_init(void) {
    set_intr_gate(0x24,rs1_interrupt);
}

