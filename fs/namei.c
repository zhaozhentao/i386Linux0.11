#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

#include <fcntl.h>
#include <errno.h>
#include <const.h>
#include <sys/stat.h>

#define ACC_MODE(x) ("\004\002\006\377"[(x)&O_ACCMODE])

#define MAY_WRITE 2

static int permission(struct m_inode * inode,int mask)
{
    int mode = inode->i_mode;

    /* special case: not even root can read/write a deleted file */
    if (inode->i_dev && !inode->i_nlinks)
        return 0;
    else if (current->euid==inode->i_uid)
        mode >>= 6;
    else if (current->egid==inode->i_gid)
        mode >>= 3;
    if (((mode & mask & 0007) == mask) || suser())
        return 1;
    return 0;
}

extern struct m_inode * root;

static int match(int len,const char * name,struct dir_entry * de)
{
	register int same ;

	if (!de || !de->inode || len > NAME_LEN)
		return 0;
	if (len < NAME_LEN && de->name[len])
		return 0;
	__asm__("cld\n\t"
		"fs ; repe ; cmpsb\n\t"
		"setz %%al"
		:"=a" (same)
		:"0" (0),"S" ((long) name),"D" ((long) de->name),"c" (len)
		);
	return same;
}

// 从目录中查找指定的目录项，返回目录项所在的缓冲区
static struct buffer_head * find_entry(struct m_inode ** dir,
    const char * name, int namelen, struct dir_entry ** res_dir) {
	int entries;
	int block,i;
	struct buffer_head * bh;
	struct dir_entry * de;
	struct super_block * sb;

#ifdef NO_TRUNCATE
    if (namelen > NAME_LEN)
        return NULL;
#else
    if (namelen > NAME_LEN)
        namelen = NAME_LEN;
#endif
    entries = (*dir)->i_size / (sizeof (struct dir_entry));
    *res_dir = NULL;
    if (!namelen)
        return NULL;

/* check for '..', as we might have to do some "magic" for it */
    if (namelen==2 && get_fs_byte(name)=='.' && get_fs_byte(name+1)=='.') {
/* '..' in a pseudo-root results in a faked '.' (just change namelen) */
        if ((*dir) == current->root)
            namelen=1;
        else if ((*dir)->i_num == ROOT_INO) {
/* '..' over a mount-point results in 'dir' being exchanged for the mounted
directory-inode. NOTE! We set mounted, so that we can iput the new dir */
            sb=get_super((*dir)->i_dev);
            if (sb->s_imount) {
                iput(*dir);
                (*dir)=sb->s_imount;
                (*dir)->i_count++;
            }
        }
    }

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
            if (!(block = bmap(*dir,i/DIR_ENTRIES_PER_BLOCK)) ||
                !(bh = bread((*dir)->i_dev,block))) {
                i += DIR_ENTRIES_PER_BLOCK;
                continue;
            }
            de = (struct dir_entry *) bh->b_data;
        }
        // 通过文件名对比目录项
        if (match(namelen,name,de)) {
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
    inode = iget(ROOT_DEV,ROOT_INO);
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
    while ((c=get_fs_byte(pathname++)))
        if (c=='/')
            basename=pathname;
    *namelen = pathname-basename-1;
    *name = basename;

    return dir;
}

// 通过路径获取文件 inode，对于很多其他的系统调用可以用这个函数来获取文件 inode
struct m_inode * namei(const char * pathname) {
    const char * basename;
    int inr,dev,namelen;
    struct m_inode * dir;
    struct buffer_head * bh;
    struct dir_entry * de;

    // 获取文件父目录
    if (!(dir = dir_namei(pathname,&namelen,&basename)))
        return NULL;

    // 检查路径中有没有给出文件名(最后是 / 结尾就是没有给出有效的文件名)
    if (!namelen)
        return dir;

    // 查找文件目录项
    bh = find_entry(&dir,basename,namelen,&de);
    if (!bh) {
        iput(dir);
        return NULL;
    }
    // 文件的 inode 号
    inr = de->inode;
    // 文件所在的设备号
    dev = dir->i_dev;
    brelse(bh);
    iput(dir);
    dir=iget(dev,inr);
    // todo 更新 inode 最后访问时间
    if (dir) {
        // dir->i_atime=CURRENT_TIME;
        dir->i_dirt=1;
    }
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

