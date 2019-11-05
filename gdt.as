global flush_tlb
extern serial_write

textbuff:
	dd "Flushing tlb", 10, 0

flush_tlb:
	mov eax, cr3
	mov cr3, eax
	push textbuff
	call serial_write
	pop eax
	ret

global invl_pg

invl_pg:
	pop eax
	invlpg [eax]
	push eax
	ret

global gdt_flush

gdt_flush:
	mov eax, [esp + 4]
	lgdt [eax]

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	jmp 0x08:.flush
.flush:
	ret

global load_idt
global irqdefm
global irqdefs
global irq1
global page_except

extern irqmaster_handler
extern irqslave_handler
extern irq1_handler
extern page_except_handler

load_idt:
	mov edx, [esp+4]
	lidt [edx]
	sti
	ret

irqdefm:
	pusha
	call irqmaster_handler
	popa
	iret

irqdefs:
	pusha
	call irqslave_handler
	popa
	iret

irq1:
	pusha
	call irq1_handler
	popa
	iret

page_except:
	push eax
	mov eax, [esp + 4]
	pusha
	push eax
	mov eax, cr2
	push eax
	call page_except_handler
	pop eax
	pop eax

	popa
	pop eax
	iret