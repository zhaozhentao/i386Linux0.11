#ifndef _BLK_H
#define _BLK_H

// 系统中包含的块设备数量
#define NR_BLK_DEV	7

/*
 * 请求队列所包含的项数，写操作仅占前 2/3 ，读操作优先处理
 * Linus 认为 32 是一个比较合理的数字，大量的写或同步操作会引起长时间的暂停
 * */
#define NR_REQUEST	32

struct request {
    int dev;                            // 设备号，没有请求时为 -1
    int cmd;                            // 命令，可以是 READ 或 WRITE
    int errors;                         // 发生错误的次数
    unsigned long sector;               // 请求的起始扇区 1块=2扇区
    unsigned long nr_sectors;           // 请求的扇区数
    char * buffer;                      // 读或写操作的缓冲区
    struct request * next;              // 下一个请求
};

struct blk_dev_struct {
    void (*request_fn)(void);           // 块设备处理函数
    struct request * current_request;   // 当前请求
};

extern struct blk_dev_struct blk_dev[NR_BLK_DEV];
extern struct request request[NR_REQUEST];

#ifdef MAJOR_NR

#if (MAJOR_NR == 3)

#define DEVICE_INTR do_hd
#define DEVICE_REQUEST do_hd_request

#endif /* #ifdef (MAJOR_NR == 3) */

#endif /* #if (MAJOR_NR == 3) */

#endif /* #ifdef MAJOR_NR */

#ifdef DEVICE_INTR
void (*DEVICE_INTR)(void) = NULL;
#endif /* #ifdef DEVICE_INTR */


