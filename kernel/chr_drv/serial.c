#include <linux/tty.h>
#include <linux/sched.h>
#include <asm/system.h>
#include <asm/io.h>

extern void rs1_interrupt(void);
extern void rs2_interrupt(void);

static void init(int port) {
    outb_p(0x80, port + 3);         /* set DLAB of line control reg */
    outb_p(0x30, port);             /* LS of divisor (48 -> 2400 bps */
    outb_p(0x00, port + 1);         /* MS of divisor */
    outb_p(0x03, port + 3);         /* 复位 DLAB 数据位8 */
    outb_p(0x0b, port + 4);         /* 设置DTR RTS 辅助用户输出2  */
    outb_p(0x0d, port + 1);         /* 除了写以外，允许所有中断 */
    (void) inb(port);               /* read data port to reset things (?) */
}

void rs_init(void) {
    set_intr_gate(0x24, rs1_interrupt);
    set_intr_gate(0x23, rs2_interrupt);
    init(tty_table[1].read_q.data);
    init(tty_table[2].read_q.data);
    outb(inb_p(0x21)&0xE7,0x21);   /* 允许 8259A 响应IRQ3 IRQ4 中断 */
}

