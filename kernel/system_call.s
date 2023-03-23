.global device_not_available, coprocessor_error, parallel_interrupt, system_call, timer_interrupt, hd_interrupt

# todo: this function
.align 2
system_call:

# todo: this function
ret_from_sys_call:

# todo: this function
.align 2
coprocessor_error:

.align 2
device_not_available:                       # 设备不存在或协处理器不存器 CPU 执行一个协处理器指令就会触发异常,在异常处理程序模拟协处理器处理
  push  %ds
  push  %es
  push  %fs
  pushl %edx
  pushl %ecx
  pushl %ebx
  pushl %eax
  movl  $0x10, %eax                         # 下面三个指令使 ds es 指向内核数据段
  mov   %ax, %ds
  mov   %ax, %es
  movl  $0x17, %eax                         # 使 fs 指向局部数据段(出错程序的数据段)
  mov   %ax, %fs
  pushl $ret_from_sys_call
  clts                                      # clear TS so that we can use math
  movl  %cr0, %eax
  testl $0x4, %eax                          # EM (math emulation bit)
  je    math_state_restore                  # 执行 c 函数 math_state_restore
  pushl %ebp                                # 如果 EM 是置位的执行 math_emulate 数学仿真程序
  pushl %esi
  pushl %edi
  call  math_emulate
  popl  %edi
  popl  %esi
  popl  %ebp
  ret                                       # 这里的 ret 是指跳转到 ret_from_sys_call

# todo this 中断
.align 2
timer_interrupt:

hd_interrupt:

parallel_interrupt:
  pushl %eax
  movb  $0x20, %al
  outb  %al, $0x20
  popl  %eax
  iret

