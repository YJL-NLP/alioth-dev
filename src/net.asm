;---------------------------
; net.asm
; 作者 : 王雨泽
; 为alioth提供运行时支持的基础库
; 平台: x86_64-pc-linux-gnu
;---------------------------

global method.SockAddr.CreateLocalHost.Vi32
global method.SockAddr.CreateBroadCast.Vi32
global method.Socket.Create.R.SockAddr
global method.Socket.recvFrom.R.SockAddr.P0i8.Vi32
global method.Socket.sendTo.R.SockAddr.P0i8.Vi32
global method.SockAddr.toReadable.P0i8.P0u32

extern method.string.from.P0i8.Vu32.Vi32

[section .text]
[bits 64]

; rdi 
net.htons:
    mov rax, 1
    push rax
    test byte[rsp], 1
    pop rax
    mov rax, rdi
    jz .E
    xchg al, ah
    .E:
    ret

net.htonl:
    mov rax, 1
    push rax
    test byte[rsp], 1
    pop rax
    mov rax, rdi
    jz .E
    xchg al, ah
    rol eax, 16
    xchg al, ah
    .E:
    ret

net.socket:
    push rdi
    push rsi
    push rdx
    mov rdi, 2
    mov rsi, 2
    mov rdx, 0
    mov rax, 41
    syscall ; sys_socket
    pop rdx
    pop rsi
    pop rdi
    ret

; rdi fd, rsi buf, rdx size, rcx *addr
net.recvfrom:
    push r10
    push r8
    push r9

    push 16
    lea r9, [rsp]
    mov r8, rcx
    xor r10, r10
    mov rax, 45
    syscall ; sys_recvfrom

    add rsp, 8
    pop r9
    pop r8
    pop r10
    ret

; rdi fd, rsi buf, rdx size, rcx *addr
net.sendto:
    push r10
    push r8
    push r9

    mov r9, 16
    mov r8, rcx
    xor r10, r10
    mov rax, 44
    syscall ; sys_sendto

    pop r9
    pop r8
    pop r10
    ret

; rdi fd
net.setrcvtimeo:
    push rsi
    push rdx
    push r10
    push r8
    push 1
    push 0
    mov rsi, 1 ; SOL_SOCKET
    mov rdx, 20 ; SO_RCVTIMEO
    lea r10, [rsp]
    mov r8, 16
    mov rax, 54 ; sys_setsockopt
    syscall
    add rsp, 16
    pop r8
    pop r10
    pop rdx
    pop rsi
    ret

; rdi fd
net.setbroadcast:
    push rsi
    push rdx
    push r10
    push r8
    push 1
    mov rsi, 1 ; SOL_SOCKET
    mov rdx, 6 ; SO_BROADCAST
    lea r10, [rsp]
    mov r8, 4
    mov rax, 54 ; sys_setsockopt
    syscall
    add rsp, 8
    pop r8
    pop r10
    pop rdx
    pop rsi
    ret

; rdi fd
net.setreuse:
    push rsi
    push rdx
    push r10
    push r8
    push 1
    mov rsi, 1 ; SOL_SOCKET
    mov rdx, 15 ; SO_REUSEPORT
    lea r10, [rsp]
    mov r8, 4
    mov rax, 54 ; sys_setsockopt
    syscall
    mov rsi, 1 ; SOL_SOCKET
    mov rdx, 2 ; SO_REUSEADDR
    lea r10, [rsp]
    mov r8, 4
    mov rax, 54 ; sys_setsockopt
    syscall
    add rsp, 8
    pop r8
    pop r10
    pop rdx
    pop rsi
    ret

; rdi rsi
method.SockAddr.CreateLocalHost.Vi32:
    push rdi
    mov rdi, rsi
    call net.htons
    mov rsi, rax
    pop rdi
    mov dword[rdi+0], 0
    mov word[rdi+4], si
    ret

; rdi rsi
method.SockAddr.CreateBroadCast.Vi32:
    push rdi
    mov rdi, rsi
    call net.htons
    mov rsi, rax
    pop rdi
    mov dword[rdi+0], 0xFFFFFFFF
    mov word[rdi+4], si
    ret

; rdi rsi
method.Socket.Create.R.SockAddr:
    push rdx

    call net.socket
    mov dword[rdi], eax

    push rdi
    mov edi, dword[rdi]
    call net.setbroadcast
    call net.setrcvtimeo
    call net.setreuse
    pop rdi

    push 0
    push 0
    mov word[rsp+0], 2 ; AF_INET
    mov ax, word[rsi+4]
    mov word[rsp+2], ax ; port
    mov eax, dword[rsi+0]
    mov dword[rsp+4], eax ; ip

    lea rsi, [rsp]
    mov rdx, 16
    mov edi, dword[rdi]
    mov rax, 49
    syscall ; sys_bind

    add rsp, 16
    pop rdx
    ret

; rdi *Socket, rsi *SockAddr, rdx buf, rcx size
method.Socket.recvFrom.R.SockAddr.P0i8.Vi32:
    push rsi
    push 0
    push 0
    lea rsi, [rsp] ; addr
    xchg rsi, rdx
    xchg rdx, rcx
    mov edi, dword[rdi] ; fd
    call net.recvfrom
    mov rdi, [rsp+16] ; 重装SockAddr指针
    mov si, word[rsp+2]
    mov [rdi+4], si
    mov esi, dword[rsp+4]
    mov [rdi], esi
    add rsp, 24
    ret

; rdi *Socket, rsi *SockAddr, rdx buf, rcx size
method.Socket.sendTo.R.SockAddr.P0i8.Vi32:
    push 0
    push 0
    mov word[rsp], 2 ; 填写sockaddr_in
    mov ax, word[rsi+4]
    mov word[rsp+2], ax
    mov eax, dword[rsi]
    mov dword[rsp+4], eax

    lea rsi, [rsp]
    xchg rsi, rdx
    xchg rcx, rdx
    mov edi, dword[rdi]
    call net.sendto
    
    add rsp, 16
    ret

; rdi *SockAddr, rsi buf, rdx *port
method.SockAddr.toReadable.P0i8.P0u32:
    push rax
    push rdi
    
    mov di, word[rdi+4] ; 计算端口号
    call net.htons
    mov word[rdx], ax

    mov rdi, [rsp]
    mov rdi, [rdi]
    push rsi ; 保存buf指针
    push rdi ; 备份IP数字
    mov rcx, 16 ; 不会变的参数
    xor rdx, rdx ; 清空数字槽

    mov dl, byte[rsp+0]
    call method.string.from.P0i8.Vu32.Vi32

    add [rsp+8], rax
    mov rsi, [rsp+8]
    mov byte[rsi], '.'
    inc rsi
    inc byte[rsp+8]
    mov dl, byte[rsp+1]
    call method.string.from.P0i8.Vu32.Vi32

    add [rsp+8], rax
    mov rsi, [rsp+8]
    mov byte[rsi], '.'
    inc rsi
    inc byte[rsp+8]
    mov dl, byte[rsp+2]
    call method.string.from.P0i8.Vu32.Vi32

    add [rsp+8], rax
    mov rsi, [rsp+8]
    mov byte[rsi], '.'
    inc rsi
    inc byte[rsp+8]
    mov dl, byte[rsp+3]
    call method.string.from.P0i8.Vu32.Vi32

    pop rdi
    pop rsi
    pop rdi
    pop rax
    ret