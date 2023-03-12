.code16                               # 开机时处于 16 位的实模式运行 bootsect 需要以 16 位编译

.global _start 

.equ SYSSIZE, 0x3000
.equ SETUPLEN, 4                      # nr of setup 模块的扇区数
.equ BOOTSEG, 0x07c0                  # 启动扇区代码的起始地址 0x7c00
.equ INITSEG, 0x9000                  # 启动代码将会搬到 0x90000 这个位置, 注意是 INITSEG 左移 4 位
.equ SYSSEG, 0x1000                   # system 模块将会被搬到这个位置 0x10000 (65536).
.equ ENDSEG, SYSSEG + SYSSIZE         # where to stop loading
.equ ROOT_DEV, 0x301

_start:
  mov  $BOOTSEG, %ax                  # 将 ds 数据段寄存器设置为0x7C0
  mov  %ax, %ds
  mov  $INITSEG, %ax                  # 将 es 附加段段寄存器设置为0x900
  mov  %ax, %es
  mov  $256, %cx                      # 设置移动计数值 256 字,即 512 字节
  sub  %si, %si                       # si = si - si 表示将 si 寄存器清零, 因为在当前实模式下 寻址方式为 ds << 4 + si 即 0x7c00 + 0x0
  sub  %di, %di                       # 同上 目标地址为 es << 4 + di 即 0x90000 + 0x0
  rep  movsw                          # 重复执行并递减 cx 的值, 将 (e)cx 个字从 ds:[(e)si] 移到 es:[(e)di]
  ljmp $INITSEG, $go                  # 段间跳转，这里 INITSEG 指出跳转到的段地址，跳转后 cs 代码段寄存器的值为0x9000

go:
  mov  %cs, %ax                       # 将 ds，es，ss 都设置成移动后代码所在的段处 (0x9000)
  mov  %ax, %ds
  mov  %ax, %es
# put stack at 0x9ff00.               # 栈指针通过 ss:sp 确定
  mov  %ax, %ss
  mov  $0xFF00, %sp                   # arbitrary value >> 512

load_setup:                           # 将 setup 模块加载到 bootsect 模块后, 即 0x90200 地址处
  mov  $0x0000, %dx                   # drive 0, head 0
  mov  $0x0002, %cx                   # sector 2, track 0
  mov  $0x0200, %bx                   # address = 512, in INITSEG
  .equ AX, 0x0200 + SETUPLEN
  mov  $AX, %ax                       # service 2, nr of sectors
  int  $0x13                          # 使用 BIOS 读取磁盘功能
  jnc  ok_load_setup                  # 成功读取磁盘内容，跳转到 ok_load_setup
  mov  $0x0000, %dx
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
# BIOS 中的打印程序会根据当前的 es 附加段寄存器去寻找 msg1 位置然后打印
  mov  $0x03, %ah                     # 读取光标位置
  xor  %bh, %bh
  int  $0x10

  mov  $30, %cx
  mov  $0x0007, %bx                   # page 0, attribute 7 (normal)
  mov  $msg1, %bp
  mov  $0x1301, %ax                   # write string, move cursor
  int  $0x10

# 接下来将 system 模块搬到 0x10000 处
  mov  $SYSSEG, %ax
  mov  %ax, %es                       # segment of 0x010000
  call read_it
  call kill_motor                     # 关闭马达

# 此处存放几个变量,用于下面的 read_it 程序
sread: .word 1 + SETUPLEN             # 当前扇区从 build.sh 中可以看到 bootsect 占 1 个扇区, setup 占 4 个扇区, bootsect 和 setup 已经读取
head:  .word 0                        # current head
track: .word 0                        # current track

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
  add  %bx, %cx                       # 这里 bx 应该为 0 ??
  jnc  ok2_read                       # 检查进位标志 CF ,CF = 0 时跳转 ok2_read
  je   ok2_read                       # 如果相等跳转 ok2_read
  xor  %ax, %ax
  sub  %bx, %ax
  shr  $9, %ax
ok2_read:
  call read_track
  mov  %ax, %cx
  add  sread, %ax
  #seg cs
  cmp  %cs:sectors+0, %ax
  jne  ok3_read
  mov  $1, %ax
  sub  head, %ax
  jne  ok4_read
  incw track
ok4_read:
  mov  %ax, head
  xor  %ax, %ax
ok3_read:
  mov  %ax, sread
  shl  $9, %cx
  add  %cx, %bx
  jnc  rp_read
  mov  %es, %ax
  add  $0x1000, %ax
  mov  %ax, %es
  xor  %bx, %bx
  jmp  rp_read
read_track:
  push %ax
  push %bx
  push %cx
  push %dx
  mov  track, %dx
  mov  sread, %cx
  inc  %cx
  mov  %dl, %ch
  mov  head, %dx
  mov  %dl, %dh
  mov  $0, %dl
  and  $0x0100, %dx
  mov  $2, %ah
  int  $0x13
  jc   bad_rt
  pop  %dx
  pop  %cx
  pop  %bx
  pop  %ax
  ret
bad_rt:	mov	$0, %ax
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
boot_flag:
  .word 0xAA55

