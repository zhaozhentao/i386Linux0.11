#include <linux/fs.h>

struct m_inode inode_table[NR_INODE]={{0,},};

static void read_inode(struct m_inode * inode);

void iput(struct m_inode * inode) {

}

struct m_inode * get_empty_inode(void) {
    struct m_inode * inode;
    static struct m_inode * last_inode = inode_table;
    int i;

    do {
        inode = NULL;
        for (i = NR_INODE; i ; i--) {
            if (++last_inode >= inode_table + NR_INODE)
                last_inode = inode_table;
            if (!last_inode->i_count) {
                inode = last_inode;
                if (!inode->i_dirt && !inode->i_lock)
                    break;
            }
        }
        if (!inode) {
            for (i=0 ; i<NR_INODE ; i++)
                printk("%04x: %6d\t",inode_table[i].i_dev,
            inode_table[i].i_num);
            panic("No free inodes in mem");
        }
        // wait_on_inode(inode);
        while (inode->i_dirt) {
            //write_inode(inode);
            //wait_on_inode(inode);
        }
    } while (inode->i_count);
    memset(inode,0,sizeof(*inode));
    inode->i_count = 1;
    return inode;
}

struct m_inode * iget(int dev,int nr) {
    struct m_inode * inode, * empty;

    if (!dev)
        panic("iget with dev==0");
    empty = get_empty_inode();
    inode = inode_table;
    while (inode < NR_INODE+inode_table) {
        if (inode->i_dev != dev || inode->i_num != nr) {
            inode++;
            continue;
        }
        // wait_on_inode(inode);
        if (inode->i_dev != dev || inode->i_num != nr) {
            inode = inode_table;
            continue;
        }
        inode->i_count++;
        if (inode->i_mount) {
            int i;
            for (i = 0 ; i<NR_SUPER ; i++)
                if (super_block[i].s_imount==inode)
                    break;
            if (i >= NR_SUPER) {
                printk("Mounted inode hasn't got sb\n");
                if (empty)
                    iput(empty);
                return inode;
            }
            iput(inode);
            dev = super_block[i].s_dev;
            nr = ROOT_INO;
            inode = inode_table;
            continue;
        }
    }
    if (!empty)
        return (NULL);
    inode=empty;
    inode->i_dev = dev;
    inode->i_num = nr;
    read_inode(inode);
    return inode;
}

static void read_inode(struct m_inode * inode) {
    struct super_block * sb;
    struct buffer_head * bh;
    int block;

    if (!(sb=get_super(inode->i_dev)))
        panic("trying to read inode without dev");
    block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
        (inode->i_num-1)/INODES_PER_BLOCK;
    if (!(bh=bread(inode->i_dev,block)))
        panic("unable to read i-node block");
    *(struct d_inode *)inode =
        ((struct d_inode *)bh->b_data)
            [(inode->i_num-1)%INODES_PER_BLOCK];
    brelse(bh);
}

