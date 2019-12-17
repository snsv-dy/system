global flush_tlb
global set_page_directory
global enable_paging
extern serial_write
extern printf

USER_CODE_SEGMENT equ 0x1B	; offset deskryptora segmentu kodu w gdt dla poziomu przywileju równego 3
USER_DATA_SEGMENT equ 0x23	; analogicznie jak wyżej, tylko dla segmentu danych
KERNEL_CODE_SEGMENT equ 0x08
KERNEL_DATA_SEGMENT equ 0x10

global breakme

breakme:	
	iret

global get_eip
get_eip:
	pop eax
	jmp eax



extern syscall_handler
global syscall_handled
;global syscall_iret

ESP_POS equ 32 ; pozycja esp w strukturze 
global syscall 		; Zakładam, że przy syscallu zawsze jest zmiana poziomu uprzywilejowania
syscall:
	pushad
	call save_registers
	popad
	; pobieranie eip i esp ze stosu
	; które wstawił tam procesor podczas przerwania
	mov edx, esp
	push dword[edx]		; tu jest eip

	mov edx, esp
	add edx, 12 + 4
	push dword [edx]	; tu jest esp

	push ecx
	push ebx
	push eax
	call syscall_handler
	; w eax jest wskaźnik do struktury proc_state
syscall_handled:
	pop edx
	pop edx
	pop edx
	pop edx
	pop edx
	;push eax
	;popad
	;pop eax
	cmp dword [eax + ESP_POS + 8], 0
	jne user_realm_sc
	mov edx, [eax + ESP_POS]	; esp w wątku kernela, tam trzeba wstawić dane do ireta
	
	;mov ebp, esp
	mov esp, edx
	pushf
	pop ebx
	or ebx, 0x200
	push ebx

	mov ebx, KERNEL_CODE_SEGMENT
	push ebx

	mov ebx, [eax + ESP_POS + 4]
	push ebx

	mov bx, KERNEL_DATA_SEGMENT
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	;call load_registers
	mov edi, [eax]
	mov esi, [eax + 4]
	mov ebp, [eax + 8]
	;mov esp, [eax + 12]
	mov ebx, [eax + 16]
	mov edx, [eax + 20]
	mov ecx, [eax + 24]
	mov eax, [eax + 28]

	iret

user_realm_sc:
	;push eax
	;popad
	;pop eax

	mov ebx, USER_DATA_SEGMENT
	push ebx

	mov ebx, [eax + ESP_POS]
	push ebx

	pushf
	pop ebx
	or ebx, 0x200
	push ebx

	mov ebx, USER_CODE_SEGMENT
	push ebx

	mov ebx, [eax + ESP_POS + 4]
	push ebx

	mov bx, USER_DATA_SEGMENT
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	mov edi, [eax]
	mov esi, [eax + 4]
	mov ebp, [eax + 8]
	;mov esp, [eax + 12]
	mov ebx, [eax + 16]
	mov edx, [eax + 20]
	mov ecx, [eax + 24]
	mov eax, [eax + 28]

	iret


multitasking_not_set_sc:

popad
syscall_iret:
	iret


prot_format:
	db "[ENTER PROTECTED] page_dir: %x", 0xA, 0x0

global interrupt_return

interrupt_return:
	cli
	mov ebp, esp
	; pop eax żeby usunąć adres powrotu?
	cmp dword [ebp + 4 + 8], 0
	je kernel_stack
	mov ax, USER_DATA_SEGMENT
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	mov eax, USER_DATA_SEGMENT
	push eax
	mov eax, [ebp + 4]
	push eax
	
	pushf	; push eflags
	pop eax
	or eax, 0x200
	push eax

	mov eax, USER_CODE_SEGMENT
	push eax
	mov eax, [ebp + 4 + 4]
	push eax
	jmp ir_end

kernel_stack:
	mov ax, KERNEL_DATA_SEGMENT
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	pushf	; push eflags
	pop eax
	or eax, 0x200
	push eax

	mov eax, KERNEL_CODE_SEGMENT
	push eax
	mov eax, [ebp + 4 + 4]
	push eax

	;mov eax, [ebp + 4]
	;mov esp, eax

ir_end:
	iret


global get_esp

get_esp:
	push ebp
	mov ebp, esp

	mov eax, esp

	mov esp, ebp
	pop ebp

	ret

set_page_directory:
	; w cr3 musi znaleść się adres katalogu stron
	push ebp
	mov ebp, esp

	mov ecx, cr3
	mov ecx, [esp + 8]
	mov cr3, ecx

	mov esp, ebp
	pop ebp

	ret

fmt:
	dd "esp: %x", 0x10, 0x0

enable_paging:
	push ebp
	mov ebp, esp
	; 
	; Włączanie obsługi stron o wielkości 4 Mib
	;mov ecx, cr4
	;or ecx, 0x00000010
	;mov cr4, ecx

	; Ustawianie bitów pg i ;pe
	mov ecx, cr0
	or ecx, 0x80000001
	mov cr0, ecx

	mov esp, ebp
	pop ebp

	ret

textbuff:
	dd "Flushing tlb", 10, 0

flush_tlb:
	mov eax, cr3
	mov cr3, eax
