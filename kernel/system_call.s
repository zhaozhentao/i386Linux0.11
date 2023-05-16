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

hd_interrupt:
  pushl %eax
  pushl %ecx
  pushl %edx
  push %ds
  push %es
  push %fs
  movl $0x10,%eax
  mov %ax,%ds
  mov %ax,%es
  movb $0x20,%al
  outb %al,$0xA0		# EOI to interrupt controller #1
1:
  xorl %edx,%edx
  xchgl do_hd,%edx
  testl %edx,%edx
  jne 1f
  movl $unexpected_hd_interrupt,%edx
1:
  outb %al,$0x20
  call *%edx		# "interesting" way of handling intr.
  pop %fs
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

