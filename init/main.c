#include <asm/system.h>
#include <linux/fs.h>

// 下面这些数据是 setup.s 中设置的
#define EXT_MEM_K (*(unsigned short *)0x90002)   // 1M 以后的扩展内存大小

extern void con_init(void);

static long memory_end = 0;                      // 内存大小
static long buffer_memory_end = 0;               // 内核可用内存结束地址边界
static long main_memory_start = 0;               // 应用程序起始内存边界

void main(void) {
    memory_end = (1<<20) + (EXT_MEM_K<<10);      // 内存大小 = 1M + 扩展内存
    memory_end &= 0xfffff000;                    // 忽略不到 4K 的内存
    if (memory_end > 16*1024*1024)
        memory_end = 16*1024*1024;               // 如果内存大于 16M 则只使用 16M

    if (memory_end > 12*1024*1024)
        buffer_memory_end = 4*1024*1024;         // 如果内存大于 12M ，则 4M 以下的内存归内核使用
    else if (memory_end > 6*1024*1024)
        buffer_memory_end = 2*1024*1024;         // 如果内存大于 6M 小于 12M ，则 2M 以下内存归内核使用
    else
        buffer_memory_end = 1*1024*1024;         // 如果内存小于 6M ，则 1M 以下的内存归内核使用
    main_memory_start = buffer_memory_end;       // main_memory_start 就是将来用来运行应用程序的内存，以内核内存结束的地址作为起始

    con_init();
    trap_init();
    sched_init();
    buffer_init(buffer_memory_end);
    sti();

    for (;;);
}
