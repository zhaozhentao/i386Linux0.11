#include <asm/system.h>
#include <linux/sched.h>

extern void rs1_interrupt(void);
extern void rs2_interrupt(void);

void rs_init(void) {
    set_intr_gate(0x24, rs1_interrupt);
    set_intr_gate(0x23, rs2_interrupt);
}

