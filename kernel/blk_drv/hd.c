#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <asm/io.h>
#include <asm/system.h>

#define MAJOR_NR 3

#include "blk.h"

#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

#define MAX_ERRORS	7
#define MAX_HD		2

// harddisk info struct 硬盘参数结构
// 磁头数, 每磁道扇区数，柱面数，写前预补偿柱面号，磁头着陆区柱面号，控制字节
struct hd_i_struct {
    int head, sect, cyl, wpcom, lzone, ctl;
};

static void recal_intr(void);

static int recalibrate = 0;              // 重新校正标志，程序会调用 recal_intr 把磁头移动到 0 柱面
static int reset = 0;                    // 复位标志

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

#define port_write(port,buf,nr) \
__asm__("cld;rep;outsw"::"d" (port),"S" (buf),"c" (nr))

extern void hd_interrupt(void);

void sys_setup(void * BIOS) {
    static int callable = 1;

    int i, drive;
    unsigned char cmos_disks;
    struct partition *p;
    struct buffer_head * bh;

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

    // 遍历每个硬盘，读取分区信息
    for (drive=0 ; drive<NR_HD ; drive++) {
        if (!(bh = bread(0x300 + drive*5,0))) {
            printk("Unable to read partition table of drive %d\n\r",
            drive);
            panic("");
        }
        // 0x55 0xaa 是分区标志
        if (bh->b_data[510] != 0x55 || (unsigned char)
            bh->b_data[511] != 0xAA) {
            printk("Bad partition table on drive %d\n\r",drive);
            panic("");
        }
        p = 0x1BE + (void *)bh->b_data;
        // 保存硬盘起始物理扇区号和扇区数量
        for (i=1;i<5;i++,p++) {
            hd[i+5*drive].start_sect = p->start_sect;
            hd[i+5*drive].nr_sects = p->nr_sects;
        }
        brelse(bh);
    }

    if (NR_HD)
        printk("Partition table%s ok.\n\r",(NR_HD>1)?"s":"");
}

// 判断并循环等待硬盘控制器就绪
static int controller_ready(void) {
    int retries=100000;

    while (--retries && (inb_p(HD_STATUS)&0x80));

    // 返回等待次数
    return (retries);
}

