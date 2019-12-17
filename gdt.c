#include "gdt.h"

#define GDT_NUM 6

gdt_entry gdt_entries[GDT_NUM];
gdt_ptr gdt_pointer;

tss_entry_t tss_entry;
unsigned int temp_tss_stack[1024];

#define GDT_TYPE_NORMAL 1
#define GDT_TYPE_TASK 0
#define GDT_RING0 0
#define GDT_RING3 3 
#define GDT_DATA_SEGMENT 0
#define GDT_CODE_SEGMENT 1

// void gdt_set(gdt_entry *e, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran);
void gdt_set(
	gdt_entry *e,
	unsigned int base, 
	unsigned int limit,
	unsigned int type,
	unsigned int privl,
	unsigned int exec);

gdt_entry *init_gdt(){
	gdt_pointer.limit = (sizeof(gdt_entry) * GDT_NUM) - 1;
	gdt_pointer.base = (unsigned int)&gdt_entries;

	// gdt_set(gdt_entries, 0, 0, 0, 0);
	// gdt_set(gdt_entries + 1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
	// gdt_set(gdt_entries + 2, 0, 0xFFFFFFFF, 0x92, 0xCF);
	// gdt_set(gdt_entries + 3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
	// gdt_set(gdt_entries + 4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
	// gdt_set(gdt_entries + 4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
	// pierwszy element gdt musi być NULLem
	unsigned char *e0 = &gdt_entries[0];
	for(int i = 0; i < sizeof(gdt_entry); i++){
		e0 = 0;
	}
	gdt_set(&gdt_entries[1], 0, 0xFFFFF, GDT_TYPE_NORMAL, GDT_RING0, GDT_CODE_SEGMENT);
	gdt_set(&gdt_entries[2], 0, 0xFFFFF, GDT_TYPE_NORMAL, GDT_RING0, GDT_DATA_SEGMENT);
	gdt_set(&gdt_entries[3], 0, 0xFFFFF, GDT_TYPE_NORMAL, GDT_RING3, GDT_CODE_SEGMENT);
	gdt_set(&gdt_entries[4], 0, 0xFFFFF, GDT_TYPE_NORMAL, GDT_RING3, GDT_DATA_SEGMENT);

	// ustawianie tss
	unsigned int tbase = (unsigned int)&tss_entry;
	unsigned int tlimit = sizeof(tss_entry);

	gdt_entry *tss = &gdt_entries[5];
	tss->limit_low = tlimit & 0xFFFF;
	tss->base_low = tbase & 0xFFFFFF;
	tss->accessed = 1;
	tss->read_write = 0;
	tss->conforming_expand_down = 0;
	tss->executable = 1;
	tss->desc_type = GDT_TYPE_TASK;
	tss->privl = 3;
	tss->present = 1;
	tss->always_0 = 0;
	tss->big = 0;
	tss->granularity = 0;
	tss->base_high = (tbase & 0xFF000000) >> 24;

	memset(&tss_entry, 0, sizeof(tss_entry_t));
	tss_entry.ss0 = 0x10;
	tss_entry.esp0 = temp_tss_stack + 1024;

	// gdt_set(&gdt_entries[5], 0, 0xFFFFF, GDT_TYPE_TASK, GDT_RING0, GDT_CODE_SEGMENT);
	// gdt_set(gdt_entries, 0, 0, 0, 0);
	// gdt_set(gdt_entries + 1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
	// gdt_set(gdt_entries + 2, 0, 0xFFFFFFFF, 0x92, 0xCF);
	// gdt_set(gdt_entries + 3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
	// gdt_set(gdt_entries + 4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
	// gdt_set(gdt_entries + 4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

	gdt_flush((unsigned int)&gdt_pointer);
	tss_flush();

	printf("temp tss stack: %x\n", (unsigned int)temp_tss_stack);

	return gdt_entries;
}

