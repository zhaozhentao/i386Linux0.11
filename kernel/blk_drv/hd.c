#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <asm/system.h>
#include <asm/io.h>

#include "blk.h"

#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

#define MAX_HD		2

struct hd_i_struct {
    int head,sect,cyl,wpcom,lzone,ctl;
};
#ifdef HD_TYPE
struct hd_i_struct hd_info[] = { HD_TYPE };
#define NR_HD ((sizeof (hd_info))/(sizeof (struct hd_i_struct)))
#else
struct hd_i_struct hd_info[] = { {0,0,0,0,0,0},{0,0,0,0,0,0} };
static int NR_HD = 0;
#endif

static struct hd_struct {
    long start_sect;
    long nr_sects;
} hd[5*MAX_HD]={{0,0},};

#define port_read(port,buf,nr) \
__asm__("cld;rep;insw"::"d" (port),"D" (buf),"c" (nr))

extern void hd_interrupt(void);

void sys_setup(void * BIOS) {
    static int callable = 1;
    int i,drive;
    unsigned char cmos_disks;
    struct partition *p;
    struct buffer_head * bh;

    if (!callable)
        return -1;
    callable = 0;
#ifndef HD_TYPE
    for (drive=0 ; drive<2 ; drive++) {
        hd_info[drive].cyl = *(unsigned short *) BIOS;
        hd_info[drive].head = *(unsigned char *) (2+BIOS);
        hd_info[drive].wpcom = *(unsigned short *) (5+BIOS);
        hd_info[drive].ctl = *(unsigned char *) (8+BIOS);
        hd_info[drive].lzone = *(unsigned short *) (12+BIOS);
        hd_info[drive].sect = *(unsigned char *) (14+BIOS);
        BIOS += 16;
    }
    if (hd_info[1].cyl)
        NR_HD=2;
    else
        NR_HD=1;
#endif
    for (i=0 ; i<NR_HD ; i++) {
        hd[i*5].start_sect = 0;
        hd[i*5].nr_sects = hd_info[i].head * hd_info[i].sect * hd_info[i].cyl;
    }

    if ((cmos_disks = CMOS_READ(0x12)) & 0xf0)
        if (cmos_disks & 0x0f)
            NR_HD = 2;
        else
            NR_HD = 1;
    else
        NR_HD = 0;

    for (i = NR_HD ; i < 2 ; i++) {
        hd[i*5].start_sect = 0;
        hd[i*5].nr_sects = 0;
    }

    for (drive=0 ; drive<NR_HD ; drive++) {
        if (!(bh = bread(0x300 + drive*5,0))) {
            printk("Unable to read partition table of drive %d\n\r", drive);
            panic("");
        }

        if (bh->b_data[510] != 0x55 || (unsigned char) bh->b_data[511] != 0xAA) {
            printk("Bad partition table on drive %d\n\r",drive);
            panic("");
        }
    }
}

static int controller_ready(void)
{
    int retries=100000;

    while (--retries && (inb_p(HD_STATUS)&0x80));
    return (retries);
}

static void hd_out(unsigned int drive,unsigned int nsect,unsigned int sect,
		unsigned int head,unsigned int cyl,unsigned int cmd,
		void (*intr_addr)(void))
{
    register int port asm("dx");

    if (drive>1 || head>15)
        panic("Trying to write bad sector");
    if (!controller_ready())
        panic("HD controller not ready");
    do_hd = intr_addr;
    outb_p(hd_info[drive].ctl,HD_CMD);
    port=HD_DATA;
    outb_p(hd_info[drive].wpcom>>2,++port);
    outb_p(nsect,++port);
    outb_p(sect,++port);
    outb_p(cyl,++port);
    outb_p(cyl>>8,++port);
    outb_p(0xA0|(drive<<4)|head,++port);
    outb(cmd,++port);
}

void unexpected_hd_interrupt(void) {
    printk("Unexpected HD interrupt\n\r");
}

struct buffer_head * current; 

static void read_intr(void) {
    port_read(HD_DATA, current->b_data, 256);
    end_request(current);
}

struct buffer_head* do_hd_read_write(int rw, struct buffer_head * bh) {
    unsigned int dev;                           // 设备号
    unsigned int block;                         // 起始扇区
    unsigned int sec, head, cyl;
    unsigned int nsect;

    bh->b_lock = 1;
    dev = MINOR(bh->b_dev);
    block = bh->b_blocknr << 1;        // 设置起始扇区
    block += hd[dev].start_sect;
    dev /= 5;

    __asm__("divl %4":"=a" (block),"=d" (sec):"0" (block),"1" (0),
        "r" (hd_info[dev].sect));
    __asm__("divl %4":"=a" (cyl),"=d" (head):"0" (block),"1" (0),
        "r" (hd_info[dev].head));
    sec++;
    nsect = 2;                        // 因为一个 buffer_head 指向的缓冲区有 1k 所以可以放 2 个block 的数据

    current = bh;
    hd_out(dev,nsect,sec,head,cyl,WIN_READ,&read_intr);
}

void hd_init(void) {
    set_intr_gate(0x2E, &hd_interrupt);          // 设置中断向量 int 0x2e
    outb_p(inb_p(0x21)&0xfb, 0x21);              // 复位 8259A int2 屏蔽位，允许从片发中断信号
    outb(inb_p(0xA1)&0xbf, 0xA1);                // 复位硬盘的中断请求屏蔽位，允许硬盘控制器发送中断请求信号
}

