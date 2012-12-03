
section .data

section .text
global _start


_start:
	push EBX
	mov EBX, 0xE
	pop EBX

	push eax
	push ebx
	push ecx
	push edx

	push esi
	push edi
	push ebp
	push esp

	pop eax
	pop ebx
	pop ecx
	pop edx

	pop esi
	pop edi
	pop ebp
	pop esp

	ret

	;mov eax, 1 ; sys_exit
	;mov ebx, 0 ; with 0
	;int 0x80 ; system call


