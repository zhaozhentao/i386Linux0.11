# 这个文件开始，cpu 进入 32 位模式，代码是从 0x0 地址开始运行，随后开头部份会被一些页表覆盖掉

.global startup_32

startup_32:
  movl  $0x10, %eax                     # 来到 32 位模式后，可以按 32 位方式操作寄存器，movl 双字模式给寄存器赋值, %eax 是 32 位寄存器, %ax 是 %eax 的低 16 位
  mov   %ax, %ds                        # 因为 %ax 是 %eax 低 16 位,所以 %ax = 0x10, 这里和 mov $0x10 %ds 一样,下面几个指令都一样
  mov   %ax, %es
  mov   %ax, %fs
  mov   %ax, %gs
  lss   stack_start, %esp