void set_kernel_stack(unsigned int stack){
	tss_entry.esp0 = stack;
}

void gdt_set(gdt_entry *e, unsigned int base, unsigned int limit,
	unsigned int type,
	unsigned int privl,
	unsigned int exec){
	e->base_low = (base & 0xFFFFFF);
	e->base_high = (base >> 24) & 0xFF;

	e->limit_low = (limit & 0xFFFF);
	e->limit_high = (limit >> 16) & 0xF;

	e->accessed = 0;
	e->read_write = 1;
	e->conforming_expand_down = 0;
	e->executable = exec & 1;
	e->desc_type = type & 1;
	e->privl = privl & 3;
	e->present = 1;
	// e->available = 1;
	e->always_0 = 0;
	e->big = 1;
	e->granularity = 1; // adresowanie ze stronami 4kib
}
// void gdt_set(gdt_entry *e, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran){
// 	e->base_low = (base & 0xFFFF);
// 	e->base_middle = (base >> 16) & 0xFF;
// 	e->base_high = (base >> 24) & 0xFF;

// 	e->limit_low = (limit & 0xFFFF);
// 	e->granularity = (limit >> 16) & 0x0F;

// 	e->granularity |= gran & 0xF0;
// 	e->access = access;
// }

idt_entry IDT[256];

#define SET_IDT_ENTRY(ID, ADDR) \
	unsigned long taddr = (unsigned long)ADDR; \
	IDT[ID].offset_low = taddr & 0xffff; \
	IDT[ID].selector = 0x08; \
	IDT[ID].zero = 0; \
	IDT[ID].type_attr = 0x8e; \
	IDT[ID].offset_high = (taddr & 0xffff0000) >> 16;


#define PICM 0x20 			// master PIC
#define PICS 0xA0			// slave PIC
#define PICM_COMMAND PICM
#define PICM_DATA (PICM + 1)
#define PICS_COMMAND PICS
#define PICS_DATA (PICS + 1)

#define PIC_EOI 0x20 		// end of interrupt
#define PIC_INIT 0x11

void init_idt(){
	serial_write("initing_idt\n");

	// Remapping interrupt numbers
	outb(PICM, PIC_INIT);
    outb(PICS, PIC_INIT);
    outb(PICM_DATA, 0x20); // offset of master PIC
    outb(PICS_DATA, 0x28); // offset of slave PIC
    outb(PICM_DATA, 0x04);
    outb(PICS_DATA, 0x02);
    outb(PICM_DATA, 0x01);
    outb(PICS_DATA, 0x01);
    outb(PICM_DATA, 0x0);
    outb(PICS_DATA, 0x0);
    

    SET_IDT_ENTRY(14, page_except);
    {
    	SET_IDT_ENTRY(8, double_fault);
	}
	{
		SET_IDT_ENTRY(13, gpf);
	}
	{
		unsigned int sc_id = 80;
		unsigned int scaddr = (unsigned int)syscall;
		IDT[sc_id].offset_low = scaddr & 0xFFFF;
		IDT[sc_id].offset_high = (scaddr & 0xFFFF0000) >> 16;
		IDT[sc_id].selector = 0x8;
		IDT[sc_id].zero = 0;
		IDT[sc_id].type_attr = 0xE | (1 << 7) | (3 << 5);
	}

    for(int i = 0; i < 8; i++){
    	if(i != 1){
    		SET_IDT_ENTRY(32 + i, irqdefm)
    	}else{
			serial_write("setting irq1\n");
    		SET_IDT_ENTRY(32 + i, irq1)
    	}
    }

    for(int i = 8; i < 16; i++){
    	SET_IDT_ENTRY(32 + i, irqdefs)
    }

    // Ustawianie przerwania timera
    {
    	SET_IDT_ENTRY(32, timer_interrupt);
    }

    unsigned long idt_addr = (unsigned long)IDT;
    unsigned long idt_ptr[2];
    idt_ptr[0] = (sizeof(idt_entry) * 256) + ((idt_addr & 0xffff) << 16);
    idt_ptr[1] = idt_addr >> 16;

    load_idt((unsigned long)idt_ptr);

    // Tutaj jest maska sprzętowych przerwań
    outb(0x21, 0xFC); // Enabling keyboard (CHANGE!!!)
    // outb(0x21, 0xFD); // Enabling keyboard (CHANGE!!!)
    init_key_map();

	serial_write("idt inited\n");
}

