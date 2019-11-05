#ifndef _IO_H_
#define _IO_H_



// wrap for assembly out
void outb(unsigned short port, unsigned char data);

// wrap for assembly in
unsigned char inb(unsigned short port);

#define SERIAL_COM1_BASE	0x3F8

#define SERIAL_DATA_PORT(base)	(base)
#define SERIAL_FIFO_COMMAND_PORT(base)	(base + 2)
#define SERIAL_LINE_COMMAND_PORT(base)	(base + 3)
#define SERIAL_MODEM_COMMAND_PORT(base)	(base + 4)
#define SERIAL_LINE_STATUS_PORT(base)	(base + 5)

#define SERIAL_LINE_ENABLE_DLAB	0x80

void serial_configure_baud_rate(unsigned short com, unsigned short divisor);
void serial_configure_line(unsigned short com);
void serial_init();
int is_transmit_empty();
void serial_putc(const char c);
void serial_write(const char *str);
void itoa(int a, char *buff);
void ultoa(unsigned long a, char *buff);
void utoa(unsigned int a, char *buff);

#endif