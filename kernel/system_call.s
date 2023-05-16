.global timer_interrupt, hd_interrupt

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

