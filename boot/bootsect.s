.code16                               # 开机时处于 16 位的实模式运行 bootsect 需要以 16 位编译

.global _start                        # 使 _start 标签全局可以访问

.equ BOOTSEG, 0x07c0                  # 启动扇区代码的起始地址 0x7c00
.equ INITSEG, 0x9000                  # 表示启动代码将会搬到 0x90000 这个位置, 注意是 INITSEG 左移 4 位

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

msg1:
  .byte 13, 10
  .ascii "IceCityOS is booting ..."
  .byte 13, 10, 13, 10

.org 510
boot_flag:
  .word 0xAA55

