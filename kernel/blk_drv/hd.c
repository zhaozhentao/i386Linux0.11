#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <asm/io.h>
#include <asm/system.h>

#include "blk.h"

#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

#define MAX_HD		2

// harddisk info struct 硬盘参数结构
// 磁头数, 每磁道扇区数，柱面数，写前预补偿柱面号，磁头着陆区柱面号，控制字节
struct hd_i_struct {
    int head, sect, cyl, wpcom, lzone, ctl;
};

#ifdef HD_TYPE
struct hd_i_struct hd_info[] = { HD_TYPE };
#define NR_HD ((sizeof (hd_info))/(sizeof (struct hd_i_struct)))
#else
struct hd_i_struct hd_info[] = { {0,0,0,0,0,0},{0,0,0,0,0,0} };
static int NR_HD = 0;
#endif

// 定义硬盘的分区结构, 5 的倍数处为整个硬盘的参数，即 hd[0] hd[5]
static struct hd_struct {
    long start_sect;          // 分区在硬盘中的绝对扇区
    long nr_sects;            // 分区中总扇区数
} hd[5*MAX_HD]={{0,0},};

#define port_read(port,buf,nr) \
__asm__("cld;rep;insw"::"d" (port),"D" (buf),"c" (nr))

extern void hd_interrupt(void);

void sys_setup(void * BIOS) {
    static int callable = 1;

    int i, drive;
    unsigned char cmos_disks;

    // sys_setup 只会被调用一次
    if (!callable)
        return -1;
    callable = 0;

#ifndef HD_TYPE
    // 获取 setup.s 中保存的参数
    for (drive = 0; drive < 2; drive++) {
        hd_info[drive].cyl = *(unsigned short *) BIOS;        // 柱面数
        hd_info[drive].head = *(unsigned char *) (2+BIOS);    // 磁头数
        hd_info[drive].wpcom = *(unsigned short *) (5+BIOS);  // 写前预补偿柱面号
        hd_info[drive].ctl = *(unsigned char *) (8+BIOS);     // 控制字节
        hd_info[drive].lzone = *(unsigned short *) (12+BIOS); // 磁头着陆区柱面号
        hd_info[drive].sect = *(unsigned char *) (14+BIOS);   // 每磁道扇区数
        BIOS += 16;
    }

    // 计算硬盘个数
    if (hd_info[1].cyl)
        NR_HD=2;
    else
        NR_HD=1;
#endif

    // 这里仅设置代表两个硬盘的整体参数 hd[0] hd[5]
    for (i=0 ; i<NR_HD ; i++) {
        // 硬盘起始扇区
        hd[i*5].start_sect = 0;
        // 硬盘总扇区数
        hd[i*5].nr_sects = hd_info[i].head*hd_info[i].sect*hd_info[i].cyl;
    }

    // 从 CMOS 偏移地址为 0x12 处读取硬盘类型字节，如果低半字节不为 0 表示系统有两硬盘，否则只有 1 个硬盘，如果 0x12 处读出值为 0 表示系统没有 AT 控制器兼容硬盘
    if ((cmos_disks = CMOS_READ(0x12)) & 0xf0)
        if (cmos_disks & 0x0f)
            NR_HD = 2;
        else
            NR_HD = 1;
    else
        NR_HD = 0;

    // 如果 NR_HD 为 0 ，系统没有 AT 控制器兼容硬盘，两个硬盘信息结构清空，NR_HD = 1 则只有一个 AT 控制器兼容硬盘，清空第 2 个硬盘信息结构
    for (i = NR_HD ; i < 2 ; i++) {
        hd[i*5].start_sect = 0;
        hd[i*5].nr_sects = 0;
    }

    // 读取 0 号磁盘的 0 号 block
    hd_read(0 * 5, 0);
}

// 判断并循环等待硬盘控制器就绪
static int controller_ready(void) {
    int retries=100000;

    while (--retries && (inb_p(HD_STATUS)&0x80));

    // 返回等待次数
    return (retries);
}

// 等待硬盘控制器就绪后，向硬盘发送控制字节和 7 字节的参数命令块，硬盘完成命令后触发中断，中断入口在 kernel/sys_call.s 中
static void hd_out(unsigned int drive,unsigned int nsect,unsigned int sect,
		unsigned int head,unsigned int cyl,unsigned int cmd,
		void (*intr_addr)(void))
{
    register int port asm("dx");

    if (drive>1 || head>15)
        panic("Trying to write bad sector");
    if (!controller_ready())
        panic("HD controller not ready");
    do_hd = intr_addr;                       // 保存中断处理函数
    outb_p(hd_info[drive].ctl,HD_CMD);       // 向控制器输出控制字节
    // 向 0x1f0 ~ 0x1f7 发送 7 字节参数
    port=HD_DATA;                            // 设置 dx 为数据寄存器端口 0x1f0
    outb_p(hd_info[drive].wpcom>>2,++port);  // 写预补偿柱面号
    outb_p(nsect,++port);                    // 读写扇区总数
    outb_p(sect,++port);                     // 起始扇区
    outb_p(cyl,++port);                      // 柱面号低 8 位
    outb_p(cyl>>8,++port);                   // 柱面号高 8 位
    outb_p(0xA0|(drive<<4)|head,++port);     // 驱动器号 磁头号
    outb(cmd,++port);                        // 硬盘控制命令
}

void unexpected_hd_interrupt(void) {
    printk("Unexpected HD interrupt\n\r");
}

static void read_intr(void) {
    char buffer[512] = {};
    port_read(HD_DATA, buffer,256);

    printk("read: %s\n", buffer);

    return;
}

void hd_read(int dev, int b_block) {
    unsigned int block;           // 扇区
    unsigned int sec, head, cyl;  // 磁道扇区号，磁头号，柱面
    unsigned int nsect;           // 需要读取的扇区数

    block = b_block << 1;         // 将块号转换为扇区号，一个块等于两个扇区
    block += hd[dev].start_sect;  // 加上起始扇区的偏移

    dev /= 5;

    // 通过前面保存的硬盘信息，把要读取的块号转换为磁头号，柱面，磁道中的扇区号
    __asm__("divl %4":"=a" (block),"=d" (sec):"0" (block),"1" (0),
        "r" (hd_info[dev].sect));
    __asm__("divl %4":"=a" (cyl),"=d" (head):"0" (block),"1" (0),
        "r" (hd_info[dev].head));

    sec++;                       // 对计算出来的磁道扇区号进行调整
    nsect = 2;                   // 一个块等于两个扇区，所以要读取两个扇区

    hd_out(dev, nsect, sec, head, cyl, WIN_READ, &read_intr);
}

void hd_init(void) {
    set_intr_gate(0x2E, &hd_interrupt);          // 设置中断向量 int 0x2e
    outb_p(inb_p(0x21)&0xfb, 0x21);              // 复位 8259A int2 屏蔽位，允许从片发中断信号
    outb(inb_p(0xA1)&0xbf, 0xA1);                // 复位硬盘的中断请求屏蔽位，允许硬盘控制器发送中断请求信号
}

