PREFIX=~/pyts/sys/cross/bin
TARGET=i686-elf

CC=$(PREFIX)/$(TARGET)-gcc
CFLAGS=-std=gnu99 -ffreestanding -Wall -Wextra -O2

AS=nasm
AFLAGS=-felf32

OBJ=objs
LINK=link.ld
LARGS=-ffreestanding -O2 -nostdlib -lgcc

out.bin: source.s cobj.o stdio.o thread.o stdlib.o semaphore.o
# 	$(AS) $(AFLAGS) source.s -o out.bin
	$(AS) $(AFLAGS) source.s -o elfo.bin
	$(CC) -T $(LINK) $(LARGS) -o myos.bin elfo.bin cobj.o stdio.o thread.o stdlib.o semaphore.o

# 	nasm -f bin source.s -o out.bin
	python3 transform_file.py

# cobj.o: sourcetempl.c
# 	$(CC) $(CFLAGS) -o cobj.o -c sourcetempl.c
cobj.o: source.c
	$(CC) $(CFLAGS) -o cobj.o -c source.c

stdio.o: stdio.c stdio.h
	$(CC) $(CFLAGS) -o stdio.o -c stdio.c

thread.o: thread.c thread.h
	$(CC) $(CFLAGS) -o thread.o -c thread.c

stdlib.o: stdlib.c stdlib.h
	$(CC) $(CFLAGS) -o stdlib.o -c stdlib.c
	
semaphore.o: semaphore.c semaphore.h
	$(CC) $(CFLAGS) -o semaphore.o -c semaphore.c
