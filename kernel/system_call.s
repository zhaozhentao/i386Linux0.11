.global system_call, sys_fork, timer_interrupt, hd_interrupt

state	= 0		# these are offsets into the task-struct.
counter	= 4

nr_system_calls = 74              # 系统调用总数

.align 2
bad_sys_call:
  movl $-1, %eax                  # 设置 eax 寄存器为 -1 后结束中断
  iret

.align 2
reschedule:

.align 2
system_call:
  cmpl $nr_system_calls-1,%eax    # 如果调用号超出返回就退出
  ja bad_sys_call
  push %ds                        # 保存原来的段寄存器
  push %es
  push %fs
  pushl %edx                      # 入栈的这几个寄存器保存了系统调用的参数(最多三个)，参数和寄存器是 gcc 决定的 ebx 是第一个采纳数 ecx 是第二个参数 edx 是第三个参数
  pushl %ecx
  pushl %ebx
  movl $0x10,%edx                 # es ds 指向内核数据段
  mov %dx,%ds
  mov %dx,%es
  movl $0x17,%edx                 # fs 指向局部数据段，即发起本次系统调用的用户程序的数据段
  mov %dx,%fs
  call *sys_call_table(,%eax,4)   # 这里根据功能号去调用对应的处理程序
  pushl %eax
  movl current,%eax
  cmpl $0,state(%eax)             # 如果任务不在就绪状态就去执行调度程序
  jne reschedule
  cmpl $0,counter(%eax)           # 如果任务的时间片用完就去执行调度程序
  je reschedule

.align 2
timer_interrupt:
  push %ds                        # 保护现场
  push %es
  push %fs
  pushl %edx
  pushl %ecx
  pushl %ebx
  pushl %eax
  movb $0x20, %al                 # 清除中断标志,以便能够重新被触发中断
  outb %al, $0x20
  call do_timer                   # 调用 c 实现的中断处理函数
  jmp ret_from_sys_call

.align 2
sys_fork:

hd_interrupt:                     # 保护现场
  pushl %eax
  pushl %ecx
  pushl %edx
  push %ds
  push %es
  push %fs
  movl $0x10, %eax                # ds es 指向内核数据段
  mov %ax, %ds
  mov %ax, %es
  movb $0x20, %al                 # 清除中断标志，以便能重新触发中断
  outb %al, $0xA0
1:
  xorl %edx, %edx                 # 清空 edx
  xchgl do_hd, %edx               # 将中断处理函数地址放到 edx
  testl %edx, %edx                # 判断一下中断处理函数地址是否为空
  jne 1f                          # 如果中断处理函数不为空，跳转到下面 1 标签处，否则跳转到 unexpected_hd_interrupt
  movl $unexpected_hd_interrupt, %edx
1:
  outb %al,$0x20
  call *%edx                      # 跳转中断处理程序
  pop %fs                         # 恢复现场
  pop %es
  pop %ds
  popl %edx
  popl %ecx
  popl %eax
  iret

ret_from_sys_call:
3:
  popl %eax                       # 恢复现场
  popl %ebx
  popl %ecx
  popl %edx
  pop %fs
  pop %es
  pop %ds
  iret

