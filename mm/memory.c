#include <linux/sched.h>

static inline void oom(void) {
    // todo oom
    printk("out of memory\n\r");
}

#define invalidate() \
__asm__("movl %%eax,%%cr3"::"a" (0))

#define LOW_MEM 0x100000
#define PAGING_MEMORY (15*1024*1024)           // 主内存最多15M
#define PAGING_PAGES (PAGING_MEMORY>>12)       // 分页后的内存页数
#define MAP_NR(addr) (((addr)-LOW_MEM)>>12)    // 内存地址映射为页号
#define USED 100

static long HIGH_MEMORY = 0;

// 复制一页内存
#define copy_page(from,to) \
__asm__("cld ; rep ; movsl"::"S" (from),"D" (to),"c" (1024))

// 物理内存字节映射图，一字节代表一页 (4K) ，每个字节保存的数值表示该内存页被引用的次数
static unsigned char mem_map [ PAGING_PAGES ] = {0,};

unsigned long get_free_page(void)
{
    register unsigned long __res asm("ax");

    // 找出 mem_map 中第一个没有被占用的项，并返回起始地址
    __asm__("std ; repne ; scasb\n\t"
      "jne 1f\n\t"
      "movb $1,1(%%edi)\n\t"
      "sall $12,%%ecx\n\t"
      "addl %2,%%ecx\n\t"
      "movl %%ecx,%%edx\n\t"
      "movl $1024,%%ecx\n\t"
      "leal 4092(%%edx),%%edi\n\t"
      "rep ; stosl\n\t"
      " movl %%edx,%%eax\n"
      "1: cld"
      :"=a" (__res)
      :"0" (0),"i" (LOW_MEM),"c" (PAGING_PAGES),
      "D" (mem_map+PAGING_PAGES-1)
      );
    return __res;
}

// 将 mem_map 中代表 addr 的一项清零，标记 addr 所在内存页可用
void free_page(unsigned long addr)
{
    if (addr < LOW_MEM) return;
    if (addr >= HIGH_MEMORY)
        panic("trying to free nonexistent page");
    addr -= LOW_MEM;
    // 内存地址转换为 mem_map 页号
    addr >>= 12;
    if (mem_map[addr]--) return;
        mem_map[addr]=0;
    panic("trying to free free page");
}

int free_page_tables(unsigned long from,unsigned long size)
{
    unsigned long *pg_table;
    unsigned long * dir, nr;

    if (from & 0x3fffff)
        panic("free_page_tables called with wrong alignment");
    if (!from)
        panic("Trying to free up swapper memory space");
    // 将参数中的 size (字节为单位) 转换为 4M 的整数，即多少个 4M，即页目录项个数
    // + 0x3fffff 是为了处理进位，如原来为 0.1M，+ 0x3fffff 后 >> 22 size 就是 1
    size = (size + 0x3fffff) >> 22;
    // 计算页目录项号 from >> 22 ，因为每个页目录项大小为 4 字节，且页目录起始地址为 0 ，
    // 所以页目录项号转换为页目录项所在地址(指针) 为页目录项号 * 4 ，
    // 所以由线性地址 from 中取页目录项地址转换为 from >> 22 << 2 = from >> 20
    dir = (unsigned long *) ((from>>20) & 0xffc); /* _pg_dir = 0 */
    // 循环将一个页表里面的每个项清零
    for ( ; size-->0 ; dir++) {
        if (!(1 & *dir))
            continue;
        pg_table = (unsigned long *) (0xfffff000 & *dir);
        for (nr=0 ; nr<1024 ; nr++) {
            if (1 & *pg_table)
                free_page(0xfffff000 & *pg_table);
            *pg_table = 0;
            pg_table++;
        }
        free_page(0xfffff000 & *dir);
        // 将页目录项清零
        *dir = 0;
    }
    invalidate();
    return 0;
}

