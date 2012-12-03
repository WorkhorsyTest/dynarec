
section .data
	msg: db "Hello, world!",0xa ; msg with newline
	len: equ $ - msg
	format: .asciz "L1 = %d  L2 = %d\n"
	L1: .int 100
	L2: .int 450

section .text
global _start


_do_call:
	int 0x80 ; system call
	ret

_sys_write_stdout:
	mov eax, 0x4 ; sys_write
	mov ebx, 1 ; stdout
	ret

_sys_exit:
	mov eax, 1 ; sys_exit
	mov ebx, 0 ; with 0
	call _do_call;

_old_start:
	call _sys_write_stdout
	mov ecx, msg
	mov edx, len
	call _do_call

	call _sys_exit

_start:
	pushl L2
	pushl L1
	pushl $format
	call printf

	movl $1, %eax
	int $0x80

