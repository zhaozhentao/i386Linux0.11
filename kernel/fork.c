#include <errno.h>

#include <linux/sched.h>
#include <asm/system.h>

long last_pid=0;

// 复制内存页表
// 为新任务复制代码和数据段段基址，段限长
int copy_mem(int nr,struct task_struct * p)
{
    unsigned long old_data_base,new_data_base,data_limit;
    unsigned long old_code_base,new_code_base,code_limit;

    // 获取代码段数据段在线性地址中的段限长 0x0f是代码段选择符 0x17 是数据段选择符
    code_limit=get_limit(0x0f);
    data_limit=get_limit(0x17);
    // 取代码段和数据段在线性地址中的段基址
    old_code_base = get_base(current->ldt[1]);
    old_data_base = get_base(current->ldt[2]);
    // 检查代码段和数据段基址是否相同，不相同就死机
    if (old_data_base != old_code_base)
        panic("We don't support separate I&D");
    // 检查段限长是否相同
    if (data_limit < code_limit)
        panic("Bad data_limit");

    // 新进程在线性地址空间中基地址为 64M * 任务号
    new_data_base = new_code_base = nr * 0x4000000;
    p->start_code = new_code_base;
    set_base(p->ldt[1],new_code_base);
    set_base(p->ldt[2],new_data_base);
    if (copy_page_tables(old_data_base,new_data_base,data_limit)) {
        printk("free_page_tables: from copy_mem\n");
        free_page_tables(new_data_base,data_limit);
        return -ENOMEM;
    }
    return 0;
}

// 这个函数的参数由从调用 system_call 处开始入栈，最后的参数表示入栈最早
// 所以用户栈地址ss, esp, 标志寄存器eflags, 返回地址cs, eip 是触发 system_call 中断自动入栈的寄存器值
// ds es fs edx ecs ebx 是在 system_call 入栈的
// none 是 system_call 调用 (call) sys_fork 入栈的返回地址
// gs esi edi ebp nr 是 sys_fork 中在 call copy_process 前入栈的数据，其中 nr 是任务数组项号
int copy_process(int nr,long ebp,long edi,long esi,long gs,long none,
		long ebx,long ecx,long edx,
		long fs,long es,long ds,
		long eip,long cs,long eflags,long esp,long ss)
{
    struct task_struct *p;
    int i;
    struct file *f;

    // 首先为新的任务数据结构分配内存，如果分配失败就返回
    p = (struct task_struct *) get_free_page();
    if (!p)
        return -EAGAIN;
    // 分配成功填入任务数组
    task[nr] = p;

    *p = *current;	/* NOTE! this doesn't copy the supervisor stack */
    p->state = TASK_UNINTERRUPTIBLE;    // 任务状态为不可中断等待状态
    p->pid = last_pid;                  // 新进程号
    p->father = current->pid;           // 任务父进程 id
    p->counter = p->priority;           // 任务运行时间片
    p->signal = 0;                      // 信号位图置 0
    p->alarm = 0;                       // 报警定时值
    p->leader = 0;                      // 进程的领导权是不能继承的
    p->utime = p->stime = 0;            // 用户态时间和核心态时间
    p->cutime = p->cstime = 0;          // 子进程用户态时间和核心态时间
    p->start_time = jiffies;            // 进程开始时间

    p->tss.back_link = 0;               // 
    p->tss.esp0 = PAGE_SIZE + (long) p; // 任务的内核态指针
    p->tss.ss0 = 0x10;                  // 内核态段选择符
    p->tss.eip = eip;                   // 指令代码指针
    p->tss.eflags = eflags;             // 标志寄存器
    p->tss.eax = 0;                     // fork 返回时返回的 0 就是这里产生的
    p->tss.ecx = ecx;
    p->tss.edx = edx;
    p->tss.ebx = ebx;
    p->tss.esp = esp;
    p->tss.ebp = ebp;
    p->tss.esi = esi;
    p->tss.edi = edi;
    p->tss.es = es & 0xffff;            // 段寄存器只有 16 位
    p->tss.cs = cs & 0xffff;
    p->tss.ss = ss & 0xffff;
    p->tss.ds = ds & 0xffff;
    p->tss.fs = fs & 0xffff;
    p->tss.gs = gs & 0xffff;
    p->tss.ldt = _LDT(nr);               // 任务局部表描述符的选择符
    p->tss.trace_bitmap = 0x80000000;    // 高 16 位有效
    if (last_task_used_math == current)
        __asm__("clts ; fnsave %0"::"m" (p->tss.i387));
    if (copy_mem(nr,p)) {
        task[nr] = NULL;
        free_page((long) p);
        return -EAGAIN;
    }
    // 如果父进程的文件是打开的，子进程的文件也是打开的，所以文件的打开次数要 +1
    for (i=0; i<NR_OPEN;i++)
        if ((f=p->filp[i]))
            f->f_count++;
    // 下面的几个同理也要 +1
    if (current->pwd)
        current->pwd->i_count++;
    if (current->root)
        current->root->i_count++;
    if (current->executable)
        current->executable->i_count++;

    set_tss_desc(gdt+(nr<<1)+FIRST_TSS_ENTRY,&(p->tss));
    set_ldt_desc(gdt+(nr<<1)+FIRST_LDT_ENTRY,&(p->ldt));

    // 最后才将新任务置为就绪状态
    p->state = TASK_RUNNING;
    return last_pid;
}

// 为新进程取得不重复的进程号，返回任务数组中的任务号(数组项)
int find_empty_process(void) {
    int i;

    repeat:
        // 如果 +1 后超出正整数范围，重置为 1
        if ((++last_pid)<0) last_pid=1;
        // 检查 last_pid 是否已经存在在任务数组中，如果是就重新获取一个 last_pid，last_pid 是全局变量不用返回
        for(i=0 ; i<NR_TASKS ; i++)
            if (task[i] && task[i]->pid == last_pid) goto repeat;
    // 在任务数组中找一个空项，返回项号
    for(i=1 ; i<NR_TASKS ; i++)
        if (!task[i])
            return i;
    return -EAGAIN;
}