// 复制页目录表项和页表项
// 新复制出来的页表各项和原来被复制的页表项都指向相同的内存(共享)，指导有父子进程其中一个有写操作才真正分配新内存
// 写时复制
int copy_page_tables(unsigned long from,unsigned long to,long size)
{
    unsigned long * from_page_table;
    unsigned long * to_page_table;
    unsigned long this_page;
    unsigned long * from_dir, * to_dir;
    unsigned long nr;

    if ((from&0x3fffff) || (to&0x3fffff))
        panic("copy_page_tables called with wrong alignment");
    // 这里的计算和 free_page_tables 一样，计算目录项地址，size 也是
    from_dir = (unsigned long *) ((from>>20) & 0xffc); /* _pg_dir = 0 */
    to_dir = (unsigned long *) ((to>>20) & 0xffc);
    size = ((unsigned) (size+0x3fffff)) >> 22;
    // 开始复制页目录项
    for( ; size-->0 ; from_dir++,to_dir++) {
        // 如果已经存在，死机
        if (1 & *to_dir)
            panic("copy_page_tables: already exist");
        // 源不存在进行下一个 页目录复制
        if (!(1 & *from_dir))
            continue;

        // 根据页目录项找到页表起始地址
        from_page_table = (unsigned long *) (0xfffff000 & *from_dir);
        // 从主内存中申请一页内存，用作目标页表
        if (!(to_page_table = (unsigned long *) get_free_page()))
            return -1;	/* Out of memory, see freeing */
        // 将页表地址存放到页目录，设置相应的权限为 7 ，表示权限为用户级，可读写，存在(Usr, RW, Present)
        *to_dir = ((unsigned long) to_page_table) | 7;
        // 这里的判断是用来处理内核第一次 fork ，只复制 640KB 内存(当时认为内核大小不超过这个数，复制太多会浪费内存)
        nr = (from==0)?0xA0:1024;
        // 复制页表下的每项
        for ( ; nr-- > 0 ; from_page_table++,to_page_table++) {
            this_page = *from_page_table;
            // 如果该页没有使用，继续下一个页检查
            if (!(1 & this_page))
                continue;
            // 复位内存页的 RW 位(1 设置为 0)，将内存页设置为只读(为将来的写时复制做准备)
            this_page &= ~2;
            *to_page_table = this_page;
            // 如果该页内存地址大于 LOW_MEM，则需要在 mem_map 管理表中将引用次数增加
            if (this_page > LOW_MEM) {
                *from_page_table = this_page;
                this_page -= LOW_MEM;
                this_page >>= 12;
                mem_map[this_page]++;
            }
        }
    }
    invalidate();
    return 0;
}

unsigned long put_page(unsigned long page,unsigned long address)
{
	unsigned long tmp, *page_table;

/* NOTE !!! This uses the fact that _pg_dir=0 */

	if (page < LOW_MEM || page >= HIGH_MEMORY)
		printk("Trying to put page %p at %p\n",page,address);
	if (mem_map[(page-LOW_MEM)>>12] != 1)
		printk("mem_map disagrees with %p at %p\n",page,address);
	page_table = (unsigned long *) ((address>>20) & 0xffc);
	if ((*page_table)&1)
		page_table = (unsigned long *) (0xfffff000 & *page_table);
	else {
		if (!(tmp=get_free_page()))
			return 0;
		*page_table = tmp|7;
		page_table = (unsigned long *) tmp;
	}
	page_table[(address>>12) & 0x3ff] = page | 7;
/* no need for invalidate */
	return page;
}

// 分配一页新的内存，使触发写保护中断的线性地址指向新分配的空间
void un_wp_page(unsigned long * table_entry) {
    unsigned long old_page,new_page;

    // 取页表中存放的物理地址
    old_page = 0xfffff000 & *table_entry;
    // 如果指向的内存地址在主内存区(old_page >= LOW_MEM) 且仅被引用一次(mem_map[MAP_NR(old_page)]==1)
    // 那么设置页表属性 RW ，直接把属性改为可读写即可
    if (old_page >= LOW_MEM && mem_map[MAP_NR(old_page)]==1) {
        *table_entry |= 2;
        invalidate();
        return;
    }
    // 否则要为该页表项重新分配一页内存，然后该页表项指向它
    if (!(new_page=get_free_page()))
        oom();
    // 原本的内存页引用次数减 1 ，因为我们要分配一页新的内存
    if (old_page >= LOW_MEM)
        mem_map[MAP_NR(old_page)]--;
    // 指向新分配的内存
    *table_entry = new_page | 7;
    invalidate();
    // 写时复制，将旧内存页的内容复制到分配到的内存页
    copy_page(old_page,new_page);
}

// 写时复制，将被设置为写保护的内存页复制，并接触写保护
void do_wp_page(unsigned long error_code,unsigned long address) {
    // address 为触发写保护异常的线性地址。
    // 将线性地址转换为页表项
    // address >> 10 = address >> 12 << 2 , >> 12 是为了取出页表项在页目录中的索引，
    // 因为每一个页表项 4 字节，所以该页目录项在页表中的偏移地址为'索引 << 2',所以这里得到的是页表中的偏移地址
    // address >> 20 =address >> 22 << 2 ，同理，这里是页目录项索引，所以这里是取页目录表中的偏移地址(因为页目录是 0 地址开始，所以偏移地址实际上也等于该页目录项的地址, 0 + 偏移地址)，
    // 最后 * (unsigned long *) 表示取出页目录项中的页表地址，页表地址 + 页表偏移地址 = 页表项的物理地址
    un_wp_page((unsigned long *)
        (((address>>10) & 0xffc) + (0xfffff000 &
        *((unsigned long *) ((address>>20) &0xffc)))));
}

