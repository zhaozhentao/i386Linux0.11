#include <asm/system.h>

#include <fcntl.h>

#include <linux/fs.h>
#include <sys/stat.h>

#define EXT_MEM_K (*(unsigned short *)0x90002)        // 1M 以后的扩展内存大小，也是在 setup.s 中设置的
#define DRIVE_INFO (*(struct drive_info *) 0x90080);  // 这个地址下的信息由 setup.s 设置,保存了 hd0 相关信息
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)    // 根文件系统所在设备号

struct drive_info { char dummy[32]; } drive_info;

extern void blk_dev_init(void);
extern void hd_init(void);
extern void con_init(void);

static long memory_end = 0;                      // 内存大小
static long buffer_memory_end = 0;               // 内核可用内存结束地址边界
static long main_memory_start = 0;               // 应用程序起始内存边界

void main(void) {
    struct stat stat;
    ROOT_DEV = ORIG_ROOT_DEV;
    drive_info = DRIVE_INFO;                         // 注意这里是值的复制，因为这个地址将来会被回收不再保存硬盘信息

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
    main_memory_start = buffer_memory_end;       // main_memory_start 就是将来用来运行应用程序的内存，以内核内存结束的地

    mem_init(main_memory_start, memory_end);
    con_init();
    trap_init();
    blk_dev_init();                                 // 初始化块设备结构
    sched_init();
    buffer_init(buffer_memory_end);
    hd_init();
    sti();

    move_to_user_mode();

    for (;;);
}
