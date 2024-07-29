cd boot 
sh boot.sh
cd ..

nasm -f elf -o lib/kernel/print.o lib/kernel/print.S
x86_64-elf-gcc -m32 -I ./lib/kernel/ -I ./lib/ -c -o kernel/main.o kernel/main.c
x86_64-elf-ld  -m elf_i386  -Ttext 0xc0001500 -e main -o kernel/kernel.bin  kernel/main.o lib/kernel/print.o

dd if=kernel/kernel.bin of=bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc

cd bochs
bochs -f bochsrc.disk
cd ..