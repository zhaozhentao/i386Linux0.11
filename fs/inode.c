#include <linux/fs.h>

// 内存中的 i 节点表
struct m_inode inode_table[NR_INODE]={{0,},};

static void read_inode(struct m_inode * inode);
static void write_inode(struct m_inode * inode);

static inline void wait_on_inode(struct m_inode * inode) {

}

static inline void lock_inode(struct m_inode * inode) {
    inode->i_lock=1;
}

static inline void unlock_inode(struct m_inode * inode) {
    inode->i_lock=0;
}

// 获取一个空闲的 inode
struct m_inode * get_empty_inode(void) {
    struct m_inode * inode;
    static struct m_inode * last_inode = inode_table;
    int i;

    do {
        inode = NULL;
        for (i = NR_INODE; i ; i--) {
            if (++last_inode >= inode_table + NR_INODE)
                last_inode = inode_table;
            // 寻找引用计数为 0 的 inode
            if (!last_inode->i_count) {
                inode = last_inode;
                // 如果找到的 inode 没有脏数据而且没有被锁定可以结束搜索
                if (!inode->i_dirt && !inode->i_lock)
                    break;
            }
        }
        if (!inode) {
            for (i=0 ; i<NR_INODE ; i++)
                printk("%04x: %6d\t",inode_table[i].i_dev, inode_table[i].i_num);
            panic("No free inodes in mem");
        }
        wait_on_inode(inode);
        // 如果等待结束后有脏数据就要写入并等待，目前只实现节点的读取，所以不会有脏数据
        while (inode->i_dirt) {
            // write_inode(inode);
            wait_on_inode(inode);
        }
    } while (inode->i_count);
    memset(inode,0,sizeof(*inode));
    inode->i_count = 1;
    return inode;
}

// 把内存中的 inode 写入到磁盘中
void sync_inodes(void) {
    int i;
    struct m_inode * inode;

    inode = 0+inode_table;
    for(i=0 ; i<NR_INODE ; i++,inode++) {
        wait_on_inode(inode);
        // inode 有修改过，而且不是管道节点，写盘（实际上是写入到缓冲区, 实际上要等缓冲区的同步才是写入到磁盘中）
        if (inode->i_dirt && !inode->i_pipe)
            write_inode(inode);
    }
}

// inode 文件 i 节点指针，block 文件中的数据块号, create 创建标志
static int _bmap(struct m_inode * inode,int block,int create) {
    struct buffer_head * bh;
    int i;

    if (block<0)
        panic("_bmap: block<0");
    // 如果块号大于 直接块数 + 间接块数 + 二级间接块数超出了范围，停机
    if (block >= 7+512+512*512)
        panic("_bmap: block>big");
    // 小于 7 ，直接块号
    if (block<7) {
        // 置位了创建标志，而且i_zone[block]可用，i_zone[block] 指向创建的数据块
        if (create && !inode->i_zone[block])
            if ((inode->i_zone[block]=new_block(inode->i_dev))) {
                // inode->i_ctime=CURRENT_TIME;
                inode->i_dirt=1;
            }
        // 返回刚刚创建的数据块
        return inode->i_zone[block];
    }
    block -= 7;
    // 块号小于 7 + 512，说明之需使用一级间接
    if (block<512) {
        // 检查创建标志，i_zone[7] 未使用，用来保存一级间接块的地址
        if (create && !inode->i_zone[7])
            // 创建下一级数据块
            if ((inode->i_zone[7]=new_block(inode->i_dev))) {
                inode->i_dirt=1;
                //inode->i_ctime=CURRENT_TIME;
            }
        // 没有创建成功
        if (!inode->i_zone[7])
            return 0;
        // 获取下一级数据块的缓冲区
        if (!(bh = bread(inode->i_dev,inode->i_zone[7])))
            return 0;
        // 读取 block 指向的数据块
        i = ((unsigned short *) (bh->b_data))[block];
        // 还没分配数据块的时候创建数据块
        if (create && !i)
            if ((i=new_block(inode->i_dev))) {
                // 保存数据块号
                ((unsigned short *) (bh->b_data))[block]=i;
                bh->b_dirt=1;
            }
        brelse(bh);
        return i;
    }
    // 下面这些是二级间接块的处理
    block -= 512;
    // 检查指向二级间接块 inode->i_zone[8] 里面的指针
    if (create && !inode->i_zone[8])
        // 还未创建过二级间接块就创建
        if ((inode->i_zone[8]=new_block(inode->i_dev))) {
            inode->i_dirt=1;
            // inode->i_ctime=CURRENT_TIME;
        }
    // 如果到这里还是没有有效的二级间接块指针就失败
    if (!inode->i_zone[8])
        return 0;
    // 获取二级间接块的数据
    if (!(bh=bread(inode->i_dev,inode->i_zone[8])))
        return 0;
    // 下面的过程和上面的一级间接块是相似的，只是二级间接块里面保存的是下一个间接块的指针，要先获取下一个间接块，才能最终获取 block 指向的数据块
    i = ((unsigned short *)bh->b_data)[block>>9];
    if (create && !i)
        if ((i=new_block(inode->i_dev))) {
            ((unsigned short *) (bh->b_data))[block>>9]=i;
            bh->b_dirt=1;
        }
    brelse(bh);
    if (!i)
        return 0;
    if (!(bh=bread(inode->i_dev,i)))
        return 0;
    i = ((unsigned short *)bh->b_data)[block&511];
    if (create && !i)
        if ((i=new_block(inode->i_dev))) {
            ((unsigned short *) (bh->b_data))[block&511]=i;
            bh->b_dirt=1;
        }
    brelse(bh);
    return i;
}

