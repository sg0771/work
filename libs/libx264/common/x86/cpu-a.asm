;*****************************************************************************
;* cpu-a.asm: x86 cpu utilities
;*****************************************************************************
;* Copyright (C) 2003-2018 x264 project
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr>
;*          Loren Merritt <lorenm@u.washington.edu>
;*          Fiona Glaser <fiona@x264.com>
;*
;* This program is free software; you can redistribute it and/or modify
;* it under the terms of the GNU General Public License as published by
;* the Free Software Foundation; either version 2 of the License, or
;* (at your option) any later version.
;*
;* This program is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;* GNU General Public License for more details.
;*
;* You should have received a copy of the GNU General Public License
;* along with this program; if not, write to the Free Software
;* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
;*
;* This program is also available under a commercial proprietary license.
;* For more information, contact us at licensing@x264.com.
;*****************************************************************************

%include "./common/x86/x86inc.inc"


SECTION .text


%if __BITS__ == 32

cglobal endian_fix32
        push    ebp                                     ; 0000 _ 55
		mov     ebp, esp                                ; 0001 _ 89. E5
        mov     eax, dword [ebp+8H]                     ; 0003 _ 8B. 45, 08
        bswap   eax                                     ; 0006 _ 0F C8
        mov     dword [ebp+8H], eax                     ; 0008 _ 89. 45, 08
        mov     eax, dword [ebp+8H]                     ; 000B _ 8B. 45, 08
        pop     ebp                                     ; 000E _ 5D
        ret                                             ; 000F _ C3

cglobal endian_fix64
        push    ebp                                     ; 0010 _ 55
        mov     ebp, esp                                ; 0011 _ 89. E5
        sub     esp, 8                                  ; 0013 _ 83. EC, 08
        mov     eax, dword [ebp+8H]                     ; 0016 _ 8B. 45, 08
        mov     dword [ebp-8H], eax                     ; 0019 _ 89. 45, F8
        mov     eax, dword [ebp+0CH]                    ; 001C _ 8B. 45, 0C
        mov     dword [ebp-4H], eax                     ; 001F _ 89. 45, FC
        mov     eax, dword [ebp-8H]                     ; 0022 _ 8B. 45, F8
        mov     edx, dword [ebp-4H]                     ; 0025 _ 8B. 55, FC
        bswap   eax                                     ; 0028 _ 0F C8
        mov     dword [ebp-8H], eax                     ; 002A _ 89. 45, F8
        mov     dword [ebp-4H], edx                     ; 002D _ 89. 55, FC
        mov     eax, dword [ebp-8H]                     ; 0030 _ 8B. 45, F8
        mov     edx, dword [ebp-4H]                     ; 0033 _ 8B. 55, FC
        leave                                           ; 0036 _ C9
        ret                                             ; 0037 _ C3

cglobal prefetch
        push    ebp                                     ; 0038 _ 55
        mov     ebp, esp                                ; 0039 _ 89. E5
        mov     eax, dword [ebp+8H]                     ; 003B _ 8B. 45, 08
        prefetcht0 [eax]                                ; 003E _ 0F 18. 08
        nop                                             ; 0041 _ 90
        pop     ebp                                     ; 0042 _ 5D
        ret                                             ; 0043 _ C3

%else  ; ARCH_X64

cglobal endian_fix32
        push    rbp                                     ; 0000 _ 55
        mov     rbp, rsp                                ; 0001 _ 48: 89. E5
        mov     dword [rbp+10H], ecx                    ; 0004 _ 89. 4D, 10
        mov     eax, dword [rbp+10H]                    ; 0007 _ 8B. 45, 10
        bswap   eax                                     ; 000A _ 0F C8
        mov     dword [rbp+10H], eax                    ; 000C _ 89. 45, 10
        mov     eax, dword [rbp+10H]                    ; 000F _ 8B. 45, 10
        pop     rbp                                     ; 0012 _ 5D
        ret                                             ; 0013 _ C3

