#include <asm/system.h>

#define DRIVE_INFO (*(struct drive_info *) 0x90080);  // 这个地址下的信息由 setup.s 设置,保存了 hd0 相关信息

struct drive_info { char dummy[32]; } drive_info;

extern void blk_dev_init(void);
extern void hd_init(void);
extern void con_init(void);

void main(void) {
    drive_info = DRIVE_INFO;                         // 注意这里是值的复制，因为这个地址将来会被回收不再保存硬盘信息

    con_init();
    trap_init();
    blk_dev_init();                                 // 初始化块设备结构
    sched_init();
    hd_init();
    sti();

    sys_setup((void *) &drive_info);
    for (;;);
}