    if ((flag & O_TRUNC) && !(flag & O_ACCMODE))
        flag |= O_WRONLY;
    mode &= 0777 & ~current->umask;
    // 添加普通文件的标志
    mode |= I_REGULAR;
    // 先取得文件所在目录，比如 /usr/root/whoami.c 要先取得 /usr/root 的 inode
    if (!(dir = dir_namei(pathname,&namelen,&basename)))
        return -ENOENT;

    if (!namelen) {			/* special case: '/usr/' etc */
        if (!(flag & (O_ACCMODE|O_CREAT|O_TRUNC))) {
            *res_inode=dir;
            return 0;
        }
        iput(dir);
        return -EISDIR;
    }

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
        if (!permission(dir,MAY_WRITE)) {
            iput(dir);
            return -EACCES;
        }

        // 创建一个新的 inode，用于指向即将要创建的文件
        inode = new_inode(dir->i_dev);
        if (!inode) {
            // 无法分配新的 inode ，返回磁盘没有空间的错误提示
            iput(dir);
            return -ENOSPC;
        }

        // 设置 inode 相应的标志位，i_dirt = 1 标记 inode 需要被写入到磁盘中
        inode->i_uid = current->euid;
        inode->i_mode = mode;
        inode->i_dirt = 1;
        // 把新创建的 inode 添加到目录文件中，成为目录的一个目录项，这样文件就属于某一个目录了
        bh = add_entry(dir,basename,namelen,&de);
        if (!bh) {
            inode->i_nlinks--;
            iput(inode);
            iput(dir);
            return -ENOSPC;
        }
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
    if (flag & O_EXCL)
        return -EEXIST;
    if (!(inode=iget(dev,inr)))
        return -EACCES;
    if ((S_ISDIR(inode->i_mode) && (flag & O_ACCMODE)) ||
        !permission(inode,ACC_MODE(flag))) {
        iput(inode);
        return -EPERM;
    }
    // inode->i_atime = CURRENT_TIME;
    // 如果打开文件时设置了截断标志且是写操作就截断文件
    if (flag & O_TRUNC)
        truncate(inode);
    // 返回文件 inode
    *res_inode = inode;
    return 0;
}

// 创建目录文件，参数为目录路径如 /usr/root/mydir，其中 mode 参数定义在 include/sys/stat.h 中
int sys_mkdir(const char * pathname, int mode) {
    const char * basename;
    int namelen;
    struct m_inode * dir, * inode;
    struct buffer_head * bh, *dir_block;
    struct dir_entry * de;

    // todo permission check
    // 检查参数的有效性，取出需要被创建目录的父目录
    if (!(dir = dir_namei(pathname,&namelen,&basename)))
        return -ENOENT;

    // 如果 namelen 为 0 说明没有指定需要被创建的目录名，释放资源并返回
    if (!namelen) {
        iput(dir);
        return -ENOENT;
    }

    // 检查一下需要被创建的目录是否已经存在
    bh = find_entry(&dir,basename,namelen,&de);
    if (bh) {
        brelse(bh);
        iput(dir);
        return -EEXIST;
    }
    // 为即将创建的目录文件分配一个 inode
    inode = new_inode(dir->i_dev);
    if (!inode) {
        iput(dir);
        return -ENOSPC;
    }
    // 设置 inode 对应的文件长度为 32 (两个目录项的大小，即 . 和 .. 目录项)
    inode->i_size = 32;
    inode->i_dirt = 1;
    // inode->i_mtime = inode->i_atime = CURRENT_TIME;
    // 为 inode 分配保存目录项的磁盘块，第一个直接块指向这个数据块
    if (!(inode->i_zone[0]=new_block(inode->i_dev))) {
        iput(dir);
        inode->i_nlinks--;
        iput(inode);
        return -ENOSPC;
    }
    inode->i_dirt = 1;
    // 读取一下刚刚分配的磁盘块，出错就释放这个数据块
    if (!(dir_block=bread(inode->i_dev,inode->i_zone[0]))) {
        iput(dir);
        free_block(inode->i_dev,inode->i_zone[0]);
        inode->i_nlinks--;
        iput(inode);
        return -ERROR;
    }
    // 如果没有出错就为这个目录文件添加两个目录项 . 和 ..
    de = (struct dir_entry *) dir_block->b_data;
    de->inode=inode->i_num;
    strcpy(de->name,".");
    de++;
    de->inode = dir->i_num;
    strcpy(de->name,"..");
    // inode 引用计数为 2 包括，比如对于一个绝对路径 /usr/root/mydir 就包括了绝对路径本身和 . 两个引用
    inode->i_nlinks = 2;
    dir_block->b_dirt = 1;
    brelse(dir_block);
    // inode->i_mode = I_DIRECTORY | (mode & 0777 & ~current->umask);
    // 设置目录文件的属性，包括文件类型(目录) 和访问权限控制位(定义在 include/sys/stat.h)
    // 其中可执行位控制了是否能 cd 到这个目录的权限
    inode->i_mode = I_DIRECTORY | mode & 0777;
    inode->i_dirt = 1;
    // 添加刚刚创建的这个目录文件到其父目录文件中
    bh = add_entry(dir,basename,namelen,&de);
    if (!bh) {
        iput(dir);
        free_block(inode->i_dev,inode->i_zone[0]);
        inode->i_nlinks=0;
        iput(inode);
        return -ENOSPC;
    }
    // 设置目录项的 inode 为我们创建的目录文件的 inode 号，然后释放资源，等待保存到磁盘中
    de->inode = inode->i_num;
    bh->b_dirt = 1;
    dir->i_nlinks++;
    dir->i_dirt = 1;
    iput(dir);
    iput(inode);
    brelse(bh);
    return 0;
}

