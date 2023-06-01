[bits 32]
[global _start]
[extern isr_install]
[extern init_screen]
[extern clear_screen]
[extern main]

[extern kernel_virtual_start]
[extern kernel_virtual_end]
[extern kernel_physical_start]
[extern kernel_physical_end]
[extern screen_physical_start]
[extern screen_virtual_start]
[extern page_dir_physical_start]
[extern page_dir_virtual_start]

_start:
        ; initalise paging
        mov     eax, page_dir_physical_start
        mov     cr3, eax
        mov     ebx, cr4
        or      ebx, 0x00000010
        mov     cr4, ebx
        mov     ebx, cr0
        or      ebx, 0x80000000
        mov     cr0, ebx

        jmp     _start.entry
.entry:
;         mov     dword [page_dir_virtual_start], 0x00000002

        ; move kernel stack
        mov     ax, 0x10
        mov     ebp, 0xc0007ff0
        mov     ss, ax
        mov     esp, ebp

        call    isr_install

        push    kernel_physical_end
        push    kernel_physical_start
        push    kernel_virtual_end
        push    kernel_virtual_start
        push    screen_physical_start
        push    screen_virtual_start
        push    page_dir_physical_start
        push    page_dir_virtual_start
        call    main

        jmp     $
