nasm -i ../include/ -o loader.bin loader.S
nasm -i ../include/ -o mbr.bin mbr.S

dd if=mbr.bin of=../bochs/hd60M.img bs=512 count=1  conv=notrunc
dd if=loader.bin of=../bochs/hd60M.img bs=512 count=4 seek=2 conv=notrunc

