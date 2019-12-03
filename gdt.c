#include "gdt.h"

#define GDT_NUM 5

gdt_entry gdt_entries[GDT_NUM];
gdt_ptr gdt_pointer;


void gdt_set(gdt_entry *e, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran);

void init_gdt(){
	gdt_pointer.limit = (sizeof(gdt_entry) * GDT_NUM) - 1;
	gdt_pointer.base = (unsigned int)&gdt_entries;

	gdt_set(gdt_entries, 0, 0, 0, 0);
	gdt_set(gdt_entries + 1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
	gdt_set(gdt_entries + 2, 0, 0xFFFFFFFF, 0x92, 0xCF);
	gdt_set(gdt_entries + 3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
	gdt_set(gdt_entries + 4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

	gdt_flush((unsigned int)&gdt_pointer);
}

void gdt_set(gdt_entry *e, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran){
	e->base_low = (base & 0xFFFF);
	e->base_middle = (base >> 16) & 0xFF;
	e->base_high = (base >> 24) & 0xFF;

	e->limit_low = (limit & 0xFFFF);
	e->granularity = (limit >> 16) & 0x0F;

	e->granularity |= gran & 0xF0;
	e->access = access;
}

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

    unsigned long idt_addr = (unsigned long)IDT;
    unsigned long idt_ptr[2];
    idt_ptr[0] = (sizeof(idt_entry) * 256) + ((idt_addr & 0xffff) << 16);
    idt_ptr[1] = idt_addr >> 16;

    load_idt((unsigned long)idt_ptr);

    outb(0x21, 0xFD); // Enabling keyboard (CHANGE!!!)
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

void page_except_handler(unsigned int vaddr, unsigned int err){

	serial_write("\t\tPAGE_FAULT: ");
	char buff[20];
	utoa(err, buff);
	serial_write(buff);
	serial_write(", at: ");
	serial_x(vaddr);
	serial_write("\n");
	outb(0x20, 0x20);
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