int bmap(struct m_inode * inode,int block) {
    return _bmap(inode,block,0);
}

// 取文件数据块block 在设备上的逻辑块号, 如果对应的逻辑块不存在就创建
// 操作成功返回逻辑块好，否则返回 0
int create_block(struct m_inode * inode, int block) {
    return _bmap(inode,block,1);
}

// 释放一个节点，因为目前是读的操作，不需要写回设备
void iput(struct m_inode * inode) {
    if (!inode)
        return;
    wait_on_inode(inode);
    if (!inode->i_count)
        panic("iput: trying to free free inode");
    // 设备号为 0  引用直接减 1 返回
    if (!inode->i_dev) {
        inode->i_count--;
        return;
    }
    if (inode->i_count>1) {
        inode->i_count--;
        return;
    }
    inode->i_count--;
    return;
}

// 获取指定设备的 nr 号 inode
struct m_inode * iget(int dev,int nr) {
    struct m_inode * inode, * empty;

    if (!dev)
        panic("iget with dev==0");

    // 从 inode 表获取一个空闲 inode
    empty = get_empty_inode();
    // 从 inode 表内存中看看是否能找到 inode (说明加载过)
    inode = inode_table;
    while (inode < NR_INODE+inode_table) {
        // 寻找节点号的 inode
        if (inode->i_dev != dev || inode->i_num != nr) {
            inode++;
            continue;
        }

        // 如果找到指定的 inode 需要检查该节点是否被上锁
        wait_on_inode(inode);
        // 等待过程 i 节点可能会发生变化，应该要重新检查，如果发生了变化需要重新扫描
        if (inode->i_dev != dev || inode->i_num != nr) {
            inode = inode_table;
            continue;
        }
        // 找到 i 节点，引用计数增加
        inode->i_count++;

        // 检查一下 inode 是否另一个文件系统的安装点
        if (inode->i_mount) {
            int i;

            for (i = 0 ; i<NR_SUPER ; i++)
                // 存在于超级块表中
                if (super_block[i].s_imount==inode)
                    break;
            // 确实是其他文件系统的安装点，但不存在在超级块表中
            if (i >= NR_SUPER) {
                printk("Mounted inode hasn't got sb\n");
                if (empty)
                    iput(empty);
                return inode;
            }
            iput(inode);
            // 返回被安装(mount)的文件系统根节点
            dev = super_block[i].s_dev;
            nr = ROOT_INO;
            inode = inode_table;
            continue;
        }
        if (empty)
            iput(empty);
        return inode;
    }
    // 如果在 i 节点表没有找到指定的 i 节点，利用前面申请的空闲 i 节点
    if (!empty)
        return (NULL);
    inode=empty;
    inode->i_dev = dev;
    inode->i_num = nr;
    // 从磁盘中读出数据到 i 节点中
    read_inode(inode);
    return inode;
}

// 从设备上读取含有 i 节点信息的 i节点盘块，为了确定 i 节点所在的设备逻辑块号
// 需要先读取超级块，用来获取计算逻辑块每块 i 节点数信息 INODES_PER_BLOCK
static void read_inode(struct m_inode * inode) {
    struct super_block * sb;
    struct buffer_head * bh;
    int block;

    lock_inode(inode);
    // 获取 i 节点所在设备的超级块
    if (!(sb=get_super(inode->i_dev)))
        panic("trying to read inode without dev");
    // i 节点所在的逻辑块号 = (启动块 + 超级块) + i 节点位图块数 + 逻辑块图块数 + (i 节点号 - 1) / 每块包含的 i 节点数
    // 这是因为 0 号 i 节点是用来代表获取不到 i 节点，所以磁盘里是不保存 0 号 i 节点的
    block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
        (inode->i_num-1)/INODES_PER_BLOCK;
    // 把 i 节点所在的逻辑块整个读取出来
    if (!(bh=bread(inode->i_dev,block)))
        panic("unable to read i-node block");
    // 从刚刚读取到的整块数据中复制指定位置的数据到 i 节点结构
    *(struct d_inode *)inode =
        ((struct d_inode *)bh->b_data)
            [(inode->i_num-1)%INODES_PER_BLOCK];
    brelse(bh);
    unlock_inode(inode);
}

// 把 inode 信息写入到缓冲区，等缓冲区同步到磁盘中才真正把 inode 信息写到磁盘
static void write_inode(struct m_inode * inode) {
    struct super_block * sb;
    struct buffer_head * bh;
    int block;
    lock_inode(inode);
    if (!inode->i_dirt || !inode->i_dev) {
        unlock_inode(inode);
        return;
    }
    if (!(sb=get_super(inode->i_dev)))
        panic("trying to write inode without device");
    // 该 i 节点所在设备的逻辑块号 = (启动块 + 超级块) + i 节点位图占用块数 + 逻辑块位图占用块数 + (i 节点号 - 1) / 每块能存放的 i 节点数
    block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
        (inode->i_num-1)/INODES_PER_BLOCK;
    if (!(bh=bread(inode->i_dev,block)))
        panic("unable to read i-node block");
    // 将 i 节点信息复制到逻辑块缓冲区中(注意位置)
    ((struct d_inode *)bh->b_data)
        [(inode->i_num-1)%INODES_PER_BLOCK] =
            *(struct d_inode *)inode;
    bh->b_dirt=1;
    inode->i_dirt=0;
    brelse(bh);
    unlock_inode(inode);
}

