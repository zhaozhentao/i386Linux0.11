#include <linux/fs.h>
#include <linux/sched.h>

// 测试指定位的 0 1 值，并返回该比特位值
#define set_bit(bitnr,addr) ({ \
register int __res ; \
__asm__("bt %2,%3;setb %%al":"=a" (__res):"a" (0),"r" (bitnr),"m" (*(addr))); \
__res; })

struct super_block super_block[NR_SUPER];
// 在 init/main.c 中初始化
int ROOT_DEV = 0;

// 根据设备号寻找对应的超级块
struct super_block * get_super(int dev) {
    struct super_block * s;

    if (!dev)
        return NULL;
    s = 0+super_block;
    while (s < NR_SUPER+super_block)
        if (s->s_dev == dev) {
            // todo 暂时未实现超级块锁定相关代码
            // wait_on_super(s);
            if (s->s_dev == dev)
                return s;
            s = 0+super_block;
        } else
            s++;
    return NULL;
}

// 下面这个函数实现超级块的读取，目前不考虑竞争问题
static struct super_block * read_super(int dev) {
    struct super_block * s;
    struct buffer_head * bh;
    int i,block;

    // 没有指明设备号，直接返回
    if (!dev)
        return NULL;
    // 目前不需要检查磁盘是否更换
    // check_disk_change(dev);
    // 如果超级块已经在缓冲区，直接使用
    if ((s = get_super(dev)))
        return s;
    // 在超级块数组中找一个空项(s_dev == 0)，如果数组用完了返回 NULL
    for (s = 0+super_block ;; s++) {
        if (s >= NR_SUPER+super_block)
            return NULL;
        if (!s->s_dev)
            break;
    }
    // 找到超级块后对超级块的内存部分字段初始化，该超级块用于指定设备
    s->s_dev = dev;
    s->s_isup = NULL;
    s->s_imount = NULL;
    s->s_time = 0;
    s->s_rd_only = 0;
    s->s_dirt = 0;
    // lock_super(s);
    // 根据设备号读取设备中的超级块内容，如果读取失败就要释放超级块，注意
    // 如果是磁盘设备，起始扇区已经在 sys_setup 中被修改过了(hd[i+5*drive].start_sect = p->start_sect)。
    if (!(bh = bread(dev,1))) {
        s->s_dev=0;
        // free_super(s);
        return NULL;
    }
    // 把从设备中读到的超级块数据拷贝到刚刚获取到的超级块项中
    *((struct d_super_block *) s) =
        *((struct d_super_block *) bh->b_data);
    brelse(bh);
    // 检查文件系统版本是否 MINIX 1.0 的文件系统版本
    if (s->s_magic != SUPER_MAGIC) {
        s->s_dev = 0;
        // free_super(s);
        return NULL;
    }

    // 初始化 i 节点位图和逻辑块位图
    for (i=0;i<I_MAP_SLOTS;i++)
        s->s_imap[i] = NULL;
    for (i=0;i<Z_MAP_SLOTS;i++)
        s->s_zmap[i] = NULL;

    // 从设备中(磁盘)读取 i 节点位图信息和逻辑块信息
    block=2;
    for (i=0; i < s->s_imap_blocks; i++)
        if ((s->s_imap[i]=bread(dev,block)))
            block++;
        else
            break;
    for (i=0 ; i < s->s_zmap_blocks ; i++)
        if ((s->s_zmap[i]=bread(dev,block)))
            block++;
        else
            break;
    // 如果读出的位图逻辑块数不等于应该占有的逻辑块数，说明文件系统有问题,释放缓冲区返回 NULL
    if (block != 2+s->s_imap_blocks+s->s_zmap_blocks) {
        for(i=0;i<I_MAP_SLOTS;i++)
            brelse(s->s_imap[i]);
        for(i=0;i<Z_MAP_SLOTS;i++)
            brelse(s->s_zmap[i]);
        s->s_dev=0;
        // free_super(s);
        return NULL;
    }
    /* 一切成功，读取超级块完成， 0 号 i 节点和逻辑块是不能使用的，因
     * 为在申请可用 i 节点时，通过返回 0 表示没有空闲节点，将其置位*/
    s->s_imap[0]->b_data[0] |= 1;
    s->s_zmap[0]->b_data[0] |= 1;
    // free_super(s);
    return s;
}

void mount_root(void) {
    int i,free;
    struct super_block * p;
    struct m_inode * mi;

    // 需要保证 d_inode 结构为 32 字节才能对得上磁盘中的数据结构
    if (32 != sizeof (struct d_inode))
        panic("bad i-node size");
    // 初始化文件表数组，系统最多只能同时打开 64 个文件
    for(i=0;i<NR_FILE;i++)
        file_table[i].f_count=0;
    if (MAJOR(ROOT_DEV) == 2) {
        printk("Insert root floppy and press ENTER");
        //wait_for_keypress();
    }
    for(p = &super_block[0] ; p < &super_block[NR_SUPER] ; p++) {
        p->s_dev = 0;
        p->s_lock = 0;
        p->s_wait = NULL;
    }
    // 从磁盘读取超级块
    if (!(p=read_super(ROOT_DEV)))
        panic("Unable to mount root");
    if (!(mi=iget(ROOT_DEV,ROOT_INO)))
        panic("Unable to read root i-node");
    // inode 被引用了 4 次
    mi->i_count += 3 ;
    p->s_isup = p->s_imount = mi;
    current->pwd = mi;
    current->root = mi;
    // 下面开始读取位图中的每一位，看看有多少空闲块
    free=0;
    i=p->s_nzones;
    while (-- i >= 0)
        if (!set_bit(i&8191,p->s_zmap[i>>13]->b_data))
            free++;
    printk("%d/%d free blocks\n\r",free,p->s_nzones);
    // 下面统计有多少空闲的 inode
    free=0;
    i=p->s_ninodes+1;
    while (-- i >= 0)
        if (!set_bit(i&8191,p->s_imap[i>>13]->b_data))
            free++;
    printk("%d/%d free inodes\n\r",free,p->s_ninodes);
}

