#define LOW_MEM 0x100000
#define PAGING_MEMORY (15*1024*1024)           // 主内存最多15M
#define PAGING_PAGES (PAGING_MEMORY>>12)       // 分页后的内存页数
#define MAP_NR(addr) (((addr)-LOW_MEM)>>12)    // 内存地址映射为页号
#define USED 100

static long HIGH_MEMORY = 0;

// 物理内存字节映射图，一字节代表一页 (4K) ，每个字节保存的数值表示该内存页被引用的次数
static unsigned char mem_map [ PAGING_PAGES ] = {0,};

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