cglobal endian_fix64
        push    rbp                                     ; 0014 _ 55
        mov     rbp, rsp                                ; 0015 _ 48: 89. E5
        mov     qword [rbp+10H], rcx                    ; 0018 _ 48: 89. 4D, 10
        mov     rax, qword [rbp+10H]                    ; 001C _ 48: 8B. 45, 10
        bswap   rax                                     ; 0020 _ 48: 0F C8
        mov     qword [rbp+10H], rax                    ; 0023 _ 48: 89. 45, 10
        mov     rax, qword [rbp+10H]                    ; 0027 _ 48: 8B. 45, 10
        pop     rbp                                     ; 002B _ 5D
        ret                                             ; 002C _ C3
; endian_fix64 End of function

cglobal prefetch
        push    rbp                                     ; 002D _ 55
        mov     rbp, rsp                                ; 002E _ 48: 89. E5
        mov     qword [rbp+10H], rcx                    ; 0031 _ 48: 89. 4D, 10
        mov     rax, qword [rbp+10H]                    ; 0035 _ 48: 8B. 45, 10
        prefetcht0 [rax]                                ; 0039 _ 0F 18. 08
        nop                                             ; 003C _ 90
        pop     rbp                                     ; 003D _ 5D
        ret                                             ; 003E _ C3
; x264_prefetch End of function


%endif
;-----------------------------------------------------------------------------
; void x264_cpu_cpuid( int op, int *eax, int *ebx, int *ecx, int *edx )
;-----------------------------------------------------------------------------
cglobal cpu_cpuid, 5,7
    push rbx
    push  r4
    push  r3
    push  r2
    push  r1
    mov  eax, r0d
    xor  ecx, ecx
    cpuid
    pop   r4
    mov [r4], eax
    pop   r4
    mov [r4], ebx
    pop   r4
    mov [r4], ecx
    pop   r4
    mov [r4], edx
    pop  rbx
    RET

;-----------------------------------------------------------------------------
; uint64_t x264_cpu_xgetbv( int xcr )
;-----------------------------------------------------------------------------
cglobal cpu_xgetbv
    movifnidn ecx, r0m
    xgetbv
%if __BITS__ == 64
    shl       rdx, 32
    or        rax, rdx
%endif
    ret

;-----------------------------------------------------------------------------
; void x264_cpu_emms( void )
;-----------------------------------------------------------------------------
cglobal cpu_emms
    emms
    ret

;-----------------------------------------------------------------------------
; void x264_cpu_sfence( void )
;-----------------------------------------------------------------------------
cglobal cpu_sfence
    sfence
    ret

%if __BITS__ == 64

;-----------------------------------------------------------------------------
; intptr_t x264_stack_align_asm( void (*func)(void*), ... ); (up to 5 args)
;-----------------------------------------------------------------------------
cvisible stack_align_asm
    mov      rax, r0mp
    mov       r0, r1mp
    mov       r1, r2mp
    mov       r2, r3mp
    mov       r3, r4mp
    mov       r4, r5mp
    push     rbp
    mov      rbp, rsp
%if WIN64
    sub      rsp, 40 ; shadow space + r4
%endif
    and      rsp, ~(STACK_ALIGNMENT-1)
%if WIN64
    mov [rsp+32], r4
%endif
    call     rax
    leave
    ret

%else

;-----------------------------------------------------------------------------
; int x264_cpu_cpuid_test( void )
; return 0 if unsupported
;-----------------------------------------------------------------------------
cglobal cpu_cpuid_test
    pushfd
    push    ebx
    push    ebp
    push    esi
    push    edi
    pushfd
    pop     eax
    mov     ebx, eax
    xor     eax, 0x200000
    push    eax
    popfd
    pushfd
    pop     eax
    xor     eax, ebx
    pop     edi
    pop     esi
    pop     ebp
    pop     ebx
    popfd
    ret

cvisible stack_align_asm
    push      ebp
    mov       ebp, esp
    sub       esp, 20
    and       esp, ~(STACK_ALIGNMENT-1)
    mov        r0, [ebp+12]
    mov        r1, [ebp+16]
    mov        r2, [ebp+20]
    mov  [esp+ 0], r0
    mov  [esp+ 4], r1
    mov  [esp+ 8], r2
    mov        r0, [ebp+24]
    mov        r1, [ebp+28]
    mov  [esp+12], r0
    mov  [esp+16], r1
    call [ebp+ 8]
    leave
    ret

%endif
