;---------------------------
; io.asm
; 作者 : 王雨泽
; 为alioth提供运行时支持的基础库
; 平台: x86_64-pc-linux-gnu
;---------------------------

global method.io.println.P0i8
global method.io.print.P0i8
global method.io.getline.P0i8.Vi32
global method.io.block
global method.io.nblock
global class.io.entity

[section .bss]
class.io.entity resb 1
termios resb 60
fl resb 4

[section .text]
[bits 64]

fcntl.getfl:
    push rax
    push rdi
    push rsi
    push rdx
    mov rax, 72
    mov rdi, 0
    mov rsi, 3
    mov rdx, 0
    syscall
    mov [fl], eax
    pop rdx
    pop rsi
    pop rdi
    pop rax
    ret

fcntl.setfl:
    push rax
    push rdi
    push rsi
    push rdx
    mov rax, 72
    mov rdi, 0
    mov rsi, 4
    mov rdx, [fl]
    syscall
    pop rdx
    pop rsi
    pop rdi
    pop rax
    ret

termios.get:
    push rax
    push rdi
    push rsi
    push rdx
    mov rax, 16
    mov rdi, 0
    mov rsi, 0x5401
    lea rdx, [termios]
    syscall
    pop rdx
    pop rsi
    pop rdi
    pop rax
    ret

termios.set:
    push rax
    push rdi
    push rsi
    push rdx
    mov rax, 16
    mov rdi, 0
    mov rsi, 0x5402
    lea rdx, [termios]
    syscall
    pop rdx
    pop rsi
    pop rdi
    pop rax
    ret

block.enable:
    call termios.get
    or dword [termios+12], 2
    call termios.set
    call fcntl.getfl
    and dword [fl], ~2048
    call fcntl.setfl
    ret

block.disable:
    call termios.get
    and dword [termios+12], ~2
    call termios.set
    call fcntl.getfl
    or dword [fl], 2048
    call fcntl.setfl
    ret

builtin.strlen:
    push rcx
    xor rcx, rcx
    .L:
        mov al, [rdi+rcx]
        test al, al
        jz .E
        inc rcx
        jmp .L
    .E:
    mov rax, rcx
    pop rcx
    ret

method.io.println.P0i8:
    push rdx
	mov rdi, rsi
    call builtin.strlen
    mov rdi, 1
    mov rdx, rax
    mov rax, 1
    syscall
    push 0
    mov byte[rsp], `\n`
    mov rdi, 1
    lea rsi, [rsp]
    mov rdx, 2
    mov rax, 1
    syscall
    add rsp, 8
    pop rdx
    ret

method.io.print.P0i8:
    push rdx
	mov rdi, rsi
    call builtin.strlen
    mov rdi, 1
    mov rdx, rax
    mov rax, 1
    syscall
    pop rdx
    ret

method.io.getline.P0i8.Vi32:
    test dword[rdi], 0x1
    jnz .G
        call block.disable
        xor rax, rax
        xor rdi, rdi
        push rsi
        push rdx
        syscall
        pop rdx
        pop rsi
        sub edx, eax
        add rsi, rax
        cmp eax, 0
        jle .E
    .G:
        call block.enable
        xor rdi, rdi
        xor rax, rax
        syscall
        test rax, rax
        jz .E
        mov byte[rsi+rax-1], 0
    .E:
    ret

method.io.block:
    mov dword[rdi], 1
    call block.enable
    ret
method.io.nblock:
    mov dword[rdi], 0
    call block.disable
    ret