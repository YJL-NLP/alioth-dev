;---------------------------
; memory.asm
; 作者 : 王雨泽
; 为alioth提供运行时支持的基础库
; 平台: x86_64-pc-linux-gnu
;---------------------------

global method.memory.alloc.Vi32
global method.memory.memset.P0i8.Vi32.Vi32
global method.memory.free.P0i8
global class.memory.entity

VHEAP_SIZE equ 102400
using equ 0x80_00_00_00
free equ 0x7F_FF_FF_FF

[section .bss]
class.memory.entity resb 1
vheap resb VHEAP_SIZE

[section .text]
[bits 64]
[default rel]

; method memory::init() void
method.memory.init:
    mov dword [vheap], (VHEAP_SIZE>>3) & free
    ret

; method memory::alloc( count int32 ) *int8
; @param count : RSI
method.memory.alloc.Vi32:
    push rbp
    push rbx

    xchg rdi, rsi

    add rdi, 11 ; 4+7 --- 4 节点头, 7 向上取整为8的整数倍
    shr rdi, 3

    lea rbp, [vheap] ; 基指针
    xor rbx, rbx ; 偏移量
    mov eax, dword [rbp+rbx*8]
    test eax, eax
    jnz .L
    call method.memory.init

    .L:
        mov eax, [rbp+rbx*8] ; 节点头
        test eax, using
        jnz .N
        cmp eax, edi
            jg .DG ; 拆解大块
            je .DE ; 大小正好的块
        .N: ; 块被占用或不够大
            add ebx, eax
            and ebx, free
            cmp ebx, VHEAP_SIZE>>3
    jl .L

    .F: 
        xor rax, rax
    .E:
        pop rbx
        pop rbp
        ret
    .DG:
        add ebx, edi
        mov dword [rbp+rbx*8], eax
        sub dword [rbp+rbx*8], edi
        sub ebx, edi
        mov dword [rbp+rbx*8], edi
    .DE:
        or dword [rbp+rbx*8], using
        lea rax, [rbp+rbx*8+4]
        jmp .E

; method memory::memset( p *int8, v int8, c int32 ) void
; @param p : RSI
; @param v : RDX
; @param c : RCX
method.memory.memset.P0i8.Vi32.Vi32:
    push rax
    push rcx

    xchg rdi, rsi
    xchg rsi, rdx
    xchg rdx, rcx

    mov rax, rsi
    mov ecx, edx
    cld
    rep stosb

    pop rcx
    pop rax
    ret

; method memory::free( target *int8 ) bool
; @param target : RSI
method.memory.free.P0i8:
    push rbp
    push rbx
    push rsi

    xchg rdi, rsi

    sub rdi, 4
    lea rbp, [vheap]
    xor rbx, rbx

    .L:
        lea rsi, [rbp+rbx*8]
        cmp rdi, rsi
            jz .FD
        mov rax, rbx ; 保存前驱
        add ebx, [rbp+rbx*8]
        and ebx, free
        cmp ebx, VHEAP_SIZE>>3
    jl .L

    .F:
        xor rax, rax
    .E:
        pop rsi
        pop rbx
        pop rbp
        ret
    .FD:
        and dword [rbp+rbx*8], free
        test ebx, ebx
            jz .AF
        test dword [rbp+rax*8], using
            jnz .AF
        mov ebx, dword [rbp+rbx*8]
        add dword [rbp+rax*8], ebx
        mov rbx, rax
        .AF:
        mov eax, ebx
        add eax, dword [rbp+rbx*8] ; 装载后继
        test dword [rbp+rax*8], using
            jnz .E
        mov eax, dword [rbp+rax*8]
        add dword [rbp+rbx*8], eax
        jmp .E