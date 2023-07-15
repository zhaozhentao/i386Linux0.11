#include <linux/sched.h>

#include <fcntl.h>
#include <errno.h>
#include <const.h>
#include <sys/stat.h>

extern struct m_inode * root;

static struct buffer_head * find_entry(struct m_inode ** dir,
    const char * name, int namelen, struct dir_entry ** res_dir) {
	int entries;
	int block,i;
	struct buffer_head * bh;
	struct dir_entry * de;
	struct super_block * sb;

    entries = (*dir)->i_size / (sizeof (struct dir_entry));
    *res_dir = NULL;
    if (!namelen)
        return NULL;

    if (!(block = (*dir)->i_zone[0]))
        return NULL;
    if (!(bh = bread((*dir)->i_dev,block)))
        return NULL;

    i = 0;
    // 把磁盘中的数据复制到 dir_entry 中
    de = (struct dir_entry *) bh->b_data;

    while (i < entries) {
        if ((char *)de >= BLOCK_SIZE+bh->b_data) {
            brelse(bh);
            bh = NULL;
        }

        // 通过文件名对比目录项
        if (!strncmp(de->name, name, namelen)) {
            *res_dir = de;
            return bh;
        }
        de++;
        i++;
    }
    brelse(bh);
    return NULL;
}

// 根据指定的目录和文件名添加目录项
static struct buffer_head * add_entry(struct m_inode * dir,
    const char * name, int namelen, struct dir_entry ** res_dir) {
    int block,i;
    struct buffer_head * bh;
    struct dir_entry * de;

    *res_dir = NULL;
#ifdef NO_TRUNCATE
    if (namelen > NAME_LEN)
        return NULL;
#else
    if (namelen > NAME_LEN)
        namelen = NAME_LEN;
#endif
    if (!namelen)
        return NULL;
    // 如果第一个检查保存数据块号的 i_zone 如果是 0 不正常就返回
    if (!(block = dir->i_zone[0]))
        return NULL;
    // 读取目录项数据块
    if (!(bh = bread(dir->i_dev,block)))
        return NULL;
    i = 0;
    de = (struct dir_entry *) bh->b_data;
    // 从目录项空间中找出第一个空闲的目录项
    while (1) {
        // 检查指向的目录项有没有超过范围
        if ((char *)de >= BLOCK_SIZE+bh->b_data) {
            brelse(bh);
            bh = NULL;
            // 创建新的块用来增加目录项
            block = create_block(dir,i/DIR_ENTRIES_PER_BLOCK);
            if (!block)
                return NULL;
            // 读取新的用于保存目录项的磁盘块，在新的数据块中找空的目录项
            if (!(bh = bread(dir->i_dev,block))) {
                i += DIR_ENTRIES_PER_BLOCK;
                continue;
            }
            de = (struct dir_entry *) bh->b_data;
        }
        // 如果下面的条件成立，说明目录没有发生因为删除目录项造成的空洞，新添加的目录项需要增加在最后
        if (i*sizeof(struct dir_entry) >= dir->i_size) {
            de->inode=0;
            // 追加在目录项数组的最后,所以长度要增加
            dir->i_size = (i+1)*sizeof(struct dir_entry);
            dir->i_dirt = 1;
            // dir->i_ctime = CURRENT_TIME;
        }
        // 找到一个空的目录项
        if (!de->inode) {
            //dir->i_mtime = CURRENT_TIME;
            // 把文件名复制到这个目录项
            for (i=0; i < NAME_LEN ; i++)
                de->name[i]=(i<namelen)?name[i]:0;
            // 表示需要写入到磁盘中
            bh->b_dirt = 1;
            // 返回可用的目录项
            *res_dir = de;
            return bh;
        }
        // 遍历下一个目录项
        de++;
        i++;
    }
    brelse(bh);
    return NULL;
}

// 根据文件路径获取文件所在目录 inode
static struct m_inode * get_dir(const char * pathname) {
    char c;
    const char * thisname;
    struct m_inode * inode;
    struct buffer_head * bh;
    int namelen, inr, idev;
    struct dir_entry * de;

    // 从根 inode 开始遍历
    inode = root;
    pathname++;

    while(1) {
        thisname = pathname;
        // todo 先跳过安全检测
        if (!S_ISDIR(inode->i_mode)) {
            iput(inode);
            return NULL;
        }
        // 通过指针扫描路径，遇到路径分隔符 / 就停止
        for(namelen=0;(c=pathname[0], pathname++, c)&&(c!='/');namelen++)
            /* nothing */ ;
        if (!c)
            return inode;

        // 从当前 inode (绝对路径从 / inode 开始)，搜索目录项，找到的目录项信息存放在 de
        if (!(bh = find_entry(&inode,thisname,namelen,&de))) {
            iput(inode);
            return NULL;
        }
        // 找到目录项，从该目录项中获取 inode 号
        inr = de->inode;
        idev = inode->i_dev;
        brelse(bh);
        iput(inode);
        // 根据刚刚获得的 inode 号去获取目录项的 inode
        if (!(inode = iget(idev,inr)))
            return NULL;
    }
}

