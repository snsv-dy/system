PREFIX=~/pyts/sys/cross/bin
TARGET=i686-elf

CC=$(PREFIX)/$(TARGET)-gcc
CFLAGS=-std=gnu99 -ffreestanding -Wall -Wextra -O2

AS=nasm
AFLAGS=-felf32

OBJ=objs
LINK=linker.ld
LARGS=-ffreestanding -O2 -nostdlib -lgcc

OSDIR=isodir/boot/

myos.iso: myos.bin
	grub-mkrescue -o myos.iso isodir

myos.bin: boot.o kernel.o io.o gdt.o shell.o
	$(CC) -T $(LINK) $(LARGS) -o myos.bin boot.o io.o kernel.o gdt.o shell.o
	cp myos.bin $(OSDIR)/myos.bin

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
