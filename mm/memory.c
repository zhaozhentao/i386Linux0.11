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

