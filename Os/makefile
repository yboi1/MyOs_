BUILD_DIR = ./build
ENTRY_POINT = 0xc0001500
AS = nasm
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
LIB = -I lib/ -I kernel/ -I lib/kernel -I lib/user -I device/
ASFLAGS = -f elf
ASIB = -I include/
CFLAGS =  -Wall -m32 $(LIB) -c -fno-builtin #-Wstrict-prototypes -Wmissing-prototypes
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o

all: build hd run

$(BUILD_DIR)/main.o: kernel/main.c lib/kernel/print.h lib/stdint.h kernel/init.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: kernel/init.c kernel/init.h lib/kernel/print.h lib/stdint.h kernel/interrupt.h device/timer.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/global.h kernel/io.h lib/kernel/print.h lib/stdint.h kernel/interrupt.h kernel/interrupt.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: lib/kernel/print.h lib/stdint.h device/timer.h device/timer.c kernel/io.h
	$(CC) $(CFLAGS) $< -o $@

# 编译loader mbr
$(BUILD_DIR)/mbr.bin : boot/mbr.S
	$(AS) $(ASIB) $< -o $@
$(BUILD_DIR)/loader.bin : boot/loader.S
	$(AS) $(ASIB) $< -o $@


# 汇编代码编译
$(BUILD_DIR)/kernel.o : kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o  $@
$(BUILD_DIR)/print.o : lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@

# 链接所有目标文件
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) &^ -o $@

.PHONY: mk_dir hd clean all
mk_dir:
	if [ ! -d $(BUILD_DIR)]; then mkdir $(BUILD_DIR); fi
	bximage -hd -mode=flat -size=10 -q hd60M.img

hd:
	dd if=$(BUILD_DIR)/mbr.bin of=bochs/hd60M.img bs=512 count=1  conv=notrunc
	dd if=$(BUILD_DIR)/loader.bin of=bochs/hd60M.img bs=512 count=4 seek=2 conv=notrunc
	dd if=$(BUILD_DIR)/kernel.bin of=bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc
	
clean:
	rm -rf hd60M.img $(BUILD_DIR)/*

build: $(BUILD_DIR)/mbr.bin $(BUILD_DIR)/loader.bin $(BUILD_DIR)/kernel.bin

run:
	cd bochs && sh run.sh