// 判断目录是否为空，空目录返回 1 ,否则返回 0
static int empty_dir(struct m_inode * inode) {
    int nr,block;
    int len;
    struct buffer_head * bh;
    struct dir_entry * de;

    // 一个目录里面至少包含 .. 和 . 目录，长度不应该小于2
    len = inode->i_size / sizeof (struct dir_entry);
    if (len<2 || !inode->i_zone[0] ||
        !(bh=bread(inode->i_dev,inode->i_zone[0]))) {
        printk("warning - bad directory on dev %04x\n",inode->i_dev);
        return 0;
    }
    // 目录里面第 0 个目录项应该是 . 第 1 个目录项应该是 .. ，如果不是说明目录有问题
    de = (struct dir_entry *) bh->b_data;
    if (de[0].inode != inode->i_num || !de[1].inode ||
        strcmp(".",de[0].name) || strcmp("..",de[1].name)) {
        printk("warning - bad directory on dev %04x\n",inode->i_dev);
        return 0;
    }
    // 来到这里说明目录中至少包含 . 和 .. 目录
    nr = 2;
    de += 2;
    // 遍历一下目录其他的目录项，看看是否有 inode 不为 0 目录项
    while (nr<len) {
        if ((void *) de >= (void *) (bh->b_data+BLOCK_SIZE)) {
            brelse(bh);
            // 寻找第 nr 个目录项所在的磁盘块
            block=bmap(inode,nr/DIR_ENTRIES_PER_BLOCK);
            // 如果该磁盘块已经不用，则扫描下一个磁盘块
            if (!block) {
                nr += DIR_ENTRIES_PER_BLOCK;
                continue;
            }
            // 如果磁盘块有效，从磁盘块开头扫描每一个目录项
            if (!(bh=bread(inode->i_dev,block)))
                return 0;
            de = (struct dir_entry *) bh->b_data;
        }
        // 如果有不为 0 的目录项，说明目录不为空
        if (de->inode) {
            brelse(bh);
            return 0;
        }
        // 扫描磁盘块中的每一个目录项
        de++;
        nr++;
    }
    // 是空目录返回 1
    brelse(bh);
    return 1;
}

