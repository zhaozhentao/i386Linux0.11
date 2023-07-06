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
    if (!(block = dir->i_zone[0]))
        return NULL;
    if (!(bh = bread(dir->i_dev,block)))
        return NULL;
    i = 0;
    de = (struct dir_entry *) bh->b_data;
    while (1) {
        if ((char *)de >= BLOCK_SIZE+bh->b_data) {
            brelse(bh);
            bh = NULL;
            block = create_block(dir,i/DIR_ENTRIES_PER_BLOCK);
            if (!block)
                return NULL;
            if (!(bh = bread(dir->i_dev,block))) {
                i += DIR_ENTRIES_PER_BLOCK;
                continue;
            }
            de = (struct dir_entry *) bh->b_data;
        }
        if (i*sizeof(struct dir_entry) >= dir->i_size) {
            de->inode=0;
            dir->i_size = (i+1)*sizeof(struct dir_entry);
            dir->i_dirt = 1;
            // dir->i_ctime = CURRENT_TIME;
        }
        if (!de->inode) {
            //dir->i_mtime = CURRENT_TIME;
            for (i=0; i < NAME_LEN ; i++)
                de->name[i]=(i<namelen)?name[i]:0;
            bh->b_dirt = 1;
            *res_dir = de;
            return bh;
        }
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

    mode |= I_REGULAR;
    // 先取得文件所在目录，比如 /usr/root/whoami.c 要先取得 /usr/root 的 inode
    if (!(dir = dir_namei(pathname,&namelen,&basename)))
        return -ENOENT;

    // 先不考虑打开目录文件的情况
    bh = find_entry(&dir, basename, namelen, &de);
    // 如果 bh 不存在，可能是需要创建文件，先不处理
    if (!bh) {
        if (!(flag & O_CREAT)) {
            iput(dir);
            return -ENOENT;
        }

        // todo 因为目前还没有引入用户概念，先跳过权限检测
        inode = new_inode(dir->i_dev);
        if (!inode) {
            iput(dir);
            return -ENOSPC;
        }

        // inode->i_uid = current->euid;
        inode->i_mode = mode;
        inode->i_dirt = 1;
        bh = add_entry(dir,basename,namelen,&de);

        de->inode = inode->i_num;
        bh->b_dirt = 1;
        brelse(bh);
        iput(dir);
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

