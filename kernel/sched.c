#include <linux/sched.h>
#include <linux/sys.h>
#include <asm/system.h>
#include <asm/io.h>

// 8253 芯片输入时钟频率约为 1.193180 MHz，Linux 内核希望定时器产生中断的频率是 100Hz ，即每 10ms 产生一次
// 中断，这个是 8253 芯片的初始值
#define LATCH (1193180/HZ)

extern int timer_interrupt(void);
extern int system_call(void);

union task_union {
    struct task_struct task;
    char stack[PAGE_SIZE];
};

// init 进程
static union task_union init_task = {INIT_TASK,};

long volatile jiffies=0;
long startup_time=0;
struct task_struct *current = &(init_task.task);
struct task_struct *last_task_used_math = NULL;

// 任务数组，最多 64 个任务
struct task_struct * task[NR_TASKS] = {&(init_task.task), };

long user_stack [ PAGE_SIZE>>2 ] ;

// 这里 0x10 是内核段选择符
struct {
  long * a;
  short b;
} stack_start = { & user_stack [PAGE_SIZE>>2] , 0x10 };

void schedule(void) {
    int i,next,c;
    struct task_struct ** p;

    // 从任务数组中找出 counter 最大的任务赋值到 next （运行时间还不长）
    while (1) {
        c = -1;
        next = 0;
        i = NR_TASKS;
        p = &task[NR_TASKS];
        while (--i) {
            if (!*--p)
                continue;
            if ((*p)->state == TASK_RUNNING && (*p)->counter > c)
                c = (*p)->counter, next = i;
        }
        // 如果找到 counter 不为 0 的任务，就可以退出循环，切换到该进程
        if (c) break;

        // 如果找不到可以运行的进程，则重置每个任务的 counter ,计算方式为 counter = counter / 2 + priority 
        // priority 为任务优先级
        for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
            if (*p)
                (*p)->counter = ((*p)->counter >> 1) + (*p)->priority;
    }

    // 切换到该进程
    switch_to(next);
}

void sleep_on(struct task_struct **p)
{
    struct task_struct *tmp;

    if (!p)
        return;
    if (current == &(init_task.task))
        panic("task[0] trying to sleep");
    tmp = *p;
    *p = current;
    current->state = TASK_UNINTERRUPTIBLE;
    schedule();
    if (tmp)
        tmp->state=0;
}

void do_timer(long cpl) {
    // 如果进程时间片还没用完，返回，否则置任务运行计数为0
    if ((--current->counter)>0) return;
    current->counter=0;
    // 如果定时中断发生在内核程序，不做处理，返回
    if (!cpl) return;
    // 触发调度
    schedule();
}

void sched_init(void) {
    int i;
    struct desc_struct * p;

    if (sizeof(struct sigaction) != 16)
        panic("Struct sigaction MUST be 16 bytes");

    // 把 init 进程的段描述符填写到 gdt 表中
    set_tss_desc(gdt+FIRST_TSS_ENTRY,&(init_task.task.tss));
    set_ldt_desc(gdt+FIRST_LDT_ENTRY,&(init_task.task.ldt));
    p = gdt+2+FIRST_TSS_ENTRY;
    // 清空任务数组和其在 gdt 表中的相应表项
    for(i=1;i<NR_TASKS;i++) {
        task[i] = NULL;
        p->a=p->b=0;
        p++;
        p->a=p->b=0;
        p++;
    }
    __asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");
    // 将任务 0 的 tss 段选择符加载到任务寄存器 tr ，将局部描述符表段选择符加载到局部描述符寄存器 ldtr 中,
    // 只明确加载这一次，以后新任务的 ldt 加载是 cpu 根据 tss 中 ldt 自动加载的
    ltr(0);
    lldt(0);
    // 下面代码用于初始化 8253 定时器，通道 0 ，工作模式 3 ，二进制计数方式，通道0 的输出
    // 引脚接在中断控制主芯片的 IRQ0 上，每 10ms 发出一个中断 LATCH 是初始定时器计数值
    outb_p(0x36, 0x43);                        // 二进制，mode 3 LSB/MSB channel 0
    outb_p(LATCH & 0xff, 0x40);                // LSB 低字节
    outb(LATCH >> 8, 0x40);                    // MSB 高字节
    set_intr_gate(0x20, &timer_interrupt);     // 注册定时器中断
    outb(inb_p(0x21)&~0x01,0x21);              // 允许时钟中断
    set_system_gate(0x80, &system_call);
}