static struct m_inode * dir_namei(const char * pathname,
    int * namelen, const char ** name) {
    char c;
    const char * basename;
    struct m_inode * dir;

    if (!(dir = get_dir(pathname)))
        return NULL;

    // 查找最后一个 / 后的名字字符串，并计算其长度
    basename = pathname;
    while (c=pathname[0], pathname++, c)
        if (c=='/')
            basename=pathname;
    *namelen = pathname-basename-1;
    *name = basename;

    return dir;
}

// namei for open 这里几乎是完整的 open 函数
// flag 是打开文件标志，可以取值 O_RDONLY(只读) O_WRONLY(只写) O_RDWR(读写) O_CREAT(创建) 等标志位
// 成功返回 0 
int open_namei(const char * pathname, int flag, int mode,
    struct m_inode ** res_inode) {
    const char * basename;
    int inr,dev,namelen;
    struct m_inode * dir, *inode;
    struct buffer_head * bh;
    struct dir_entry * de;

    // 添加普通文件的标志
    mode |= I_REGULAR;
    // 先取得文件所在目录，比如 /usr/root/whoami.c 要先取得 /usr/root 的 inode
    if (!(dir = dir_namei(pathname,&namelen,&basename)))
        return -ENOENT;

    // 先不考虑打开目录文件的情况
    bh = find_entry(&dir, basename, namelen, &de);
    // 如果 bh 不存在，可能是需要创建文件，检测一下打开的标志位
    if (!bh) {
        // 如果文件不存在，且创建文件标志没有置位就返回错误
        if (!(flag & O_CREAT)) {
            iput(dir);
            return -ENOENT;
        }

        // todo 因为目前还没有引入用户概念，先跳过权限检测

        // 创建一个新的 inode，用于指向即将要创建的文件
        inode = new_inode(dir->i_dev);
        if (!inode) {
            // 无法分配新的 inode ，返回磁盘没有空间的错误提示
            iput(dir);
            return -ENOSPC;
        }

        // 设置 inode 相应的标志位，i_dirt = 1 标记 inode 需要被写入到磁盘中
        // inode->i_uid = current->euid;
        inode->i_mode = mode;
        inode->i_dirt = 1;
        // 把新创建的 inode 添加到目录文件中，成为目录的一个目录项，这样文件就属于某一个目录了
        bh = add_entry(dir,basename,namelen,&de);

        // 因为目录文件也被更新了，所以目录文件也需要被写入到磁盘中 b_dirt = 1
        de->inode = inode->i_num;
        // 因为向目录文件中添加了目录项，被添加到 bh 中的目录项需要写入到磁盘中去
        bh->b_dirt = 1;
        brelse(bh);
        iput(dir);
        // 创建好的文件信息复制到传入的参数中返回给上一级
        *res_inode = inode;
        return 0;
    }

    // 文件 inode 号
    inr = de->inode;
    dev = dir->i_dev;
    brelse(bh);
    iput(dir);

    if (!(inode=iget(dev,inr)))
        return -EACCES;

    // 返回文件 inode
    *res_inode = inode;
    return 0;
}

// 删除文件
int sys_unlink(const char * name) {
    const char * basename;
    int namelen;
    struct m_inode * dir, * inode;
    struct buffer_head * bh;
    struct dir_entry * de;

    // 检查参数有效性，返回文件所在目录
    if (!(dir = dir_namei(name,&namelen,&basename)))
        return -ENOENT;
    // 如果文件名长度为 0 ,说明路径没有指定文件名，释放 dir 的 inode
    if (!namelen) {
        iput(dir);
        return -ENOENT;
    }

    // todo permission check

    // 查找目录项，返回目录项所在的缓冲区块 buffer_head
    bh = find_entry(&dir,basename,namelen,&de);
    if (!bh) {
        iput(dir);
        return -ENOENT;
    }
    // 根据目录项里面的 i 节点号获取文件的 inode
    if (!(inode = iget(dir->i_dev, de->inode))) {
        iput(dir);
        brelse(bh);
        return -ENOENT;
    }

    // todo permission check

    // unlink 只能删除文件，不能删除目录，如果是目录返回
    if (S_ISDIR(inode->i_mode)) {
        iput(inode);
        iput(dir);
        brelse(bh);
        return -EPERM;
    }
    if (!inode->i_nlinks) {
        printk("Deleting nonexistent file (%04x:%d), %d\n",
            inode->i_dev,inode->i_num,inode->i_nlinks);
        inode->i_nlinks=1;
    }
    // 设置目录项 i 节点号为 0 释放该目录项
    de->inode = 0;
    // 释放目录项后，目录项所在的缓冲区需要被写入到磁盘
    bh->b_dirt = 1;
    brelse(bh);
    // 当 i_nlinks 为 0 时，表示文件被删除了
    inode->i_nlinks--;
    inode->i_dirt = 1;
    // inode->i_ctime = CURRENT_TIME;
    iput(inode);
    iput(dir);
    return 0;
}

