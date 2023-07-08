#include <linux/sched.h>

#define clear_block(addr) \
__asm__ __volatile__ ("cld\n\t" \
    "rep\n\t" \
    "stosl" \
    ::"a" (0),"c" (BLOCK_SIZE/4),"D" ((long) (addr)))

// 把某一位置位
#define set_bit(nr,addr) ({\
register int res ; \
__asm__ __volatile__("btsl %2,%3\n\tsetb %%al": \
"=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \
res;})

// 从一个数据块 1024 字节中，找到第一个为 0 的位
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

// 向设备申请一个逻辑块
// 首先要在超级块中的逻辑块位图找到第一个0值比特位并将其置位表示占用，
// 然后为该数据块申请缓冲块，标记修改标志，最后返回逻辑块号
int new_block(int dev) {
    struct buffer_head * bh;
    struct super_block * sb;
    int i,j;
    // 获取设备超级块
    if (!(sb = get_super(dev)))
        panic("trying to get new block from nonexistant device");
    j = 8192;
    // 扫描 8 逻辑位图块，找到得一个0值比特位
    for (i=0 ; i<8 ; i++)
        if ((bh=sb->s_zmap[i]))
            if ((j=find_first_zero(bh->b_data))<8192)
                break;
    if (i>=8 || !bh || j>=8192)
        return 0;
    // 置位找到的比特位表示需要占用这个位对应的数据块
    if (set_bit(j,bh->b_data))
        panic("new_block: bit already set");
    // 设置更新标志，需要保存到设别
    bh->b_dirt = 1;
    // 计算逻辑块号
    j += i*8192 + sb->s_firstdatazone-1;
    if (j >= sb->s_nzones)
        return 0;
    if (!(bh=getblk(dev,j)))
        panic("new_block: cannot get block");
    if (bh->b_count != 1)
        panic("new block: count is != 1");
    // 清零缓冲区数据
    clear_block(bh->b_data);
    bh->b_uptodate = 1;
    bh->b_dirt = 1;
    brelse(bh);
    return j;
}

// 从内存中获取一个空闲的 inode ，并从 inode 位图中获取一个空闲 inode (磁盘中的)
struct m_inode * new_inode(int dev) {
    struct m_inode * inode;
    struct super_block * sb;
    struct buffer_head * bh;
    int i,j;

    // 从内存中获取一个空闲的 inode
    if (!(inode=get_empty_inode()))
        return NULL;

    // 获取设备的超级块
    if (!(sb = get_super(dev)))
        panic("new_inode with unknown device");

    // 8192 = 1024 * 8,8 代表一个字节 8 个位， 从每个 i 节点位图块中寻找第一个空闲的 inode
    j = 8192;
    for (i=0 ; i<8 ; i++)
        if ((bh=sb->s_imap[i]))
            if ((j=find_first_zero(bh->b_data))<8192)
                break;
    // 校验一下找到的 inode 位是否合法，j 不能大于每块的最大编号，j+i*8192 不能大于 inode 位图包含的 inode 总数
    if (!bh || j >= 8192 || j+i*8192 > sb->s_ninodes) {
        iput(inode);
        return NULL;
    }

    // 标记一下找到的位
    if (set_bit(j,bh->b_data))
        panic("new_inode: bit already set");
    // 因为修改了位图的某一位，所以需要把修改写入到磁盘中
    bh->b_dirt = 1;
    inode->i_count=1;
    inode->i_nlinks=1;
    inode->i_dev=dev;
    //inode->i_uid=current->euid;
    //inode->i_gid=current->egid;
    // 记录 inode 号，需要写入到磁盘中
    inode->i_dirt=1;
    inode->i_num = j + i*8192;
    inode->i_mtime = inode->i_atime = inode->i_ctime = 1;
    return inode;
}

