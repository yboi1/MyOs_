x86_64-elf-gcc -ffreestanding -c main.c -o main.o &&
x86_64-elf-ld  main.o -Ttext 0xc0001500 -e main -o kernel.bin &&
dd if=kernel.bin of=../../bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc

