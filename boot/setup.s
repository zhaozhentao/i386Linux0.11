.global _start

_start:

# 打印 Now we are in setup ...
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
  .byte 13,10,13,10


