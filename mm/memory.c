#define invalidate() \
__asm__("movl %%eax,%%cr3"::"a" (0))

#define LOW_MEM 0x100000
#define PAGING_MEMORY (15*1024*1024)           // 主内存最多15M
#define PAGING_PAGES (PAGING_MEMORY>>12)       // 分页后的内存页数
#define MAP_NR(addr) (((addr)-LOW_MEM)>>12)    // 内存地址映射为页号
#define USED 100

static long HIGH_MEMORY = 0;

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

