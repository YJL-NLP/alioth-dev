;---------------------------
; alioth.asm
; 作者 : 王雨泽
; 为alioth提供运行时支持的基础库
; 平台: x86_64-pc-linux-gnu
;---------------------------

global method.exit@4.
global _start
extern start

[section .text]
[bits 64]


method.exit@4.:
	mov	eax, 60
	syscall
	ret

_start:
	pop		rdi
	lea		rsi,[rsp]
	call	start
	mov		rdi, rax
	call	method.exit@4.