#include <linux/fs.h>
#include <asm/system.h>

#include "blk.h"

struct request request[NR_REQUEST];

struct blk_dev_struct blk_dev[NR_BLK_DEV] = {
	{ NULL, NULL },		/* no_dev */
	{ NULL, NULL },		/* dev mem */
	{ NULL, NULL },		/* dev fd */
	{ NULL, NULL },		/* dev hd */
	{ NULL, NULL },		/* dev ttyx */
	{ NULL, NULL },		/* dev tty */
	{ NULL, NULL }		/* dev lp */
};

void blk_dev_init(void) {
    int i;

    // 初始化请求项
    for (i=0 ; i<NR_REQUEST ; i++) {
        request[i].dev = -1;
        request[i].next = NULL;
    }
}

static inline void lock_buffer(struct buffer_head * bh) {
    while (bh->b_lock) {
        // 如果 buffer 被锁，我们需要先等 buffer 解锁，目前没有实现调度相关，直接空循环等待
    }
    bh->b_lock=1;
}

static inline void unlock_buffer(struct buffer_head * bh) {
    if (!bh->b_lock)
        printk("ll_rw_block.c: buffer not locked\n\r");
    bh->b_lock = 0;
}

static void add_request(struct blk_dev_struct * dev, struct request * req) {
    struct request * tmp;

    req->next = NULL;
    cli();
    if (req->bh)
        req->bh->b_dirt = 0;
    // 检查一下设备当前有没有正在进行的请求,如果没有就可以发起真正的硬件操作
    if (!(tmp = dev->current_request)) {
        dev->current_request = req;
        sti();
        (dev->request_fn)();
        return;
    }

    // 这里是电梯调度算法，将请求放到适合的位置
    for ( ; tmp->next ; tmp=tmp->next)
        if ((IN_ORDER(tmp,req) ||
            !IN_ORDER(tmp,tmp->next)) &&
            IN_ORDER(req,tmp->next))
            break;
    req->next=tmp->next;
    tmp->next=req;
    sti();
}

static void make_request(int major,int rw, struct buffer_head * bh) {
    struct request * req;
    int rw_ahead;

    // 如果 buffer 已经被上锁，则放弃预读，否则就作为普通的读取
    if ((rw_ahead = (rw == READA || rw == WRITEA))) {
        if (bh->b_lock)
            return;
        if (rw == READA)
            rw = READ;
        else
            rw = WRITE;
    }

    // 如果既不是读取，也不是写，内核出错停机
    if (rw!=READ && rw!=WRITE)
        panic("Bad block dev command, must be R/W/RA/WA");

    lock_buffer(bh);
    /*
     * 如果是写请求但 buffer 但是自从上次写入后，数据没有更新过，
     * 或者是读请求，但数据上次读入过之后，没有发生过变化，数据和硬盘的一致，这两种请求都不需要发起请求
     */
    if ((rw == WRITE && !bh->b_dirt) || (rw == READ && bh->b_uptodate)) {
        unlock_buffer(bh);
        return;
    }

    req->dev = bh->b_dev;
    req->cmd = rw;
    req->errors=0;
    req->sector = bh->b_blocknr<<1;
    req->nr_sectors = 2;
    req->buffer = bh->b_data;
    req->bh = bh;
    req->next = NULL;
    add_request(major+blk_dev,req);
}

void ll_rw_block(int rw, struct buffer_head * bh) {
    unsigned int major;

    // 如果主设备号不存在或操作函数不存在就打印异常并返回，否则提交一个请求
    if ((major=MAJOR(bh->b_dev)) >= NR_BLK_DEV ||
        !(blk_dev[major].request_fn)) {
        printk("Trying to read nonexistent block-device\n\r");
        return;
    }
    make_request(major,rw,bh);
}

