.code16
.global _start

.equ INITSEG, 0x9000                        # bootsect 被移动到这里 0x90000
.equ SYSSEG, 0x1000                         # system 被加载到这个地址 0x10000
.equ SETUPSEG, 0x9020                       # setup 模块所在位置

_start:
  mov  %cs, %ax
  mov  %ax, %ds
  mov  %ax, %es

# 打印一点信息，现在我们来到了 setup 模块了
  mov  $0x03, %ah
  xor  %bh, %bh
  int  $0x10
  
  mov  $29, %cx
  mov  $0x000b, %bx
  mov  $msg2, %bp
  mov  $0x1301, %ax
  int  $0x10

# 保存光标位置
  mov  $INITSEG, %ax                        # this is done in bootsect already, but...
  mov  %ax, %ds                             # 将数据段寄存器 ds 再次设置为 0x90000
  mov  $0x03, %ah                           # 读取光标位置功能号
  xor  %bh, %bh                             # 清零
  int  $0x10                                # 发起中断 读取光标位置
  mov  %dx, %ds:0                           # 将读取到的光标位置保存到 0x90000.

# 保存内存大小 单位是 kb
  mov  $0x88, %ah
  int  $0x15
  mov  %ax, %ds:2                           # 保存到 0x90002

# 获取显卡数据
  mov  $0x0f, %ah
  int  $0x10
  mov  %bx, %ds:4                           # 保存到 0x90004 bh = 当前显示页
  mov  %ax, %ds:6                           # 数据保存到 0x90006 al = 显示模式, ah = 窗口宽度

# 获取 VGA 参数
  mov  $0x12, %ah
  mov  $0x10, %bl
  int  $0x10
  mov  %ax, %ds:8                           # 0x90008
  mov  %bx, %ds:10                          # 0x9000f
  mov  %cx, %ds:12                          # 0x9000c

# Get hd0 data
  mov  $0x0000, %ax
  mov  %ax, %ds
  lds  %ds:4*0x41, %si
  mov  $INITSEG, %ax
  mov  %ax, %es
  mov  $0x0080, %di
  mov  $0x10, %cx
  rep  movsb

# Get hd1 data
  mov  $0x0000, %ax
  mov  %ax, %ds
  lds  %ds:4*0x46, %si
  mov  $INITSEG, %ax
  mov  %ax, %es
  mov  $0x0090, %di
  mov  $0x10, %cx
  rep  movsb

# modify ds
  mov  $INITSEG, %ax
  mov  %ax, %ds
  mov  $SETUPSEG, %ax
  mov  %ax, %es

# 检查一下是否存在 hd1
  mov  $0x01500, %ax                        # ah = 0x15 功能号
  mov  $0x81, %dl                           # dl 驱动器号 0x81 第一个驱动器 0x82 第二个驱动器
  int  $0x13                                # 利用 BIOS 0x13 取盘类型功能
  jc   no_disk1                             # 不存在 disk1 跳转到这里
  cmp  $3, %ah
  je   is_disk1                             # 存在 disk1 跳转到这里
no_disk1:
  mov  $INITSEG, %ax
  mov  %ax, %es
  mov  $0x0090, %di
  mov  $0x10, %cx
  mov  $0x00, %ax
  rep  stosb
is_disk1:
# 从这里开始关闭中断,因为要准备进入保护模式
  cli                                       # 关闭中断
# 准备将 system 模块移动,下面这段程序把原本存放在 0x10000 ~ 0x8ffff 的 system 模块整体向下迁移到 0x0 地址处
  mov  $0x0000, %ax                         # 
  cld                                       # 方向标志位清零, 'direction'=0, movs moves forward
do_move:
  mov  %ax,%es                              # 设置要迁移的目的地址
  add  $0x1000, %ax                         # 将 ax 指向源地址
  cmp  $0x9000, %ax                         # 检查 ax 是否已经到源地址末端
  jz   end_move                             # 如果已经到 0x90000 迁移完成
  mov  %ax, %ds                             # 设置复制源头,源地址是 ds:si (初始为 0x1000:0x0)
  sub  %di, %di                             # destination index 目的地寄存器设为 0
  sub  %si, %si
  mov  $0x8000, %cx                         # 设置要移动的字数 (64kb 字节)
  rep  movsw
  jmp  do_move

