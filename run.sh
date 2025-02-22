cd boot 
sh boot.sh
cd ..

x86_64-elf-gcc -m32 -I ./lib/kernel/ -I ./lib/ -I kernel/ -c -fno-builtin \
               -o build/main.o kernel/main.c

nasm -f elf -o build/print.o lib/kernel/print.S

nasm -f elf -o build/kernel.o kernel/kernel.S

x86_64-elf-gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin \
                -o build/timer.o device/timer.c

x86_64-elf-gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin \
                -o build/interrupt.o kernel/interrupt.c

x86_64-elf-gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin \
                -o build/init.o kernel/init.c              

x86_64-elf-ld  -m elf_i386  -Ttext 0xc0001500 -e main -o build/kernel.bin  build/main.o  build/init.o \
            build/interrupt.o build/print.o build/kernel.o build/timer.o

dd if=build/kernel.bin of=bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc

cd bochs
bochs -f bochsrc.disk
cd ..