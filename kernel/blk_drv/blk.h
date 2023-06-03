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
    struct buffer_head * bh;            // request 指向的缓冲区头
    struct request * next;              // 下一个请求
};

#define IN_ORDER(s1,s2) \
((s1)->cmd<(s2)->cmd || ((s1)->cmd==(s2)->cmd && \
((s1)->dev < (s2)->dev || ((s1)->dev == (s2)->dev && \
(s1)->sector < (s2)->sector))))

struct blk_dev_struct {
    void (*request_fn)(void);           // 块设备处理函数
    struct request * current_request;   // 当前请求
};

extern struct blk_dev_struct blk_dev[NR_BLK_DEV];
extern struct request request[NR_REQUEST];

#ifdef MAJOR_NR

#if (MAJOR_NR == 3)

#define DEVICE_NAME "harddisk"
#define DEVICE_INTR do_hd
#define DEVICE_REQUEST do_hd_request
#define DEVICE_NR(device) (MINOR(device)/5)
#define DEVICE_OFF(device)

#endif /* #ifdef (MAJOR_NR == 3) */

#define CURRENT (blk_dev[MAJOR_NR].current_request)
#define CURRENT_DEV DEVICE_NR(CURRENT->dev)

#ifdef DEVICE_INTR
void (*DEVICE_INTR)(void) = NULL;
#endif /* #ifdef DEVICE_INTR */

static inline void unlock_buffer(struct buffer_head * bh)
{
	if (!bh->b_lock)
		printk(DEVICE_NAME ": free buffer being unlocked\n");
	bh->b_lock=0;
}

static inline void end_request(int uptodate)
{
	DEVICE_OFF(CURRENT->dev);
	if (CURRENT->bh) {
		CURRENT->bh->b_uptodate = uptodate;
		unlock_buffer(CURRENT->bh);
	}
	if (!uptodate) {
		printk(DEVICE_NAME " I/O error\n\r");
		printk("dev %04x, block %d\n\r",CURRENT->dev,
			CURRENT->bh->b_blocknr);
	}
	CURRENT->dev = -1;
	CURRENT = CURRENT->next;
}

#define INIT_REQUEST \
repeat: \
	if (!CURRENT) \
		return; \
	if (MAJOR(CURRENT->dev) != MAJOR_NR) \
		panic(DEVICE_NAME ": request list destroyed"); \
	if (CURRENT->bh) { \
		if (!CURRENT->bh->b_lock) \
			panic(DEVICE_NAME ": block not locked"); \
	}

#endif /* #ifdef MAJOR_NR */

#endif /* #ifndef _BLK_H */

