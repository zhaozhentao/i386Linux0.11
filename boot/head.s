# 这个文件开始，cpu 进入 32 位模式，代码是从 0x0 地址开始运行，随后开头部份会被一些页表覆盖掉

.global startup_32

startup_32:
  movl  $0x10, %eax                     # 来到 32 位模式后，可以按 32 位方式操作寄存器，movl 双字模式给寄存器赋值, %eax 是 32 位寄存器, %ax 是 %eax 的低 16 位
  mov   %ax, %ds                        # 因为 %ax 是 %eax 低 16 位,所以 %ax = 0x10, 这里和 mov $0x10 %ds 一样,下面几个指令都一样
  mov   %ax, %es
  mov   %ax, %fs
  mov   %ax, %gs
  lss   stack_start, %esp               # 这里设置一下栈，stack_start 定义在 kernel/sched.c 中, 这个指令将 stack_start 中的 a 赋值 esp 寄存器, b 赋值 ss 寄存器, 其中 a 是 user_stack 地址, b 是段选择符
  call  setup_idt                       # 设置 idt

# setup_idt 需要结合中断描述符的格式去看
# 一个中断描述符由 8 个字节组成, 这里选用edx 和 eax 寄存器存放,格式如下
# edx [31...16(过程入口点偏移值), ]  [ignore_int 高 4 位, 8, E, 0, 0]
# eax [31...16(段选择符), 15..0(过程入口点偏移值)], 8 字节 [0,0,8,0, ignore_int 的低 4 位]
setup_idt:
  lea  ignore_int, %edx                 # 将标号 ignore_int 地址(偏移值)存入 edx 寄存器，即函数入口地址
  movl $0x00080000, %eax                # 将选择符 0x0008 存入 eax 高 16 位中
  movw %dx, %ax                         # 将 ignore_int 低 16 位 存放到 ax 寄存器
  movw $0x8E00, %dx                     /* interrupt gate - dpl=0, present */

  lea idt, %edi                         # 加载中断描述符表地址到 edi 寄存器
  mov $256, %ecx                        # 设置 256 个表项
rp_sidt:
  movl %eax, (%edi)                     # 将上面设置好的中断描述符放到 目的地址 idt 表中, 每个描述符是 8 字节，下一条指令是
  movl %edx, 4(%edi)                    # 向 %edi 寄存器地址 +4 处存入中断描述符的后 4 位
  addl $8, %edi                         # edi 寄存器指向下一个描述符位置
  dec  %ecx                             # 减少未填写描述符数量
  jne  rp_sidt                          # 检查是否填写完中断描述符表 (256 项)
  lidt idt_descr                        # 通知 idt 寄存器加载 idt 表
  ret

int_msg:
  .asciz "Unknown interrupt\n\r"

.align 2
ignore_int:                             # 这是一个哑巴中断处理程序，完成了保护现场和调用 printk 打印一下信息
  pushl  %eax
  pushl  %ecx
  pushl  %edx
  push   %ds
  push   %es
  push   %fs
  movl   $0x10, %eax
  mov    %ax, %ds
  mov    %ax, %es
  mov    %ax, %fs
  pushl  $int_msg
  call   printk
  popl   %eax
  pop    %fs
  pop    %es
  pop    %ds
  popl   %edx
  popl   %ecx
  popl   %eax
  iret

.align 2
.word 0
idt_descr:
.word 256*8-1                            # idt contains 256 entries
.long idt

idt:
.fill 256, 8, 0                          # idt 未初始化