char key_map[256];
char char_keys_map[20];

void irqmaster_handler(){
	outb(PICM_COMMAND, PIC_EOI);
}

void irqslave_handler(){
	outb(PICS_COMMAND, PIC_EOI);
	outb(PICM_COMMAND, PIC_EOI);
}

void double_fault_handler(unsigned int err){
	serial_write(" - DOUBLE FAULT - ");
	serial_x(err);
	serial_write("\n");
	halt();
}

#include "thread.h"

void page_except_handler(unsigned int vaddr, unsigned int err){
	printf("Page fault: \n");

	thread_t *th = get_current_thread();
	printf("thread: %d\n", th->pid);

	serial_write("\t\tPAGE_FAULT: ");
	char buff[20];
	utoa(err, buff);
	serial_write(buff);
	serial_write(", at: ");
	serial_x(vaddr);
	serial_write("\n");
	halt();
	//outb(0x20, 0x20);

}

void gpf_handler(unsigned int err){
	printf(" GPF\n");
	serial_write("	- GPF - \nat: ");
	serial_x(err);
	serial_write("\n");
	halt();
}


#define TIME_INTERVAL 100
#define PIT_CHANNEL0 0x40
#define PIT_CONFIGURE 0x43
#define PIT_MODE3 0x36
void init_pit(){
	int divisor = 1193182 / TIME_INTERVAL;

	outb(PIT_CONFIGURE, PIT_MODE3);

	outb(PIT_CHANNEL0, (char)(divisor & 0xFF));
	outb(PIT_CHANNEL0, (char)((divisor >> 8) & 0xFF));
	serial_write("initing pit\n");
}

// proc_state *timer_handler(unsigned int esp, unsigned int eip){
// 	queue_dec()	

// 	thread_t *cur_thread = get_current_thread();
// 	proc_state *cur_state = NULL;
// 	extern unsigned int do_switching;
// 	if(do_switching && cur_thread != NULL){
// 	cur_state = &cur_thread->state;

// 	if(do_switching && get_current_thread() != NULL){
// 		proc_state *ret = switch_task(esp, eip);
// 		if(ret != NULL)
// 			cur_state = ret;
// 	}

// 	ticks_sience_start++;
// 	if(ticks_sience_start % TIME_INTERVAL == 0);
// 		// sekunda mineła
// 	}
// 	outb(0x20, 0x20);

// 	return cur_state;
// }

#define SYSCALL_OUTS 0
#define SYSCALL_GETS 1
#define SYSCALL_SWITCH_TASK 2
#define SYSCALL_SLEEP 3