# 这里开始加载段描述符
end_move:
  mov  $SETUPSEG, %ax                       # right, forgot this at first. didn't work :-)
  mov  %ax, %ds                             # 使数据段寄存器 ds 页指向 0x90200
  lidt idt_48                               # 加载中断描述符
  lgdt gdt_48                               # 加载全局描述符表

# 开启 A20
  inb  $0x92, %al                           # open A20 line(Fast Gate A20).
  orb  $0b00000010, %al
  outb %al, $0x92

# 重新对中断编程
  mov  $0x11, %al                           # initialization sequence(ICW1)
                                            # ICW4 needed(1),CASCADE mode,Level-triggered
  out  %al, $0x20                           # send it to 8259A-1
  .word	0x00eb,0x00eb                       # jmp $+2, jmp $+2
  out	%al, $0xA0                          # and to 8259A-2
  .word	0x00eb,0x00eb
  mov	$0x20, %al                          # start of hardware int's (0x20)(ICW2)
  out	%al, $0x21                          # from 0x20-0x27
  .word	0x00eb,0x00eb
  mov	$0x28, %al                          # start of hardware int's 2 (0x28)
  out	%al, $0xA1                          # from 0x28-0x2F
  .word	0x00eb,0x00eb                       # IR 7654 3210
  mov	$0x04, %al                          # 8259-1 is master(0000 0100) --\
  out	%al, $0x21                          #				|
  .word	0x00eb,0x00eb                       #			 INT	/
  mov	$0x02, %al                          # 8259-2 is slave(       010 --> 2)
  out	%al, $0xA1
  .word	0x00eb,0x00eb
  mov	$0x01, %al                          # 8086 mode for both
  out	%al, $0x21
  .word	0x00eb,0x00eb
  out	%al, $0xA1
  .word	0x00eb,0x00eb
  mov	$0xFF, %al                          # mask off all interrupts for now
  out	%al, $0x21
  .word	0x00eb,0x00eb
  out	%al, $0xA1

# 设置好中断、全局描述符表之后，正式进入保护模式
  mov  %cr0, %eax                           # 获取机器状态 get machine status(cr0|MSW)	
  bts  $0, %eax                             # 设置保护模式的位
  mov  %eax, %cr0                           # 开启保护模式
              
# segment-descriptor        (INDEX:TI:RPL)
  .equ sel_cs0, 0x0008                      # 选择 gdt 描述符
  ljmp $sel_cs0, $0                         # 选择下面的 gdt 表第一个描述符，因为段选择子是 0x8, 偏移量是0,离开 setup 模块

gdt:
  .word 0,0,0,0                             # 第一个描述符，不用
# 在 gdt 表中这里的偏移量是 0x8,是内核代码段选择服的值，对应上面的 ljmp $sel_cs0, $0
  .word 0x07FF                              # 8Mb - limit=2047 (2048*4096=8Mb)
  .word 0x0000                              # base address=0
  .word 0x9A00                              # code read/exec
  .word 0x00C0                              # granularity=4096, 386
# 在 gdt 表中这里的偏移量是 0x10 是内核数据段选择符的值
  .word 0x07FF                              # 8Mb - limit=2047 (2048*4096=8Mb)
  .word 0x0000                              # base address=0
  .word 0x9200                              # data read/write
  .word 0x00C0                              # granularity=4096, 386

# CPU 进入保护模式之前要求设置 IDT 表，这里先设置一个空表
idt_48:
  .word 0                                   # 这里两个字节是 idt 表的限长,这里先设 为 0
  .word 0, 0                                # idt 表在线性空间地址中 32 位基地址，这里设置为 0

gdt_48:
  .word 0x800                               # gdt limit=2048, 256 GDT entries
  .word 512+gdt, 0x9                        # gdt base = 0X9xxxx

msg2:
  .byte 13, 10
  .ascii "Now we are in setup ..."
  .byte 13, 10, 13, 10