void get_empty_page(unsigned long address)
{
    unsigned long tmp;

    if (!(tmp=get_free_page()) || !put_page(tmp,address)) {
        free_page(tmp);		/* 0 is ok - ignored */
        oom();
    }
}

static int try_to_share(unsigned long address, struct task_struct * p)
{
    unsigned long from;
    unsigned long to;
    unsigned long from_page;
    unsigned long to_page;
    unsigned long phys_addr;

    from_page = to_page = ((address>>20) & 0xffc);
    from_page += ((p->start_code>>20) & 0xffc);
    to_page += ((current->start_code>>20) & 0xffc);
    /* is there a page-directory at from? */
    from = *(unsigned long *) from_page;
    if (!(from & 1))
        return 0;
    from &= 0xfffff000;
    from_page = from + ((address>>10) & 0xffc);
    phys_addr = *(unsigned long *) from_page;
    /* is the page clean and present? */
    if ((phys_addr & 0x41) != 0x01)
        return 0;
    phys_addr &= 0xfffff000;
    if (phys_addr >= HIGH_MEMORY || phys_addr < LOW_MEM)
        return 0;
    to = *(unsigned long *) to_page;
    if (!(to & 1)) {
        if ((to = get_free_page()))
            *(unsigned long *) to_page = to | 7;
        else
            oom();
    }
    to &= 0xfffff000;
    to_page = to + ((address>>10) & 0xffc);
    if (1 & *(unsigned long *) to_page)
        panic("try_to_share: to_page already exists");
    /* share them: write-protect */
    *(unsigned long *) from_page &= ~2;
    *(unsigned long *) to_page = *(unsigned long *) from_page;
    invalidate();
    phys_addr -= LOW_MEM;
    phys_addr >>= 12;
    mem_map[phys_addr]++;
    return 1;
}

static int share_page(unsigned long address)
{
    struct task_struct ** p;

    if (!current->executable)
        return 0;
    if (current->executable->i_count < 2)
        return 0;
    for (p = &LAST_TASK ; p > &FIRST_TASK ; --p) {
        if (!*p)
            continue;
        if (current == *p)
            continue;
        if ((*p)->executable != current->executable)
            continue;
        if (try_to_share(address,*p))
            return 1;
    }
    return 0;
}

// 缺页处理
void do_no_page(unsigned long error_code,unsigned long address) {
    int nr[4];
    unsigned long tmp;
    unsigned long page;
    int block,i;

    // 缺页所在页
    address &= 0xfffff000;
    // 减去进程 start_code 可得出对应的逻辑地址
    tmp = address - current->start_code;
    // tmp >= current->end_data 表明进程要在新申请的内存放数据，直接申请新的空闲页面
    if (!current->executable || tmp >= current->end_data) {
        get_empty_page(address);
        return;
    }

    // 说明所缺页面在进程执行影像文件范围内，尝试共享页面，如果共享不成功申请一页新内存并将执行文件的内容读到该页内存
    if (share_page(tmp))
        return;

    if (!(page = get_free_page()))
        oom();

    block = 1 + tmp/BLOCK_SIZE;
    for (i=0 ; i<4 ; block++,i++)
        nr[i] = bmap(current->executable,block);
    bread_page(page,current->executable->i_dev,nr);
    i = tmp + 4096 - current->end_data;
    tmp = page + 4096;
    while (i-- > 0) {
        tmp--;
        *(char *)tmp = 0;
    }
    if (put_page(page,address))
        return;
    free_page(page);
    oom();
}

void mem_init(long start_mem, long end_mem)
{
    int i;

    // 首先简单将所有数组项都标记为占用
    HIGH_MEMORY = end_mem;
    for (i=0 ; i<PAGING_PAGES ; i++)
        mem_map[i] = USED;
    // 计算第一个可分配的内存页号
    i = MAP_NR(start_mem);
    end_mem -= start_mem;
    end_mem >>= 12;
    // 从第一个可分配的页号开始将映射图数组清零，直到最后一页，表示后面的内存页全部可用
    while (end_mem-->0)
        mem_map[i++]=0;
}

