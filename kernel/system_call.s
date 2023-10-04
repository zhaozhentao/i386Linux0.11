.global system_call, sys_fork, timer_interrupt, hd_interrupt

state	= 0

nr_system_calls = 74

.align 2
bad_sys_call:
    movl $-1,%eax
    iret

.align 2
reschedule:
  pushl $ret_from_sys_call
  jmp schedule

.align 2
system_call:
  cmpl $nr_system_calls-1,%eax
  ja bad_sys_call
  push %ds
  push %es
  push %fs
  pushl %edx
  pushl %ecx		# push %ebx,%ecx,%edx as parameters
  pushl %ebx		# to the system call
  movl $0x10,%edx		# set up ds,es to kernel space
  mov %dx,%ds
  mov %dx,%es
  movl $0x17,%edx		# fs points to local data space
  mov %dx,%fs
  call *sys_call_table(,%eax,4)
  pushl %eax
  movl current,%eax
  cmpl $0,state(%eax)
  jne reschedule

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

.align 2
sys_fork:
  call find_empty_process
  testl %eax,%eax
  js 1f
  push %gs
  pushl %esi
  pushl %edi
  pushl %ebp
  pushl %eax
  call copy_process
  addl $20,%esp
1:
  ret


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

