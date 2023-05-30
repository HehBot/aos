[bits 32]
[global _start]
[extern isr_install]
[extern clear_screen]
[extern main]
_start:
        call    isr_install
        call    clear_screen
        call    main
        jmp     $
