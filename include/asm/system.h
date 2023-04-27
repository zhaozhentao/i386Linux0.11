#define sti() __asm__ ("sti"::)                         // 开中断
#define cli() __asm__ ("cli"::)                         // 关中断

#define _set_gate(gate_addr,type,dpl,addr) \
__asm__ ("movw %%dx,%%ax\n\t" \
    "movw %0,%%dx\n\t" \
    "movl %%eax,%1\n\t" \
    "movl %%edx,%2" \
    : \
    : "i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
    "o" (*((char *) (gate_addr))), \
    "o" (*(4+(char *) (gate_addr))), \
    "d" ((char *) (addr)),"a" (0x00080000))

// 设置中断门函数(自动屏蔽后续中断)
// n 是中断号，addr 是中断处理函数的偏移地址
// &idt[n] 是中断描述符表里面中断号 n 对应的偏移地址, 14 是中断描述符类型，0 是特权级
#define set_intr_gate(n,addr) \
    _set_gate(&idt[n],14,0,addr)

#define set_trap_gate(n,addr) \
    _set_gate(&idt[n],15,0,addr)

