PREFIX=~/pyts/sys/cross/bin
TARGET=i686-elf

CC=$(PREFIX)/$(TARGET)-gcc
CFLAGS=-std=gnu99 -ffreestanding -Wall -Wextra -O2

AS=nasm
AFLAGS=-felf32

OBJ=objs
LINK=link.ld
LARGS=-ffreestanding -O2 -nostdlib -lgcc

out.bin: source.s cobj.o
# 	$(AS) $(AFLAGS) source.s -o out.bin
	$(AS) $(AFLAGS) source.s -o elfo.bin
	$(CC) -T $(LINK) $(LARGS) -o myos.bin elfo.bin cobj.o

# 	nasm -f bin source.s -o out.bin
	python3 transform_file.py

cobj.o: source.c
	$(CC) $(CFLAGS) -o cobj.o -c source.c



