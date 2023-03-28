.code16                               # 开机时处于 16 位的实模式运行 bootsect 需要以 16 位编译

.global _start                        # 使 _start 标签全局可以访问

.equ BOOTSEG, 0x07c0                  # 启动扇区代码的起始地址 0x7c00
.equ INITSEG, 0x9000                  # 表示启动代码将会搬到 0x90000 这个位置, 注意是 INITSEG 左移 4 位
.equ SETUPLEN, 4                      # nr of setup 模块的扇区数
.equ AX, 0x0200 + SETUPLEN
.equ SYSSIZE, 0x3000
.equ SYSSEG, 0x1000                   # system 模块将会被搬到这个位置 0x10000 (65536).
.equ ENDSEG, SYSSEG + SYSSIZE         # where to stop loading
.equ SETUPSEG, 0x9020                 # setup 模块将会被加载到这个地址 0x90200
.equ ROOT_DEV, 0x301

_start:
  mov  $BOOTSEG, %ax                  # 这两条指令将 ds 数据段寄存器设置为0x7C0
  mov  %ax, %ds
  mov  $INITSEG, %ax                  # 将 es 附加段段寄存器设置为0x900
  mov  %ax, %es
  mov  $256, %cx                      # 设置移动计数值 256 字,即 512 字节
  sub  %si, %si                       # si = si - si 表示将 si 寄存器清零, 因为在当前实模式下 寻址方式为 ds << 4 + si 即 0x7c00 + 0x0
  sub  %di, %di                       # 同上 目标地址为 es << 4 + di 即 0x90000 + 0x0
  rep  movsw                          # 重复执行并递减 cx 的值, 将 (e)cx 个字从 ds:[(e)si] 移到 es:[(e)di]
  ljmp $INITSEG, $go                  # 段间跳转，这里 INITSEG 指出跳转到的段地址，跳转后 cs 代码段寄存器的值为0x9000

go:
  mov  %cs, %ax                       # 将 ds，es，ss 都设置成移动后代码所在的段处 (0x9000),因为跳转到 go 后 cs 寄存器自动被设置为 9000
  mov  %ax, %ds
  mov  %ax, %es
# put stack at 0x9ff00.               # 栈指针通过 ss:sp 确定
  mov  %ax, %ss
  mov  $0xFF00, %sp                   # 因为栈指针通过 ss << 4 + sp 确定，所以执行完这条指令后，栈指向 0x9ff00

load_setup:                           # 将 setup 模块加载到 bootsect 模块后, 即 0x90200 地址处
  mov  $0x0000, %dx                   # drive 0, 0 号磁头
  mov  $0x0002, %cx                   # 磁道 0 ，第二个扇区
  mov  $0x0200, %bx                   # 将磁盘读出的数据放到这个位置, 位置是由 es:bx 确定，目前 es 为 0x9000,所以地址为 0x9000 << 4 + 0x200 = 0x90200
  mov  $AX, %ax                       # service 2, nr of sectors
  int  $0x13                          # 使用 BIOS 读取磁盘功能
  jnc  ok_load_setup                  # 成功读取磁盘内容，跳转到 ok_load_setup
  mov  $0x0000, %dx                   # 如果上面的指令没有成功，则继续执行下面这几条指令，重新 load_setup，在实验中不会执行到这里
  mov  $0x0000, %ax                   # reset the diskette
  int  $0x13
  jmp  load_setup

ok_load_setup:
  mov  $0x00, %dl                     # 调用 BIOS 获取硬盘参数
  mov  $0x0800, %ax                   # AH=8 表示获取硬盘参数
  int  $0x13
  mov  $0x00, %ch
  #seg cs
  mov  %cx, %cs:sectors + 0           # 将保存在 cx 寄存器的磁道数保存在 sectors 中
  mov  $INITSEG, %ax
  mov  %ax, %es                       # 重新将 INITSEG 保存到 es 附加寄存器

# 利用 BIOS 打印 IceCityOS is booting ...
# BIOS 中的打印程序会根据当前的 es 附加段寄存器和 bp 寄存器保存的偏移值去寻找 msg1 位置然后打印
  mov  $0x03, %ah                     # 读取光标位置功能号
  xor  %bh, %bh
  int  $0x10                          # 发起中断，调用读取光标位置的功能，执行后 dx 寄存器会保存光标位置，供后面的打印程序使用

  mov  $30, %cx                       # 需要打印 30 个字节, 具体可以数一下 msg1 下的字符个数
  mov  $0x0007, %bx                   # page 0, attribute 7 (normal)
  mov  $msg1, %bp                     # 将 msg1 在程序中的偏移地址保存到 bp 寄存器
  mov  $0x1301, %ax                   # 功能号,将字符写到屏幕上，同时移动一下光标
  int  $0x10                          # 发起中断，执行 BIOS 打印程序

# 接下来将 system 模块搬到 0x10000 处
  mov  $SYSSEG, %ax
  mov  %ax, %es                       # 指定接下来从磁盘读出的内容要存放的地址, 依然是 (es << 4 + bx)
  call read_it
  call kill_motor                     # 关闭马达

  mov  %cs:root_dev+0, %ax            # 读取 root_dev
  cmp  $0, %ax                        # 和 0 比较一下
  jne  root_defined                   # 如果 root_dev != 0 跳转到 root_defined
  #seg cs
  mov  %cs:sectors+0, %bx
  mov  $0x0208, %ax		# /dev/ps0 - 1.2Mb
  cmp  $15, %bx
  je   root_defined
  mov  $0x021c, %ax		# /dev/PS0 - 1.44Mb
  cmp  $18, %bx
  je   root_defined
undef_root:
  jmp undef_root                      # 找不到设备，死循环
root_defined:
  mov  %ax, %cs:root_dev+0            # 将检查过的设备号保存到 root_dev 中

