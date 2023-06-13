#ifndef _HDREG_H
#define _HDREG_H

#define HD_DATA    0x1f0    /* _CTL when writing */
#define HD_ERROR   0x1f1    /* see err-bits */
#define HD_STATUS  0x1f7

#define ERR_STAT	0x01
#define INDEX_STAT	0x02
#define ECC_STAT	0x04	/* Corrected error */
#define DRQ_STAT	0x08
#define SEEK_STAT	0x10
#define WRERR_STAT	0x20
#define READY_STAT	0x40
#define BUSY_STAT	0x80

#define DRQ_STAT   0x08

#define HD_CMD     0x3f6
#define WIN_RESTORE		0x10
#define WIN_READ   0x20
#define WIN_WRITE  0x30
#define WIN_SPECIFY		0x91

struct partition {
  unsigned char boot_ind;    /* 引导标志，4 个分区中只有一个是引导分区 0 表示非引导分区，0x80 表示引导分区 */
  unsigned char head;        /* 分区起始磁头号 */
  unsigned char sector;      /* 分区起始扇区号(0 ~ 5位)，起始柱面号高2位(6 ~ 7位) */
  unsigned char cyl;         /* 分区起始柱面号低8位(8位) */
  unsigned char sys_ind;     /* 分区类型字节 */
  unsigned char end_head;    /* 分区结束磁头号  */
  unsigned char end_sector;  /* 结束扇区号(0~5位) 结束柱面号高2位(6~7位) */
  unsigned char end_cyl;     /* 结束柱面号低8位 */
  unsigned int start_sect;   /* 分区起始物理扇区号 */
  unsigned int nr_sects;     /* 分区占用扇区数  */
};

#endif

