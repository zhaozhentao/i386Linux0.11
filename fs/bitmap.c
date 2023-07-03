#include <linux/sched.h>

#define clear_block(addr) \
__asm__ __volatile__ ("cld\n\t" \
    "rep\n\t" \
    "stosl" \
    ::"a" (0),"c" (BLOCK_SIZE/4),"D" ((long) (addr)))

#define set_bit(nr,addr) ({\
register int res ; \
__asm__ __volatile__("btsl %2,%3\n\tsetb %%al": \
"=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \
res;})

#define find_first_zero(addr) ({ \
int __res; \
__asm__ __volatile__ ("cld\n" \
    "1:\tlodsl\n\t" \
    "notl %%eax\n\t" \
    "bsfl %%eax,%%edx\n\t" \
    "je 2f\n\t" \
    "addl %%edx,%%ecx\n\t" \
    "jmp 3f\n" \
    "2:\taddl $32,%%ecx\n\t" \
    "cmpl $8192,%%ecx\n\t" \
    "jl 1b\n" \
    "3:" \
    :"=c" (__res):"c" (0),"S" (addr)); \
__res;})

int new_block(int dev) {
    struct buffer_head * bh;
    struct super_block * sb;
    int i,j;
    if (!(sb = get_super(dev)))
        panic("trying to get new block from nonexistant device");
    j = 8192;
    for (i=0 ; i<8 ; i++)
        if ((bh=sb->s_zmap[i]))
            if ((j=find_first_zero(bh->b_data))<8192)
                break;
    if (i>=8 || !bh || j>=8192)
        return 0;
    if (set_bit(j,bh->b_data))
        panic("new_block: bit already set");
    bh->b_dirt = 1;
    j += i*8192 + sb->s_firstdatazone-1;
    if (j >= sb->s_nzones)
        return 0;
    if (!(bh=getblk(dev,j)))
        panic("new_block: cannot get block");
    if (bh->b_count != 1)
        panic("new block: count is != 1");
    clear_block(bh->b_data);
    bh->b_uptodate = 1;
    bh->b_dirt = 1;
    brelse(bh);
    return j;
}

struct m_inode * new_inode(int dev) {
    struct m_inode * inode;
    struct super_block * sb;
    struct buffer_head * bh;
    int i,j;

    if (!(inode=get_empty_inode()))
        return NULL;
    if (!(sb = get_super(dev)))
        panic("new_inode with unknown device");
    j = 8192;
    for (i=0 ; i<8 ; i++)
        if ((bh=sb->s_imap[i]))
            if ((j=find_first_zero(bh->b_data))<8192)
                break;
    if (!bh || j >= 8192 || j+i*8192 > sb->s_ninodes) {
        iput(inode);
        return NULL;
    }
    if (set_bit(j,bh->b_data))
        panic("new_inode: bit already set");
    bh->b_dirt = 1;
    inode->i_count=1;
    inode->i_nlinks=1;
    inode->i_dev=dev;
    //inode->i_uid=current->euid;
    //inode->i_gid=current->egid;
    inode->i_dirt=1;
    inode->i_num = j + i*8192;
    inode->i_mtime = inode->i_atime = inode->i_ctime = 1;
    return inode;
}

