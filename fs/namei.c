#include <linux/sched.h>

#include <errno.h>
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
    de = (struct dir_entry *) bh->b_data;

    while (i < entries) {
        if ((char *)de >= BLOCK_SIZE+bh->b_data) {
            brelse(bh);
            bh = NULL;
        }

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

static struct m_inode * get_dir(const char * pathname) {
    char c;
    const char * thisname;
    struct m_inode * inode;
    struct buffer_head * bh;
    int namelen, inr, idev;;
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
        for(namelen=0;(c=pathname[0], pathname++, c)&&(c!='/');namelen++)
            /* nothing */ ;
        if (!c)
            return inode;

		if (!(bh = find_entry(&inode,thisname,namelen,&de))) {
			iput(inode);
			return NULL;
		}
        inr = de->inode;
        idev = inode->i_dev;
        brelse(bh);
        iput(inode);
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

    return dir;
}

int open_namei(const char * pathname, int flag, int mode,
    struct m_inode ** res_inode) {
    const char * basename;
    int inr,dev,namelen;
    struct m_inode * dir;

    if (!(dir = dir_namei(pathname,&namelen,&basename)))
        return -ENOENT;
}