;	push textbuff
;	call serial_write
;	pop eax
	ret

global invl_pg

invl_pg:
	pop eax
	invlpg [eax]
	push eax
	ret

global tss_flush
tss_flush:
	mov ax, 0x28 ; bo segment taska jest 0x28 bajtów od adresu tablicy gdt

	ltr ax
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
global timer_interrupt

extern irqmaster_handler
extern irqslave_handler
extern irq1_handler
extern page_except_handler
extern timer_handler
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

global drumroll
extern save_registers
extern load_registers
extern get_privl_level
extern get_switching

; prawdopodobne problemy:
; przy przełączaniu z wątku kernela, nie jest zmieniany stos
; nie ma oddzielnych stosów w kernelu, dla każdego wątku
; chyba trzeba przenieść do rejestrów segmentu USER_DATA_SEGMENT lub KERNEL_DATA_SEGMENT, w zależności od wątku
timer_interrupt:
	pushad
	push eax
	call get_switching
	cmp dword eax, 0x0 		; sprawdzanie, czy wielowątkowość została zainicjowana
	pop eax
	jne continue_timer
	call irqmaster_handler
	popad
	iret

continue_timer:
	;pushad
	call save_registers
	popad

	cmp dword [esp + 4], USER_CODE_SEGMENT	; sprawdzanie, poziomu uprzywilejowania obecnego wątku
	je user_realm_e							

	mov ebx, [esp]	; pobieranie licznika programu
	mov eax, esp 	; pobieranie wskaźnika stosu. Stos jest taki sam w wątku jak i w obsłudze przerwania
	add eax, 12		; tylko procesor wstawił na niego 3 wartości, więc stos przed przerwaniem, był o 12 większy
	jmp common_timer
user_realm_e:
	mov ebx, [esp]			; pobieranie wartości eip i esp ze stosu
	mov eax, [esp + 12] 	; przy zmianie poziomu uprzywilejowania, procesor wstawia je tam obie, zmieniając wcześniej stos

common_timer:
	push eax
	push ebx
	call timer_handler
	pop edx
	pop edx

	;; odtąd kod taki sam jak w interrupcie
	cmp dword [eax + ESP_POS + 8], 0	; sprawdzanie poziomu uprzywilejowania, następnego wątku
	jne user_realm 				
	mov edx, [eax + ESP_POS]	; wskaźnik stosu ze struktury saved_registers obecnego wątku
	
	;mov ebp, esp
	mov esp, edx 	; zmienianie wskaźnika stosu, ponieważ obsułga przerwania jest wykonywana na poziomie 0
					; i wątek docelowy też jest na poziomie 0, to przy wywoływaniu instrukcji iret procesor nie odczyta esp ze stosu
					; więc trzeba już wcześniej zmienić stos na ten wątku wstawić tam odpowiednie wartości
	pushf		; pushowanie rejestru eflags
	pop ebx
	or ebx, 0x200	; ustawianie flagi przerwań
	push ebx	

	mov ebx, KERNEL_CODE_SEGMENT
	push ebx

	mov ebx, [eax + ESP_POS + 4]	; licznik programu ze struktury saved_registers obecnego wątku
	push ebx

	mov bx, KERNEL_DATA_SEGMENT
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx


	push eax
call irqmaster_handler		; informowanie pic, że przerwanie zostało obsłużone
	pop eax

	;call load_registers
	mov edi, [eax]			; prywracanie rejestrów ogólnego przeznaczenia
	mov esi, [eax + 4]		; na takie jakie były przed przerwaniem 
	mov ebp, [eax + 8]		; (nie koniecznie takie jakie były na początku przerwania
	;mov esp, [eax + 12]	; w linice 314)
	mov ebx, [eax + 16]
	mov edx, [eax + 20]
	mov ecx, [eax + 24]
	mov eax, [eax + 28]

	iret

user_realm:

	mov ebx, USER_DATA_SEGMENT
	push ebx

	mov ebx, [eax + ESP_POS]	; wskaźnik stosu ze struktury saved registers wątku
	push ebx

	pushf						; od tej pory działanie takie samo jak wyżej
	pop ebx
	or ebx, 0x200
	push ebx

	mov ebx, USER_CODE_SEGMENT
	push ebx

	mov ebx, [eax + ESP_POS + 4]
	push ebx

	mov bx, USER_DATA_SEGMENT
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	push eax
call irqmaster_handler
	pop eax

	mov edi, [eax]
	mov esi, [eax + 4]
	mov ebp, [eax + 8]
	;mov esp, [eax + 12]
	mov ebx, [eax + 16]
	mov edx, [eax + 20]
	mov ecx, [eax + 24]
	mov eax, [eax + 28]

	iret

irq1:
	pusha
	call irq1_handler
	popa
	iret

extern gpf_handler
global gpf
gpf:
	push eax
	mov eax, [esp + 4]
	pusha
	push eax
	call gpf_handler
	pop eax

	popa
	pop eax
	pop eax
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
	pop eax
	iret

extern double_fault_handler
global double_fault
double_fault:
	pusha
	call double_fault_handler
	popa
	iret

global halt
halt:
	hlt
	jmp halt

global get_cr3
get_cr3:
	mov eax, cr3
	ret