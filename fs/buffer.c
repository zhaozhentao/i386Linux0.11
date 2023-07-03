#include <linux/sched.h>

extern int end;  // 这个变量是由编译器添加的，是 bss 段结束后的第一个地址，表示内核程序的结束边界

struct buffer_head * start_buffer = (struct buffer_head *) &end; // 取内核程序的结束地址作为内核缓冲区的起始地址
struct buffer_head * hash_table[NR_HASH];                        // 内核使用 hash_table 管理内存，307 项
static struct buffer_head * free_list;                           // 空闲的内存链表
int NR_BUFFERS = 0;                                              // 用于统计缓冲块数量

static inline void wait_on_buffer(struct buffer_head * bh) {
    while (bh->b_lock);
}

int sync_dev(int dev) {
    int i;
    struct buffer_head * bh;

    bh = start_buffer;
    for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
        if (bh->b_dev != dev)
            continue;
        wait_on_buffer(bh);
        if (bh->b_dev == dev && bh->b_dirt)
            ll_rw_block(WRITE,bh);
    }
    sync_inodes();
    bh = start_buffer;
    for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
        if (bh->b_dev != dev)
            continue;
        wait_on_buffer(bh);
        if (bh->b_dev == dev && bh->b_dirt)
            ll_rw_block(WRITE,bh);
    }
    return 0;
}

/*
 * 关键字除留余数法，hash 函数包含 dev 和 block 两个关键字，对关键字 MOD 保证
 * 计算得出的结果在数组范围內
 */
#define _hashfn(dev,block) (((unsigned)(dev^block))%NR_HASH)
#define hash(dev,block) hash_table[_hashfn(dev,block)]

static inline void remove_from_queues(struct buffer_head * bh) {
    /* 把 buffer_head 从 hash 数组中移除 */
    if (bh->b_next)
        bh->b_next->b_prev = bh->b_prev;
    if (bh->b_prev)
        bh->b_prev->b_next = bh->b_next;
    if (hash(bh->b_dev,bh->b_blocknr) == bh)
        hash(bh->b_dev,bh->b_blocknr) = bh->b_next;

    if (!(bh->b_prev_free) || !(bh->b_next_free)) {
        printk("Free block list corrupted");
        for(;;);
    }

    /* 将buffer_head 从原来的空闲双向链表中移除 */
    bh->b_prev_free->b_next_free = bh->b_next_free;
    bh->b_next_free->b_prev_free = bh->b_prev_free;
    if (free_list == bh)
        free_list = bh->b_next_free;
}

static inline void insert_into_queues(struct buffer_head * bh) {
    /* 把 buffer_head 插入到空闲链表的末端 */
    bh->b_next_free = free_list;
    bh->b_prev_free = free_list->b_prev_free;
    free_list->b_prev_free->b_next_free = bh;
    free_list->b_prev_free = bh;

    /* 把 buffer_head 插入到 hash 数组的最前面 */
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

    // hash(dev,block) 计算出 dev, block 在 hash_table 中的槽，遍历该槽下的链表找出需要的 bh
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
    struct buffer_head * tmp, * bh;

    if ((bh = get_hash_table(dev,block)))
        return bh;

    // 指向空闲链表头
    tmp = free_list;

    do {
        // 如果 tmp 指向的缓冲头正在被引用就跳过，继续查找下一个缓冲头
        // 注意，没有被引用的缓冲头不一定就是干净的，因为如果缓冲头被一个进程写入数据后就释放了 (未同步到硬盘中)，这时 b_dirt 为 1
        // b_lock 位也有类似情况，当任务执行 b_reada 预读取几个块时，只要执行了 ll_rw_block ，b_count 就会递减，但硬盘操作可能还在进行 b_lock 未释放
        if (tmp->b_count)
            continue;
        if (!bh || BADNESS(tmp)<BADNESS(bh)) {
            bh = tmp;                            // bh 为空，或者 tmp 比 bh 更好 (BADNESS 更小)，将 bh 替换为 tmp
            if (!BADNESS(tmp))                   // 如果 tmp 指向的块没有 b_dirt 和 b_lock 说明可用，可以退出循环
                break;
        }
    /* 遍历链表直到找到可用的 buffer_head */
    } while ((tmp = tmp->b_next_free) != free_list);

    /* 找到可用的 buffer_head 后，需要设置一下引用计数
     * 同时将获取到的 buffer_head 移动到链表的末端
     */
    bh->b_count=1;
    bh->b_dirt=0;
    bh->b_uptodate=0;
    remove_from_queues(bh);
    bh->b_dev=dev;
    bh->b_blocknr=block;
    insert_into_queues(bh);
    return bh;
}

struct buffer_head * bread(int dev, int block) {
    struct buffer_head * bh;

    // 根据设备号获取 buffer_head
    if (!(bh = getblk(dev, block))) {
        panic("bread: getblk returned NULL\n");
    }

    // 如果buffer_head 中的数据已经是最新状态可以直接使用，减少真正的硬件访问。
    if (bh->b_uptodate)
        return bh;

    // low level read write 调用真正的硬件驱动程序
    ll_rw_block(READ, bh);

    // 等待硬件操作完成
    wait_on_buffer(bh);

    // 如果数据已经更新，说明硬件操作完成，返回 buffer_head
    if (bh->b_uptodate)
        return bh;

    // 否则释放缓冲区
    brelse(bh);
    return NULL;
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

void brelse(struct buffer_head* bh) {
    if (!bh)
        return;
    if (!(bh->b_count--))
        panic("Trying to free free buffer");
}

