BUILD_DIR = ./build
ENTRY_POINT = 0xc0001500
AS = nasm
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
LIB = -I lib/ -I kernel/ -I lib/kernel -I lib/user -I device/ -I thread/ -I userprog
ASFLAGS = -f elf
ASIB = -I include/
CFLAGS =  -Wall -m32 $(LIB) -c -fno-builtin #-Wstrict-prototypes -Wmissing-prototypes
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main #-Map $(BUILD_DIR)/kernel.map
OBJS =  $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
		$(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o \
		$(BUILD_DIR)/debug.o $(BUILD_DIR)/string.o $(BUILD_DIR)/bitmap.o \
		$(BUILD_DIR)/memory.o $(BUILD_DIR)/thread.o $(BUILD_DIR)/list.o \
		$(BUILD_DIR)/switch.o $(BUILD_DIR)/console.o $(BUILD_DIR)/sync.o \
		$(BUILD_DIR)/keyboard.o $(BUILD_DIR)/ioqueue.o $(BUILD_DIR)/tss.o \
		$(BUILD_DIR)/process.o 

all: build hd run 

$(BUILD_DIR)/main.o: kernel/main.c lib/kernel/print.h lib/stdint.h kernel/init.h thread/thread.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: kernel/init.c kernel/init.h kernel/memory.h lib/kernel/print.h lib/stdint.h kernel/interrupt.h device/timer.h device/console.h device/keyboard.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c kernel/interrupt.h kernel/global.h kernel/io.h lib/kernel/print.h lib/stdint.h 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: device/timer.c device/timer.h lib/kernel/print.h lib/stdint.h  kernel/io.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/console.o: device/console.c device/console.c kernel/global.h lib/stdint.h 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/keyboard.o: device/keyboard.c device/keyboard.h kernel/global.h kernel/interrupt.h lib/stdint.h 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/ioqueue.o: device/ioqueue.c device/ioqueue.h lib/stdint.h thread/thread.h thread/sync.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/sync.o : thread/sync.c thread/sync.h lib/kernel/list.h lib/stdint.h thread/thread.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/debug.o: kernel/debug.c kernel/debug.h lib/kernel/print.h lib/stdint.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/string.o: lib/string.c lib/string.h kernel/global.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@
 
$(BUILD_DIR)/bitmap.o: lib/bitmap.c lib/bitmap.h kernel/debug.h kernel/interrupt.h lib/stdint.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/list.o:   lib/kernel/list.c lib/kernel/list.h kernel/interrupt.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o: kernel/memory.c kernel/memory.h lib/bitmap.h lib/stdint.h lib/kernel/print.h kernel/debug.h lib/string.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/thread.o: thread/thread.c thread/thread.h lib/stdint.h lib/string.c kernel/global.h kernel/memory.h kernel/debug.h kernel/interrupt.h lib/kernel/print.h lib/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/tss.o:	userprog/tss.c userprog/tss.h kernel/global.h thread/thread.h lib/kernel/print.h lib/string.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/process.o:	userprog/process.c userprog/process.h kernel/global.h  lib/stdint.h lib/string.c kernel/global.h kernel/memory.h kernel/debug.h kernel/interrupt.h lib/kernel/print.h lib/kernel/list.h userprog/tss.h
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
$(BUILD_DIR)/switch.o : thread/switch.S
	$(AS) $(ASFLAGS) $< -o $@


# 链接所有目标文件
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY: mk_dir hd clean all
mk_dir:
	if [ ! -d $(BUILD_DIR)]; then mkdir $(BUILD_DIR); fi
	bximage -hd -mode=flat -size=10 -q hd60M.img

hd:
	dd if=$(BUILD_DIR)/mbr.bin of=bochs/hd60M.img bs=512 count=1  conv=notrunc
	dd if=$(BUILD_DIR)/loader.bin of=bochs/hd60M.img bs=512 count=4 seek=2 conv=notrunc
	dd if=$(BUILD_DIR)/kernel.bin of=bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc
	
clean:
	rm -rf  $(BUILD_DIR)/*

build: $(BUILD_DIR)/mbr.bin \
		$(BUILD_DIR)/loader.bin \
		$(BUILD_DIR)/kernel.bin

run:
	cd bochs && sh run.sh

