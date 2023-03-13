.code16
.global _start

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

msg2:
  .byte 13, 10
  .ascii "Now we are in setup ..."
  .byte 13, 10, 13, 10

