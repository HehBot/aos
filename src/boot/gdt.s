; GDT
.start:

.null:
dd 0x0
dd 0x0

.code_seg:
; base=0x0, limit=0xffff
; 1st flags: (present)1 (privilege)00 (descriptor type)1 -> 1001b
; type flags: (code)1 (conforming)0 (readable)1 (accessed)0 -> 1010b
; 2nd flags: (granularity)1 (32-bit default)1 (64-bit seg)0 (AVL)0 -> 1100b
dw 0xffff       ; Limit (bits 0-15)
dw 0x0          ; Base (bits 0-15)
db 0x0          ; Base (bits 16-23)
db 10011010b    ; 1st flags, type flags
db 11001111b    ; 2nd flags, Limit (bits 16-19)
db 0x0          ; Base (bits 24-31)

.data_seg:
; same as code segment GDT except for type flags
; type flags: (code)0 (expand down)0 (writeable)1 (accessed)0 -> 0010b
dw 0xffff       ; Limit (bits 0-15)
dw 0x0          ; Base (bits 0-15)
db 0x0          ; Base (bits 16-23)
db 10010010b    ; 1st flags, type flags
db 11001111b    ; 2nd flags, Limit (bits 16-19)
db 0x0          ; Base (bits 24-31)

.end:

; GDT descriptor
.descriptor:
dw .end - .start - 1;
dd .start
