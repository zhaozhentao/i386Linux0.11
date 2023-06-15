#include <linux/fs.h>

#define set_bit(bitnr,addr) ({ \
register int __res ; \
__asm__("bt %2,%3;setb %%al":"=a" (__res):"a" (0),"r" (bitnr),"m" (*(addr))); \
__res; })

struct super_block super_block[NR_SUPER];

int ROOT_DEV = 0;

struct super_block * get_super(int dev) {
    struct super_block * s;

    if (!dev)
        return NULL;
    s = 0+super_block;
    while (s < NR_SUPER+super_block)
        if (s->s_dev == dev) {
            if (s->s_dev == dev)
                return s;
            s = 0+super_block;
        } else
            s++;
    return NULL;
}

static struct super_block * read_super(int dev) {
    struct super_block * s;
    struct buffer_head * bh;
    int i,block;

    if (!dev)
        return NULL;
    if ((s = get_super(dev)))
        return s;
    for (s = 0+super_block ;; s++) {
        if (s >= NR_SUPER+super_block)
            return NULL;
        if (!s->s_dev)
            break;
    }
    s->s_dev = dev;
    s->s_isup = NULL;
    s->s_imount = NULL;
    s->s_time = 0;
    s->s_rd_only = 0;
    s->s_dirt = 0;
    if (!(bh = bread(dev,1))) {
        s->s_dev=0;
        return NULL;
    }
    *((struct d_super_block *) s) =
        *((struct d_super_block *) bh->b_data);
    brelse(bh);

    for (i=0;i<I_MAP_SLOTS;i++)
        s->s_imap[i] = NULL;
    for (i=0;i<Z_MAP_SLOTS;i++)
        s->s_zmap[i] = NULL;

    block=2;
    for (i=0 ; i < s->s_imap_blocks ; i++)
        if ((s->s_imap[i]=bread(dev,block)))
            block++;
        else
            break;
    for (i=0 ; i < s->s_zmap_blocks ; i++)
        if ((s->s_zmap[i]=bread(dev,block)))
            block++;
        else
            break;
    if (block != 2+s->s_imap_blocks+s->s_zmap_blocks) {
        for(i=0;i<I_MAP_SLOTS;i++)
            brelse(s->s_imap[i]);
        for(i=0;i<Z_MAP_SLOTS;i++)
            brelse(s->s_zmap[i]);
        s->s_dev=0;
        return NULL;
    }
    s->s_imap[0]->b_data[0] |= 1;
    s->s_zmap[0]->b_data[0] |= 1;

    return s;
}

void mount_root(void) {
    int i, free;
    struct super_block * p;
    struct m_inode * mi;

    if (32 != sizeof (struct d_inode))
        panic("bad i-node size");

    for(i=0;i<NR_FILE;i++)
        file_table[i].f_count=0;

    if (MAJOR(ROOT_DEV) == 2) {
        panic("Insert root floppy and press ENTER");
    }

    for(p = &super_block[0] ; p < &super_block[NR_SUPER] ; p++) {
        p->s_dev = 0;
        p->s_lock = 0;
        p->s_wait = NULL;
    }

    if (!(p=read_super(ROOT_DEV)))
        panic("Unable to mount root");

    free=0;
    i=p->s_nzones;
    while (-- i >= 0)
        if (!set_bit(i&8191,p->s_zmap[i>>13]->b_data))
            free++;
    printk("%d/%d free blocks\n\r",free,p->s_nzones);
    free=0;
    i=p->s_ninodes+1;
    while (-- i >= 0)
        if (!set_bit(i&8191,p->s_imap[i>>13]->b_data))
            free++;
    printk("%d/%d free inodes\n\r",free,p->s_ninodes);
}

