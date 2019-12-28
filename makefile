PREFIX=~/pyts/sys/cross/bin
TARGET=i686-elf

CC=$(PREFIX)/$(TARGET)-gcc
CFLAGS=-std=gnu99 -ffreestanding -Wall -Wextra -O2 -g

AS=nasm
AFLAGS=-felf32 -g -F dwarf

OBJ=objs
LINK=linker.ld
LARGS=-ffreestanding -O2 -nostdlib -lgcc -g

OSDIR=isodir/boot/

myos.iso: myos.bin isodir
	grub-mkrescue -o myos.iso isodir

isodir: 
	mkdir isodir 

myos.bin: boot.o kernel.o io.o gdt.o shell.o stdio.o memory2.o heap.o util.o thread.o doubly_linked_list.o
	$(CC) -T $(LINK) $(LARGS) -o myos.bin boot.o io.o stdio.o kernel.o gdt.o shell.o memory2.o heap.o util.o thread.o doubly_linked_list.o
	$(PREFIX)/$(TARGET)-objcopy --only-keep-debug myos.bin myos.sym
	$(PREFIX)/$(TARGET)-objcopy --strip-debug myos.bin
	cp myos.bin $(OSDIR)/myos.bin

util.o: util.c util.h
	$(CC) $(CFLAGS) -o util.o -c util.c	

kernel.o: kernel.c io.h
	$(CC) $(CFLAGS) -o kernel.o -c kernel.c

boot.o: boot.s
	$(AS) $(AFLAGS) -o boot.o boot.s

gdt.o: gdt.c gdt.as gdt.h
	$(AS) $(AFLAGS) -o gdt2.o gdt.as
	$(CC) $(CFLAGS) -o gdt1.o -c gdt.c
	$(PREFIX)/$(TARGET)-ld -relocatable gdt1.o gdt2.o -o gdt.o

shell.o: shell.c shell.h
	$(CC) $(CFLAGS) -o shell.o -c shell.c

io.o: io.s io.c io.h
	$(AS) $(AFLAGS) -o io1.o io.s
	$(CC) $(CFLAGS) -o io2.o -c io.c
	$(PREFIX)/$(TARGET)-ld -relocatable io1.o io2.o -o io.o

stdio.o: stdio.c stdio.h
	$(CC) $(CFLAGS) -o stdio.o -c stdio.c

memory2.o: memory2.h memory2.c
	$(CC) $(CFLAGS) -o memory2.o -c memory2.c

heap.o: heap.c heap.h
	$(CC) $(CFLAGS) -o heap.o -c heap.c	

thread.o: thread.c thread.h
	$(CC) $(CFLAGS) -o thread.o -c thread.c


doubly_linked_list.o: doubly_linked_list.h doubly_linked_list.c
	$(CC) $(CFLAGS) -o doubly_linked_list.o -c doubly_linked_list.c	