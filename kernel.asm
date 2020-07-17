;; our kernel's starting point, used to invoke our C program.
bits 32                     ; tell nasm assembler to generate code to run on CPU that in 32bit mode.
section .text               ; begin text section (aka. code section).
        ;multiboot spec
        align 4
        dd 0x1BADB002       ; magic
        dd 0x00             ; flags
        dd - (0x1BADB002 + 0x00)

global start                ; tell nasm assembler where the starting point is.
global keyboard_handler
global read_port
global write_port
global load_idt

extern kernel_start         ; invoke function defined in C program.
extern keyboard_handler_main

read_port:
  mov edx, [esp + 4]        ; take the first argument to edx
  in al, dx                 ; read data from dx to al
  ret

write_port:
  mov edx, [esp + 4]        ; take the first argument to edx
  mov al, [esp + 8]         ; take the second argument to al
  out dx, al                ; output data from al to dx
  ret

load_idt:
  mov edx, [esp + 4]        ; take the first argument to edx, where this argument is a pointer to a array, where
                            ; its first element is the offset of IDT and its second argument is the segment of IDT.
  lidt [edx]                ; init IDT
  sti                       ; turn on interrupts
  ret

keyboard_handler:
  call keyboard_handler_main
  iret

start:
  cli                       ; disable interrupts
  mov esp, stack_space      ; point esp to the beginning of 8KB stack
  call kernel_start         ; call smain function
  hlt                       ; halt CPU

section .bss
resb 8192                   ; allocate 8KB memory for stack
stack_space: