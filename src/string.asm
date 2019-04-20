;---------------------------
; alioth.asm
; 作者 : 王雨泽
; 为alioth提供运行时支持的基础库
; 平台: x86_64-pc-linux-gnu
;---------------------------

global method.string.from.P0i8.Vu32.Vi32
global method.string.toInt32.P0i8

global class.string.entity

[section .bss]
class.string.entity resb 1

[section .text]
[bits 64]
    
; method string::from( buf *int8, data int32, max int32 ) int32
; @param buf : RSI
; @param data : RDX
; @param max : RCX
method.string.from.P0i8.Vu32.Vi32:
    push rdx
    push rcx
    push rbx

    xchg rdi, rsi
    xchg rsi, rdx
    xchg rdx, rcx

    xor rbx, rbx

    cmp si, 10
    jb .1
    cmp si, 100
    jb .2
    cmp si, 1000
    jb .3
    cmp si, 10000
    jb .4
    .5 : inc rdi
        inc rbx
    .4 : inc rdi
        inc rbx
    .3 : inc rdi
        inc rbx
    .2 : inc rdi
        inc rbx
    .1 : inc rdi
        inc rbx
    mov byte [rdi], 0
    dec rdi

    test si, si
    jz .Z
        mov rcx, 10
        mov rax, rsi
        .L:
            xor rdx, rdx
            div word cx
            mov byte [rdi], dl
            add byte [rdi], '0'
            dec rdi
            test ax, ax
            jnz .L

    .E:
        mov rax, rbx
        pop rbx
        pop rcx
        pop rdx
        ret

    .Z:
        mov byte [rdi], '0'
        jmp .E

; method string::toInt32( buf *int8 ) int32
; @param buf RSI
method.string.toInt32.P0i8:
    push rdx
    push rcx
    push rbx
    push rsi

    xor rbx, rbx
    xor rax, rax
    mov ecx, 10

    .L:
        mov bl, [rsi]
        test bl, bl
            jz .E
        sub bl, '0'
        mul ecx
        add eax, ebx
        inc rsi
    jmp .L

    .E:
    pop rsi
    pop rbx
    pop rcx
    pop rdx
    ret
