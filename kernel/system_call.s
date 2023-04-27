.global timer_interrupt

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