// extern void switch_task();
// prog_esp to wskaźnik stosu, programu/wątku który wykonał syscall'a 
// używany po to aby można było odczytać więcej parametrów jeśli potrzeba
void *syscall_handler(unsigned int param1, unsigned int param2, unsigned int prog_esp, unsigned int esp, unsigned int eip){
	// printf("[SYSCALL] param1: %x, param2: %x, eip: %x\n", param1, param2, prog_eip);
	serial_write("[SYSCALL] it's wednesday\nparam1: ");
	serial_x(param1);
	serial_write("\nparam2: ");
	serial_x(param2);
	serial_write("\nprog_esp: ");
	serial_x(prog_esp);
	serial_write("\nesp: ");
	serial_x(esp);
	serial_write("\neip: ");
	serial_x(eip);
	serial_write("\ncr3: ");
	extern get_cr3();
	serial_x(get_cr3());
	serial_write("\n");
	serial_write("\n");
	struct registers_struct *cur_registers = &(get_current_thread()->saved_registers);
	if(cur_registers != NULL){
		cur_registers->eip = eip;
		cur_registers->esp = esp;
		// printf("[SYSCALL] eip: %x, esp: %x\n", cur_registers->eip, cur_registers->esp);
	}else{
		printf("[SYSCALL] CUR REGISTERS == NULL\n");
	}
	void *ret = cur_registers;
	// printf("Returned state: \n");
	// display_state(ret);
	switch(param2){
		case SYSCALL_OUTS:
		{
			// printf("SYSCAL PRINTFING\n");
			// printf("[SYSCALL] %s\n", (char *)param1);

			// printf("[SYSCALL] Outs\n");
			printf("%s\n", (char *)param1);

			// printf((char *)param1);
		}
		break;
		case SYSCALL_SWITCH_TASK:
		{
			printf("[SYSCALL] Switching tasks\n");
			// printf("ESP: %x\n", esp);
			// printf("EIP: %x\n", eip);
			ret = switch_task(esp, eip);
		}
		break;
		case SYSCALL_SLEEP:
		{
			ret = sleep_task(esp, eip, param1);
			// ret = switch_task(esp, eip);
		}
		break;
	}
	serial_write("After eip: ");
	serial_x(((struct registers_struct *)ret)->eip);
	serial_write("\nAfter esp: ");
	serial_x(((struct registers_struct *)ret)->esp);
	serial_write("\ncr3: ");
	serial_x(get_cr3());
	serial_write("\n");
	serial_write("--------------\n");

	// struct registers_struct *after = (struct registers_struct *)ret;
	// printf("cur regs: %x, x: %x\n", cur_registers->privl, *((unsigned int *)((char *)ret + 40)));
	// printf("ret esp: %x, ret eip: %x, ret pri: %d\n", *((unsigned int *)((char *)ret + 32)), *((unsigned int *)((char *)ret + 36)), *((unsigned int *)((char *)ret + 40)));

	return ret;
}

#define KEY_CTRL 0
#define KEY_SHIFT 1
#define KEY_ALT 2

unsigned char special_keys[3];
#define PUNCT_SIZE 10
char punct_chars[PUNCT_SIZE];

void irq1_handler(){ // handler klawiatury
	// serial_write("Key pressed: ");
	// char num[10];
	unsigned char scancode = inb(KEYBOARD_DATA_PORT);
	switch(scancode){
		case 0x2A: // left shift pressed
			special_keys[KEY_SHIFT] = 1;
			serial_write("[KEYBOARD] shift pressed\n");
		break;
		case 0x36: // left shift pressed
			special_keys[KEY_SHIFT] = 1;
			serial_write("[KEYBOARD] shift pressed\n");
		break;
		case 0xAA: // left shift released
			special_keys[KEY_SHIFT] = 0;
			serial_write("[KEYBOARD] shift released\n");
		break;
		case 0xB6:
			special_keys[KEY_SHIFT] = 0;
			serial_write("[KEYBOARD] shift released\n");
		break;
	}
	if(scancode <= 0x39 && key_map[scancode] != '\0'){
		switch(key_map[scancode]){
			case '\e':
				terminal_enter();
			break;
			case '\t':
				terminal_redraw();
			break;
			case '\b':
				terminal_backspace();
			break;

			default:
			{
				char char_to_put = key_map[scancode];
				if(special_keys[KEY_SHIFT]){
					if(isalpha(char_to_put)){
						char_to_put = toupper(char_to_put);
						// serial_write("[KEYBOARD] IS ALPHA\n");
					}else if(isnum(char_to_put)){
						// serial_write("[KEYBOARD] IS NUM\n");
						char_to_put = char_keys_map[char_to_put - '0'];
					}else{
						for(int k = 0; k < PUNCT_SIZE; k++)
							if(char_to_put == punct_chars[k]){
								char_to_put = char_keys_map[k + 10];
								break;
							}
					}
					// char_to_put = '_';
				}
				terminal_putc(char_to_put);
				terminal_input_from_keyboard();
			}
		}
	}
	
	// itoa((int)scancode, num);
	// serial_putc(scancode);
	// serial_write(num);
	// serial_write("\n");
	outb(0x20, 0x20); // interrupt handled
}