static int win_result(void) {
    int i=inb_p(HD_STATUS);

    if ((i & (BUSY_STAT | READY_STAT | WRERR_STAT | SEEK_STAT | ERR_STAT))
        == (READY_STAT | SEEK_STAT))
        return(0); /* ok */
    if (i&1) i=inb(HD_ERROR);
        return (1);
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

static int drive_busy(void) {
    unsigned int i;

    for (i = 0; i < 10000; i++)
        if (READY_STAT == (inb_p(HD_STATUS) & (BUSY_STAT|READY_STAT)))
            break;
    i = inb(HD_STATUS);
    i &= BUSY_STAT | READY_STAT | SEEK_STAT;
    if (i == (READY_STAT | SEEK_STAT))
        return(0);
    printk("HD controller times out\n\r");
    return(1);
}

static void reset_controller(void) {
    int	i;

    outb(4,HD_CMD);
    for(i = 0; i < 100; i++) nop();
        outb(hd_info[0].ctl & 0x0f ,HD_CMD);
    if (drive_busy())
        printk("HD-controller still busy\n\r");
    if ((i = inb(HD_ERROR)) != 1)
        printk("HD-controller reset failed: %02x\n\r",i);
}

static void reset_hd(int nr) {
    reset_controller();
    hd_out(nr,hd_info[nr].sect,hd_info[nr].sect,hd_info[nr].head-1,
        hd_info[nr].cyl,WIN_SPECIFY,&recal_intr);
}

void unexpected_hd_interrupt(void) {
    printk("Unexpected HD interrupt\n\r");
}

static void bad_rw_intr(void) {
    // 如果请求出错次数大于 7 结束请求
    if (++CURRENT->errors >= MAX_ERRORS)
        end_request(0);
    // 如果请求出错次数大于限定值，触发复位
    if (CURRENT->errors > MAX_ERRORS/2)
        reset = 1;
}

static void read_intr(void) {
    /*
     * 首先判断一下此次读操作有没有出错，若命令结束后控制器还出于忙状态，
     * 或者命令执行错误，则硬盘处理操作失败，请求硬盘复位并执行其他请求项，
     * 如果请求项累计出错超过 7 次，结束本次请求项，执行下一个请求项
     */
    if (win_result()) {
        bad_rw_intr();
        do_hd_request();
        return;
    }

    port_read(HD_DATA,CURRENT->buffer,256);
    CURRENT->errors = 0;
    CURRENT->buffer += 512;
    CURRENT->sector++;
    // 一个请求包含两个扇区，每次中断中获取一个扇区的数据 512 字节。
    if (--CURRENT->nr_sectors) {
        do_hd = &read_intr;
        return;
    }
    end_request(1);
    do_hd_request();
}

static void write_intr(void) {
    printk("write_intr called\n");
}

static void recal_intr(void) {
    if (win_result())
        bad_rw_intr();
    do_hd_request();
}

void do_hd_request(void) {
    int i, r = 0;
    unsigned int block, dev;      // 扇区，设备
    unsigned int sec, head, cyl;  // 磁道扇区号，磁头号，柱面
    unsigned int nsect;           // 需要读取的扇区数

    // 参数检查
    INIT_REQUEST;
    dev = MINOR(CURRENT->dev);    // 取出设备号
    block = CURRENT->sector;      // 取出要操作的扇区,一个块等于两个扇区
    if (dev >= 5*NR_HD || block+2 > hd[dev].nr_sects) {
        end_request(0);
        goto repeat;
    }
    block += hd[dev].start_sect;  // 加上起始扇区的偏移
    dev /= 5;                     // 此时 dev 代表硬盘号 0 号还是 1 号硬盘

    // 通过前面保存的硬盘信息，把要读取的块号转换为磁头号，柱面，磁道中的扇区号
    __asm__("divl %4":"=a" (block),"=d" (sec):"0" (block),"1" (0),
        "r" (hd_info[dev].sect));
    __asm__("divl %4":"=a" (cyl),"=d" (head):"0" (block),"1" (0),
        "r" (hd_info[dev].head));

    sec++;                       // 对计算出来的磁道扇区号进行调整
    nsect = CURRENT->nr_sectors; // 需要操作的扇区数量，一般每次操作 2 个扇区
    if (reset) {
        reset = 0;
        recalibrate = 1;
        reset_hd(CURRENT_DEV);
        return;
    }
    if (recalibrate) {
        recalibrate = 0;
        hd_out(dev,hd_info[CURRENT_DEV].sect,0,0,0,
            WIN_RESTORE,&recal_intr);
        return;
    }

    if (CURRENT->cmd == WRITE) {
        // 向硬盘控制器发出写命令
        hd_out(dev, nsect, sec, head, cyl, WIN_WRITE, &write_intr);

        for(i=0 ; i<3000 && !(r=inb_p(HD_STATUS)&DRQ_STAT) ; i++)
            /* 什么都不做，只是为了延时 */;

        if (!r) {
            printk("send write cmd failed\n");
        }

        port_write(HD_DATA, "write test data\n", 256);
    } else if (CURRENT->cmd == READ) {
        // 向硬盘控制器发出读命令
        hd_out(dev, nsect, sec, head, cyl, WIN_READ, &read_intr);
    }
}

void hd_init(void) {
    blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST; // 注册块设备
    set_intr_gate(0x2E, &hd_interrupt);          // 设置中断向量 int 0x2e
    outb_p(inb_p(0x21)&0xfb, 0x21);              // 复位 8259A int2 屏蔽位，允许从片发中断信号
    outb(inb_p(0xA1)&0xbf, 0xA1);                // 复位硬盘的中断请求屏蔽位，允许硬盘控制器发送中断请求信号
}

