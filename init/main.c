#include <linux/tty.h>

#include <linux/fs.h>

extern void blk_dev_init(void);
extern void chr_dev_init(void);
extern void mem_init(long start, long end);

/*
 * 这些参数是在 setup 模块里面存下来的
 */
#define EXT_MEM_K (*(unsigned short *)0x90002)             // 可以看到是 14M 内存
#define DRIVE_INFO (*(struct drive_info *)0x90080)
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)

static long memory_end = 0;
static long buffer_memory_end = 0;
static long main_memory_start = 0;

struct drive_info { char dummy[32]; } drive_info;

void main(void)
{
    ROOT_DEV = ORIG_ROOT_DEV;
    drive_info = DRIVE_INFO;
    memory_end = (1 << 20) + (EXT_MEM_K << 10);
    memory_end &= 0xfffff000;

    if (memory_end > 16*1024*1024)
        memory_end = 16*1024*1024;

    if (memory_end > 12*1024*1024) 
        buffer_memory_end = 4*1024*1024;
    else if (memory_end > 6*1024*1024)
        buffer_memory_end = 2*1024*1024;
    else
        buffer_memory_end = 1*1024*1024;

    main_memory_start = buffer_memory_end;

    mem_init(main_memory_start,memory_end);
    trap_init();
    blk_dev_init();
    chr_dev_init();
    tty_init();

    for(;;);
}