void init_key_map(){
	char_keys_map[1] = '!';
	char_keys_map[2] = '@';
	char_keys_map[3] = '#';
	char_keys_map[4] = '$';
	char_keys_map[5] = '%';
	char_keys_map[6] = '^';
	char_keys_map[7] = '&';
	char_keys_map[8] = '*';
	char_keys_map[9] = '(';
	char_keys_map[0] = ')';
	char_keys_map[10] = '_';
	char_keys_map[11] = '+';
	char_keys_map[12] = '{';
	char_keys_map[13] = '}';
	char_keys_map[14] = '|';
	char_keys_map[15] = ':';
	char_keys_map[16] = '"';
	char_keys_map[17] = '<';
	char_keys_map[18] = '>';
	char_keys_map[19] = '?';

	punct_chars[0] = '-';
	punct_chars[1] = '=';
	punct_chars[2] = '[';
	punct_chars[3] = ']';
	punct_chars[4] = '\\';
	punct_chars[5] = ';';
	punct_chars[6] = '\'';
	punct_chars[7] = ',';
	punct_chars[8] = '.';
	punct_chars[9] = '/';


	special_keys[0] = 0;
	special_keys[1] = 0;
	special_keys[2] = 0;

	key_map[0x2] = '1';
	key_map[0x3] = '2';
	key_map[0x4] = '3';
	key_map[0x5] = '4';
	key_map[0x6] = '5';
	key_map[0x7] = '6';
	key_map[0x8] = '7';
	key_map[0x9] = '8';
	key_map[0xA] = '9';
	key_map[0xB] = '0';

	key_map[0xC] = '-';
	key_map[0xD] = '=';
	key_map[0xE] = '\b';
	key_map[0xF] = '\t';
	key_map[0x10] = 'q';
	key_map[0x11] = 'w';
	key_map[0x12] = 'e';
	key_map[0x13] = 'r';
	key_map[0x14] = 't';
	key_map[0x15] = 'y';
	key_map[0x16] = 'u';
	key_map[0X17] = 'i';
	key_map[0x18] = 'o';
	key_map[0x19] = 'p';
	key_map[0x1A] = '[';
	key_map[0x1B] = ']';
	key_map[0x1C] = '\e'; //enter
	key_map[0x1D] = '1'; // left control
	key_map[0x1E] = 'a';
	key_map[0x1F] = 's';
	key_map[0x20] = 'd';
	key_map[0x21] = 'f';
	key_map[0x22] = 'g';
	key_map[0x23] = 'h';
	key_map[0x24] = 'j';
	key_map[0x25] = 'k';
	key_map[0x26] = 'l';
	key_map[0x27] = ';';
	key_map[0x28] = '\'';
	key_map[0x29] = '`'; 
	key_map[0x2A] = '\0'; // left shift
	key_map[0x2B] = '\\';
	key_map[0x2C] = 'z';
	key_map[0x2D] = 'x';
	key_map[0x2E] = 'c';
	key_map[0x2F] = 'v';
	key_map[0x30] = 'b';
	key_map[0x31] = 'n';
	key_map[0x32] = 'm';
	key_map[0x33] = ',';
	key_map[0x34] = '.';
	key_map[0x35] = '/'; 
	key_map[0x36] = '\0'; // right shift
	key_map[0x37] = '*';  // keypad *
	key_map[0x38] = '\0';  // left alt
	key_map[0x39] = ' '; 
}