.global page_fault

page_fault:
  xchgl %eax,(%esp)  # 发生异常时，错误码自动压入栈，取出错误码
  pushl %ecx         # 保存通用寄存器
  pushl %edx
  push %ds
  push %es
  push %fs
  movl $0x10,%edx    # 内核段选择符
  mov %dx,%ds
  mov %dx,%es
  mov %dx,%fs
  movl %cr2,%edx     # 保存引起中断的线性地址
  pushl %edx         # 将地址和错误号入栈（作为函数调用的参数）
  pushl %eax
  testl $1,%eax      # 测试页存在标志位，判断是不是缺页中断
  jne 1f
  call do_no_page    # 跳转到缺页中断处理函数
  jmp 2f
1:
  call do_wp_page    # 跳转到写保护处理函数
2:
  addl $8,%esp       # 参数出栈
  pop %fs            # 还原现场
  pop %es
  pop %ds
  popl %edx
  popl %ecx
  popl %eax
  iret

