cmake_minimum_required(VERSION 3.10)

# Project name
project(KernelBuild)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)  
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the build directory
set(BUILD_DIR ${CMAKE_BINARY_DIR}/build)

# Set the entry point
set(ENTRY_POINT 0xc0001500)

# Compiler and linker settings
set(AS nasm)
set(CC x86_64-elf-gcc)
set(LD x86_64-elf-ld)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/kernel ${CMAKE_SOURCE_DIR}/lib/kernel ${CMAKE_SOURCE_DIR}/lib/user ${CMAKE_SOURCE_DIR}/device ${CMAKE_SOURCE_DIR}/include)
# include_directories(${CMAKE_SOURCE_DIR}/lib)
# include_directories(${CMAKE_SOURCE_DIR}/kernel)
# include_directories(${CMAKE_SOURCE_DIR}/lib/kernel)
# include_directories(${CMAKE_SOURCE_DIR}/lib/user)
# include_directories(${CMAKE_SOURCE_DIR}/device)
# include_directories(${CMAKE_SOURCE_DIR}/include)


# Compiler flags
set(CFLAGS -Wall -m32 -c -fno-builtin)
set(ASFLAGS -f elf)
set(LDFLAGS -m elf_i386 -Ttext ${ENTRY_POINT} -e main)

# Source files
set(SOURCES
    kernel/main.c
    kernel/init.c
    kernel/interrupt.c
    device/timer.c
    kernel/debug.c
    kernel/kernel.S
    lib/kernel/print.S
    boot/mbr.S
    boot/loader.S
)

# Ensure the build directory exists
file(MAKE_DIRECTORY ${BUILD_DIR})

# Add custom commands for each target
add_custom_command(OUTPUT ${BUILD_DIR}/main.o
    COMMAND ${CC} ${CFLAGS} ${CMAKE_SOURCE_DIR}/kernel/main.c -o ${BUILD_DIR}/main.o
    DEPENDS kernel/main.c lib/kernel/print.h lib/stdint.h kernel/init.h
)

add_custom_command(OUTPUT ${BUILD_DIR}/init.o
    COMMAND ${CC} ${CFLAGS} ${CMAKE_SOURCE_DIR}/kernel/init.c -o ${BUILD_DIR}/init.o
    DEPENDS kernel/init.c kernel/init.h lib/kernel/print.h lib/stdint.h kernel/interrupt.h device/timer.h
)

add_custom_command(OUTPUT ${BUILD_DIR}/interrupt.o
    COMMAND ${CC} ${CFLAGS} ${CMAKE_SOURCE_DIR}/kernel/interrupt.c -o ${BUILD_DIR}/interrupt.o
    DEPENDS kernel/interrupt.c kernel/interrupt.h kernel/global.h kernel/io.h lib/kernel/print.h lib/stdint.h
)

add_custom_command(OUTPUT ${BUILD_DIR}/timer.o
    COMMAND ${CC} ${CFLAGS} ${CMAKE_SOURCE_DIR}/device/timer.c -o ${BUILD_DIR}/timer.o
    DEPENDS device/timer.c device/timer.h lib/kernel/print.h lib/stdint.h kernel/io.h
)

add_custom_command(OUTPUT ${BUILD_DIR}/debug.o
    COMMAND ${CC} ${CFLAGS} ${CMAKE_SOURCE_DIR}/kernel/debug.c -o ${BUILD_DIR}/debug.o
    DEPENDS kernel/debug.c kernel/debug.h lib/kernel/print.h lib/stdint.h kernel/interrupt.h
)

add_custom_command(OUTPUT ${BUILD_DIR}/mbr.bin
    COMMAND ${AS} -I ${CMAKE_SOURCE_DIR}/include/ ${CMAKE_SOURCE_DIR}/boot/mbr.S -o ${BUILD_DIR}/mbr.bin
    DEPENDS boot/mbr.S
)

add_custom_command(OUTPUT ${BUILD_DIR}/loader.bin
    COMMAND ${AS} -I ${CMAKE_SOURCE_DIR}/include/ ${CMAKE_SOURCE_DIR}/boot/loader.S -o ${BUILD_DIR}/loader.bin
    DEPENDS boot/loader.S
)

add_custom_command(OUTPUT ${BUILD_DIR}/kernel.o
    COMMAND ${AS} ${ASFLAGS} ${CMAKE_SOURCE_DIR}/kernel/kernel.S -o ${BUILD_DIR}/kernel.o
    DEPENDS kernel/kernel.S
)

add_custom_command(OUTPUT ${BUILD_DIR}/print.o
    COMMAND ${AS} ${ASFLAGS} ${CMAKE_SOURCE_DIR}/lib/kernel/print.S -o ${BUILD_DIR}/print.o
    DEPENDS lib/kernel/print.S
)

add_custom_command(OUTPUT ${BUILD_DIR}/kernel.bin
    COMMAND ${LD} ${LDFLAGS} ${BUILD_DIR}/main.o ${BUILD_DIR}/init.o ${BUILD_DIR}/interrupt.o ${BUILD_DIR}/timer.o ${BUILD_DIR}/kernel.o ${BUILD_DIR}/print.o ${BUILD_DIR}/debug.o -o ${BUILD_DIR}/kernel.bin
    DEPENDS ${BUILD_DIR}/main.o ${BUILD_DIR}/init.o ${BUILD_DIR}/interrupt.o ${BUILD_DIR}/timer.o ${BUILD_DIR}/kernel.o ${BUILD_DIR}/print.o ${BUILD_DIR}/debug.o
)

# Add custom targets for build, hd, and run
add_custom_target(build ALL
    DEPENDS ${BUILD_DIR}/kernel.bin
    COMMAND ${CMAKE_COMMAND} -E echo "Build completed."
)

add_custom_target(hd
    COMMAND dd if=${BUILD_DIR}/mbr.bin of=bochs/hd60M.img bs=512 count=1 conv=notrunc
    COMMAND dd if=${BUILD_DIR}/loader.bin of=bochs/hd60M.img bs=512 count=4 seek=2 conv=notrunc
    COMMAND dd if=${BUILD_DIR}/kernel.bin of=bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc
    DEPENDS build
)

add_custom_target(run
    COMMAND cd bochs && sh run.sh
    DEPENDS hd
)

# Add custom target for clean
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${BUILD_DIR}
)
