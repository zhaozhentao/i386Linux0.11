#include <linux/sched.h>
#include <asm/system.h>
#include <asm/io.h>

// 8253 芯片输入时钟频率约为 1.193180 MHz，Linux 内核希望定时器产生中断的频率是 100Hz ，即每 10ms 产生一次
// 中断，这个是 8253 芯片的初始值
#define LATCH (1193180/HZ)

extern int timer_interrupt(void);

long user_stack [ PAGE_SIZE>>2 ] ;

// 这里 0x10 是内核段选择符
struct {
  long * a;
  short b;
} stack_start = { & user_stack [PAGE_SIZE>>2] , 0x10 };

void do_timer(long cpl) {
    printk("do_timer\n");
}

void sched_init(void) {
    // 下面代码用于初始化 8253 定时器，通道 0 ，工作模式 3 ，二进制计数方式，通道0 的输出
    // 引脚接在中断控制主芯片的 IRQ0 上，每 10ms 发出一个中断 LATCH 是初始定时器计数值
    outb_p(0x36, 0x43);                        // 二进制，mode 3 LSB/MSB channel 0
    outb_p(LATCH & 0xff, 0x40);                // LSB 低字节
    outb(LATCH >> 8, 0x40);                    // MSB 高字节
    set_intr_gate(0x20, &timer_interrupt);     // 注册定时器中断
    outb(inb_p(0x21)&~0x01,0x21);              // 允许时钟中断
}

