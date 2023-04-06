.global startup_32, pg_dir

pg_dir:
startup_32:
  movl  $0x10, %eax                     # 设置段选择子为 0x10 ，此时使用的是全局描述符可查看 setup.s 中的 gdt 表
  mov   %ax, %ds                        # 将段选择子设置到 ds es fs gs 几个寄存器
  mov   %ax, %es
  mov   %ax, %fs
  mov   %ax, %gs
  lss   stack_start, %esp               # 这里设置一下栈，stack_start 定义在 kernel/sched.c 中, 这个指令将 stack_start 中的 a 赋值 esp 寄存器, b 赋值 ss 寄存器, 其中 a 是 user_stack 地址, b 是段选择符
  call  setup_idt                       # 设置 idt 中断描述符表
  call  setup_gdt                       # 设置 gdt 全局描述符表
  movl  $0x10, %eax                     # 修改完 gdt 后，需要重新加载一下段寄存器
  mov   %ax,   %ds
  mov   %ax,   %es
  mov   %ax,   %fs
  mov   %ax,   %gs
  lss   stack_start, %esp
  xorl  %eax, %eax
1:
  incl  %eax                            # 检查一下A20是否真的开启了
  movl  %eax, 0x000000                  # 死循环表示没有开启成功
  cmpl  %eax, 0x100000
  je    1b

# 检查数学协处理器
  movl  %cr0,%eax                       # 检查数学协处理器
  andl  $0x80000011, %eax               # Save PG,PE,ET
  /* "orl $0x10020,%eax" here for 486 might be good */
  orl   $2, %eax                        # set MP
  movl  %eax, %cr0
  call  check_x87
  jmp   after_page_tables

check_x87:
  fninit
  fstsw %ax
  cmpb  $0,%al
  je    1f                              /* no coprocessor: have to set bits */
  movl %cr0, %eax
  xorl $6, %eax                         /* reset MP, set EM */
  movl %eax, %cr0
  ret
.align 2
1:
  .byte 0xDB, 0xE4		/* fsetpm for 287, ignored by 387 */
  ret

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

setup_gdt:
  lgdt gdt_descr
  ret

.org 0x1000
pg0:

.org 0x2000
pg1:

.org 0x3000
pg2:

.org 0x4000
pg3:

.org 0x5000

after_page_tables:
  pushl  $0                             # These are the parameters to main :-)
  pushl  $0
  pushl  $0
  pushl  $L6                            # return address for main, if it decides to.
  pushl  $main
  jmp    setup_paging
L6:
  jmp L6                                # main should never return here, but

.align 2
ignore_int:                             # 这是一个哑巴中断处理程序，完成了保护现场和调用 printk 打印一下信息

.align 2
setup_paging:
  movl  $1024*5, %ecx		/* 5 pages - pg_dir+4 page tables */
  xorl  %eax, %eax
  xorl  %edi, %edi			/* pg_dir is at 0x000 */
  cld;rep;stosl
  movl  $pg0+7, pg_dir		/* set present bit/user r/w */
  movl  $pg1+7, pg_dir+4		/*  --------- " " --------- */
  movl  $pg2+7, pg_dir+8		/*  --------- " " --------- */
  movl  $pg3+7, pg_dir+12		/*  --------- " " --------- */
  movl  $pg3+4092, %edi
  movl  $0xfff007, %eax		/*  16Mb - 4096 + 7 (r/w user,p) */
  std
1:
  stosl			/* fill pages backwards - more efficient :-) */
  subl  $0x1000, %eax
  jge   1b
  cld
  xorl  %eax,  %eax		/* pg_dir is at 0x0000 */
  movl  %eax,  %cr3		/* cr3 - page directory start */
  movl  %cr0,  %eax
  orl   $0x80000000, %eax
  movl  %eax, %cr0		/* set paging (PG) bit */
  ret			/* this also flushes prefetch-queue */

.align 2
.word 0
idt_descr:
  .word 256*8-1                            # idt contains 256 entries
  .long idt

.align 2
.word 0
gdt_descr:
  .word 256*8-1		# so does gdt (not that that's any
  .long gdt		# magic number, but it works for me :^)

idt:
  .fill 256, 8, 0                          # idt 未初始化

gdt:
  .quad 0x0000000000000000                 # 空的描述符
  .quad 0x00c09a0000000fff                 # 内核代码段最大长度 16 M,各项说明 00c0: granularity=4096, 386, 9A00: code read/exec, 0000: base address=0, 0fff: 16M
  .quad 0x00c0920000000fff                 # 内核数据段最大长度 16 M,各项说明 00c0: granularity=4096, 386, 9200: data read/write, 0000: base address=0, 0fff: 16M
  .quad 0x0000000000000000
  .fill 252,8,0                            # 保留
