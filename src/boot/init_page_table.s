; Phy0x8000
align 4096
PAGE_DIR:
dd 0x0009003
times 767 dd 0x00000002
dd 0x0009003                    ; PDE 768
times 255 dd 0x00000002

; for addresses 0xc0000000-0xc0008fff mapped to Phy0x0-Phy0x8fff
; these are in 4kB pages 0xc0000-0xc0008
; these are in PDE 0b1100000000 (=768), PTE 0b0000000000-0b0000001000
; for addresses 0xc0009000-0xc0009fff mapped to Phy0xb8000-Phy0xb8fff
; these are in 4kB page 0xc0009
; these are in PDE 0b1100000000 (=768), PTE 0b0000001001
TEMP_PAGE_TABLE:
dd 0x00000003
dd 0x00001003
dd 0x00002003
dd 0x00003003
dd 0x00004003
dd 0x00005003
dd 0x00006003
dd 0x00007003
dd 0x00008003
dd 0x000b8003
times 1014 dd 0x00000002