# 加载完所有模块后，可以跳转到 setup 了, bye bye bootsect
  ljmp  $SETUPSEG, $0                 # 跳转下一个模块 setup

# 此处存放几个变量,用于下面的 read_it 程序
sread: .word 1 + SETUPLEN             # 当前磁道已读扇区数，当前扇区从 build.sh 中可以看到 bootsect 占 1 个扇区, setup 占 4 个扇区, bootsect 和 setup 已经读取
head:  .word 0                        # current head
track: .word 0                        # 当前磁道

read_it:
  mov  %es, %ax
  test $0x0fff, %ax                   # 此时 ax = 0x1000, 指令表示 (0x0fff & 0x1000) = 0, ZF 标志位清零,该标志位影响下面的指令
die:
  jne die                             # 这三个汇编指令表示，system 必须放在 64k (0x1000 << 4) 边界处,否则死循环
  xor  %bx, %bx                       # 将 bx 寄存器清零，后面使用 bx 寄存器存放段内偏移
rp_read:
  mov  %es, %ax                       # es 寄存器会指向正在复制的地址，后面的指令检查是否已经复制完
  cmp  $ENDSEG, %ax                   # 检查是否已经到达结束地址
  jb   ok1_read                       # 如果 ax 小于 ENDSEG 跳转到 ok1_read ,否则 ret 结束
  ret
ok1_read:
  mov  %cs:sectors+0, %ax             # 获取每个磁道的扇区数,保存至 ax
  sub  sread, %ax                     # 计算当前磁道还要读取的扇区数 ax = ax - sread
  mov  %ax, %cx                       # 将当前磁道还要读取的扇区数保存至计数寄存器 cx 中
  shl  $9, %cx                        # cs = cs << 9,每个扇区为 512 字节, 即 2^9 ,这里保存读取的字节数
  add  %bx, %cx                       # 目标地址增加 cx, 因为目标地址是 es: bx 
  jnc  ok2_read                       # 检查进位标志 CF ,CF = 0 时跳转 ok2_read
  je   ok2_read                       # 如果相等跳转 ok2_read
  xor  %ax, %ax
  sub  %bx, %ax
  shr  $9, %ax
ok2_read:
  call read_track                     # 读当前磁道指定的开始扇区和需要读取的扇区,真正的读操作
  mov  %ax, %cx                       # 从 read_track 运行结果中获取刚刚读到的扇区数量
  add  sread, %ax                     # 将当前磁道已读取扇区数量增加 sread = sread + ax
  #seg cs
  cmp  %cs:sectors+0, %ax             # 看看这个磁道读完了没有
  jne  ok3_read                       # ax != 磁道扇区数， 跳转至 ok3_read 继续读取当前磁道
  mov  $1, %ax                        # ax == 磁道扇区数，设置为 1 准备读取下一个磁道
  sub  head, %ax                      # 判断当前磁头号，如果是 0 磁头，则取读取 1 号磁头，否则就可以取读取下一个磁道了
  jne  ok4_read                       # 不想等跳转到 ok4_read
  incw track                          # 当前磁道递增
ok4_read:
  mov  %ax, head                      # 保存磁头号
  xor  %ax, %ax                       # 清零
ok3_read:
  mov  %ax, sread                     # 加载当前磁道已读扇区数
  shl  $9, %cx                        # 上次已读数据
  add  %cx, %bx                       # 调整目的地址段内偏移, 目的地址 (es << 4 + bx)
  jnc  rp_read                        # 检查有没有进位，如果进位了，段 + 1，清零段偏移
  mov  %es, %ax                       # 下面几句完成段 + 1
  add  $0x1000, %ax
  mov  %ax, %es
  xor  %bx, %bx                       # 段内偏移清零
  jmp  rp_read

# 读当前磁道开始扇区和需要读的扇区数到 es:bx 处
read_track:
  push %ax
  push %bx
  push %cx
  push %dx
  mov  track, %dx                     # 加载当前磁道道 dx 寄存器
  mov  sread, %cx                     # 加载当前扇区道 cx 寄存器
  inc  %cx                            # 当前扇区 + 1
  mov  %dl, %ch                       # 当前磁道号, dl 是 dx 寄存器的低 8 位
  mov  head, %dx                      # 取当前磁头号
  mov  %dl, %dh                       # dh 保存磁头号, dh dl 是 dx 寄存器的高 8 位和低 8 位
  mov  $0, %dl                        # 驱动器号
  and  $0x0100, %dx                   # 磁头号不大于 1
  mov  $2, %ah                        # ah = 2 表示调用读磁盘扇区, al 中保存需要读的扇区数量,在 ok1_read 中设置了
  int  $0x13                          # 发起读磁盘扇区功能
  jc   bad_rt                         # 如果出错，调用 bad_rt
  pop  %dx
  pop  %cx
  pop  %bx
  pop  %ax
  ret

bad_rt:
  mov  $0, %ax
  mov  $0, %dx
  int  $0x13
  pop  %dx
  pop  %cx
  pop  %bx
  pop  %ax
  jmp  read_track

# 关闭马达
kill_motor:
  push  %dx
  mov   $0x3f2, %dx
  mov   $0, %al
  outsb
  pop   %dx
  ret

sectors:                              # 每个磁道的扇区数
  .word 0

msg1:
  .byte 13, 10
  .ascii "IceCityOS is booting ..."
  .byte 13, 10, 13, 10

.org 508                              # 此处经过编译后是地址 508 处, 通过后面的标识 55 aa，BIOS 就会认为这是 主引导记录MBR 代码，会将 bootsect 所在的 512 byte加载到内存中运行
root_dev:
  .word ROOT_DEV
.org 510
boot_flag:
  .word 0xAA55

