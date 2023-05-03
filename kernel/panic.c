#include <linux/kernel.h>
#include <linux/sched.h>

// 内核恐慌函数，内核发生错误，不再继续运行，进入死循环
void panic(const char * s) {
    printk("Kernel panic: %s\n\r",s);

    for(;;);
}

