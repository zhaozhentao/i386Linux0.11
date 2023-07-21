#include <linux/sched.h>

#include <sys/stat.h>

// 释放一级数据块
static void free_ind(int dev,int block) {
	struct buffer_head * bh;
	unsigned short * p;
	int i;

	if (!block)
		return;
	// 读取一级数据块
	if ((bh=bread(dev,block))) {
		p = (unsigned short *) bh->b_data;
		// 遍历释放每个数据块
		for (i=0;i<512;i++,p++)
			if (*p)
				free_block(dev,*p);
		brelse(bh);
	}
	// block 号数据块
	free_block(dev,block);
}

// 释放二级间接块
static void free_dind(int dev,int block) {
	struct buffer_head * bh;
	unsigned short * p;
	int i;

	if (!block)
		return;
	// 读取第一级存放次级块号的磁盘块
	if ((bh=bread(dev,block))) {
		p = (unsigned short *) bh->b_data;
		// 遍历释放二级间接块
		for (i=0;i<512;i++,p++)
			if (*p)
				free_ind(dev,*p);
		brelse(bh);
	}
	// 释放一级间接块数据块自身
	free_block(dev,block);
}

// 截断文件函数，把文件长度截断为 0 ，释放占用的数据块
void truncate(struct m_inode * inode) {
	int i;

	// 判断文件有效性，不是常规文件或者目录就返回
	if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode)))
		return;
	// 释放直接块号所指向的数据块
	for (i=0;i<7;i++)
		if (inode->i_zone[i]) {
			free_block(inode->i_dev,inode->i_zone[i]);
			inode->i_zone[i]=0;
		}
	// 释放一级间接块
	free_ind(inode->i_dev,inode->i_zone[7]);
	// 释放二级间接块
	free_dind(inode->i_dev,inode->i_zone[8]);
	inode->i_zone[7] = inode->i_zone[8] = 0;
	// 文件长度标记为 0
	inode->i_size = 0;
	inode->i_dirt = 1;
	// inode->i_mtime = inode->i_ctime = CURRENT_TIME;
}

