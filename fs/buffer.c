#include <stdarg.h>

#include <linux/sched.h>

extern int end;  // 这个变量是由编译器添加的，是 bss 段结束后的第一个地址，表示内核程序的结束边界

struct buffer_head * start_buffer = (struct buffer_head *) &end; // 取内核程序的结束地址作为内核缓冲区的起始地址
struct buffer_head * hash_table[NR_HASH];                        // 内核使用 hash_table 管理内存，307 项
static struct buffer_head * free_list;                           // 空闲的内存链表
int NR_BUFFERS = 0;                                              // 用于统计缓冲块数量

#define _hashfn(dev,block) (((unsigned)(dev^block))%NR_HASH)
#define hash(dev,block) hash_table[_hashfn(dev,block)]

static inline void remove_from_queues(struct buffer_head * bh) {
    if (bh->b_next)
        bh->b_next->b_prev = bh->b_prev;
    if (bh->b_prev)
        bh->b_prev->b_next = bh->b_next;

    // 使 hash_table 原本指向 bh 的 item 指向新的项
    if (hash(bh->b_dev,bh->b_blocknr) == bh)
        hash(bh->b_dev,bh->b_blocknr) = bh->b_next;

    // 将 bh 从 free_list 中移除
    if (!(bh->b_prev_free) || !(bh->b_next_free))
        panic("Free block list corrupted");
    bh->b_prev_free->b_next_free = bh->b_next_free;
    bh->b_next_free->b_prev_free = bh->b_prev_free;
    if (free_list == bh)
        free_list = bh->b_next_free;
}

static inline void insert_into_queues(struct buffer_head * bh) {
    // 将 bh 放到 free_list 的末尾
    bh->b_next_free = free_list;
    bh->b_prev_free = free_list->b_prev_free;
    free_list->b_prev_free->b_next_free = bh;
    free_list->b_prev_free = bh;

    // 将 bh 放到新的 hash_table 槽中
    bh->b_prev = NULL;
    bh->b_next = NULL;
    if (!bh->b_dev)
        return;
    bh->b_next = hash(bh->b_dev,bh->b_blocknr);
    hash(bh->b_dev,bh->b_blocknr) = bh;
    bh->b_next->b_prev = bh;
}


static struct buffer_head * find_buffer(int dev, int block) {
    struct buffer_head * tmp;

    for (tmp = hash(dev,block) ; tmp != NULL ; tmp = tmp->b_next)
        if (tmp->b_dev==dev && tmp->b_blocknr==block)
            return tmp;

    return NULL;
}


struct buffer_head * get_hash_table(int dev, int block) {
    struct buffer_head * bh;

    for (;;) {
        if (!(bh=find_buffer(dev,block)))
            return NULL;

        bh->b_count++;

        if (bh->b_dev == dev && bh->b_blocknr == block)
            return bh;

        bh->b_count--;
    }
}

/* 这个宏用于同时判断修改标志和锁定标志，而且修改标志的权重大于锁定标志(因为 << 1)
   例如，修改过而且锁定状态时，值为 (1 << 1) + 1 = 2 + 1 */
#define BADNESS(bh) (((bh)->b_dirt<<1)+(bh)->b_lock)

struct buffer_head * getblk(int dev,int block) {
    struct buffer_head *tmp, *bh;

repeat:
    // 如果能从 hash table 中直接获取到 bh 就返回
    if ((bh = get_hash_table(dev,block)))
        return bh;

    tmp = free_list;

    do {
        // 如果 tmp 指向的缓冲头正在被引用就跳过，继续查找下一个缓冲头
        // 注意，没有被引用的缓冲头不一定就是干净的，因为如果缓冲头被一个进程写入数据后就释放了 (未同步到硬盘中)，这时 b_dirt 为 1
        // b_lock 位也有类似情况，当任务执行 b_reada 预读取几个块时，只要执行了 ll_rw_block ，b_count 就会递减，但硬盘操作可能还在进行 b_lock 未释放
        if (tmp->b_count)
            continue;

        if (!bh || BADNESS(tmp) < BADNESS(bh)) {
            bh = tmp;                                // bh 为空，或者 tmp 比 bh 更好 (BADNESS 更小)，将 bh 替换为 tmp
            if (!BADNESS(tmp))                       // 如果 tmp 指向的块没有 b_dirt 和 b_lock 说明可用，可以退出循环
                break;
        }
    } while ((tmp = tmp->b_next_free) != free_list); // 继续循环找下一个缓冲头

    // 目前我们只做简单的测试，所以经过上面的循环，是可以找到缓冲头的
    // 在完整的内核中，如果找不到缓冲头，说明缓冲区全部繁忙，进程就需要 sleep 以等待可用的缓冲区，我们暂时不实现这部分代码
    bh->b_count=1;
    bh->b_dirt=0;
    bh->b_uptodate=0;
    remove_from_queues(bh);
    bh->b_dev=dev;
    bh->b_blocknr=block;
    insert_into_queues(bh);
    return bh;
}

// 释放获取到的 buffer_head 
void brelse(struct buffer_head * buf) {
	if (!buf)
		return;

	if (!(buf->b_count--))
		panic("Trying to free free buffer");
}

void buffer_init(long buffer_end) {
    struct buffer_head * h = start_buffer;
    void * b;
    int i;

    if (buffer_end == 1<<20)
        b = (void *) (640*1024);
    else
        b = (void *) buffer_end;

    while ( (b -= BLOCK_SIZE) >= ((void *) (h+1)) ) {
        h->b_dev = 0;                                 // 使该缓冲区的设备号
        h->b_dirt = 0;                                // 脏标志，缓冲区是否被修改过
        h->b_count = 0;                               // 缓冲区引用计数
        h->b_lock = 0;                                // 缓冲区是否被锁
        h->b_uptodate = 0;                            // 缓冲区更新标志(缓冲区数据是否有效)
        h->b_wait = NULL;                             // 等待该缓冲区解锁的进程
        h->b_next = NULL;                             // 具有相同 hash 值的下一个缓冲头
        h->b_prev = NULL;                             // 具有相同 hash 值的前一个缓冲头
        h->b_data = (char *) b;                       // 缓冲区指向的具体数据块
        h->b_prev_free = h-1;                         // 链表的前一项
        h->b_next_free = h+1;                         // 链表的下一项
        h++;                                          // 使 h 指向下一个缓冲头
        NR_BUFFERS++;                                 // 缓冲区块数量增加
        if (b == (void *) 0x100000)
            b = (void *) 0xA0000;
    }

    h--;
    free_list = start_buffer;                         // 空闲链表指向第一个缓冲头块
    free_list->b_prev_free = h;                       // 使空闲链表的前一项指向最后一个缓冲头，形成双向环形链表
    h->b_next_free = free_list;                       // 最后一个缓冲头的下一项指向空闲链表的头，也是为了形成环形链表
    for (i=0;i<NR_HASH;i++)
        hash_table[i]=NULL;                           // 初始化 hash_table 全部指向 NULL
}