// 删除目录，name 路径名，成功返回 0 ，失败返回错误号
int sys_rmdir(const char * name) {
    const char * basename;
    int namelen;
    struct m_inode * dir, * inode;
    struct buffer_head * bh;
    struct dir_entry * de;

    // 检查被删除目录的父目录是否存在
    if (!(dir = dir_namei(name,&namelen,&basename)))
        return -ENOENT;
    // 如果 namelen 为 0 说明删除的路径路径最后没有指出目录名
    if (!namelen) {
        iput(dir);
        return -ENOENT;
    }

    // 从父目录中找出要删除的目录项
    bh = find_entry(&dir,basename,namelen,&de);
    if (!bh) {
        iput(dir);
        return -ENOENT;
    }

    // 通过目录项中的 inode 号找到目录文件的 inode
    if (!(inode = iget(dir->i_dev, de->inode))) {
        iput(dir);
        brelse(bh);
        return -EPERM;
    }

    // 检查 inode 是否能够被删除，引用计数如果大于 1 说明有符号连接，不能删除
    if (inode->i_dev != dir->i_dev || inode->i_count>1) {
        iput(dir);
        iput(inode);
        brelse(bh);
        return -EPERM;
    }
    // 如果被删除的目录项 inode 等于包含该目录的 inode 即删除 . 目录项是不允许的
    // 但是可以删除 ../dir 这样
    if (inode == dir) {
        iput(inode);
        iput(dir);
        brelse(bh);
        return -EPERM;
    }
    // 如果删除的 inode 不是一个目录，返回错误
    if (!S_ISDIR(inode->i_mode)) {
        iput(inode);
        iput(dir);
        brelse(bh);
        return -ENOTDIR;
    }
    // 如果被删除的目录不为空也不能删除
    if (!empty_dir(inode)) {
        iput(inode);
        iput(dir);
        brelse(bh);
        return -ENOTEMPTY;
    }
    // 对于一个空目录，目录项链接数应该为 2 ，链接到本目录和上层目录
    if (inode->i_nlinks != 2)
        printk("empty directory has nlink!=2 (%d)",inode->i_nlinks);
    // 将目录项 inode 标记为 0 表示释放
    de->inode = 0;
    bh->b_dirt = 1;
    brelse(bh);
    // 重置 inode 属性
    inode->i_nlinks=0;
    inode->i_dirt=1;
    // 减少父目录链接数
    dir->i_nlinks--;
    // dir->i_ctime = dir->i_mtime = CURRENT_TIME;
    dir->i_dirt=1;
    iput(dir);
    iput(inode);
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

// 给文件创建硬链接
int sys_link(const char * oldname, const char * newname) {
    struct dir_entry * de;
    struct m_inode * oldinode, * dir;
    struct buffer_head * bh;
    const char * basename;
    int namelen;

    // 先检查原路径名的有效性，如果原路径存在而且不是目录，先取出原路径 inode
    oldinode=namei(oldname);
    if (!oldinode)
        return -ENOENT;
    // 判断 inode 是否目录
    if (S_ISDIR(oldinode->i_mode)) {
        iput(oldinode);
        return -EPERM;
    }
    // 取出新路径的父目录，提取新路径的文件名
    dir = dir_namei(newname,&namelen,&basename);
    if (!dir) {
        iput(oldinode);
        return -EACCES;
    }
    if (!namelen) {
        iput(oldinode);
        iput(dir);
        return -EPERM;
    }
    // 不能跨设备创建链接
    if (dir->i_dev != oldinode->i_dev) {
        iput(dir);
        iput(oldinode);
        return -EXDEV;
    }
    //if (!permission(dir,MAY_WRITE)) {
    //    iput(dir);
    //    iput(oldinode);
    //    return -EACCES;
    //}
    // 检查新路径是否已经存在，如果已经存在不能创建
    bh = find_entry(&dir,basename,namelen,&de);
    if (bh) {
        brelse(bh);
        iput(dir);
        iput(oldinode);
        return -EEXIST;
    }
    // 添加新的目录项到新路径的父目录中
    bh = add_entry(dir,basename,namelen,&de);
    if (!bh) {
        iput(dir);
        iput(oldinode);
        return -ENOSPC;
    }
    // 新的目录项 inode 指向旧的 inode
    de->inode = oldinode->i_num;
    bh->b_dirt = 1;
    brelse(bh);
    iput(dir);
    // inode 引用计数增加
    oldinode->i_nlinks++;
    // oldinode->i_ctime = CURRENT_TIME;
    oldinode->i_dirt = 1;
    iput(oldinode);
    return 0;
}

