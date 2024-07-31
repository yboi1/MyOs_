cmake_minimum_required(VERSION 3.10)

project(MyKernel)

# 设置编译器选项
set(CMAKE_C_COMPILER "x86_64-elf-gcc")
set(CMAKE_ASM_COMPILER "nasm")

set(CMAKE_C_FLAGS "-m32")
set(CMAKE_C_FLAGS "-fno-builtin")

set(CMAKE_ASM_FLAGS "-f elf")

# 设置包含路径
include_directories(lib/kernel lib kernel)

# 定义源文件
set(SOURCES
    ../kernel/main.c
    ../kernel/interrupt.c
    ../kernel/init.c
)

set(ASM_SOURCES
    ../lib/kernel/print.S
    ../kernel/kernel.S
)

# 定义输出目录
set(BUILD_DIR ${CMAKE_BINARY_DIR}/build)
file(MAKE_DIRECTORY ${BUILD_DIR})

# 编译C文件
foreach(src ${SOURCES})
    get_filename_component(src_name ${src} NAME_WE)
    add_custom_command(
        OUTPUT ${BUILD_DIR}/${src_name}.o
        COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -I lib/kernel/ -I lib/ -I kernel/ -c ${src} -o ${BUILD_DIR}/${src_name}.o
        DEPENDS ${src}
    )
    list(APPEND OBJECTS ${BUILD_DIR}/${src_name}.o)
endforeach()

# 编译ASM文件
foreach(src ${ASM_SOURCES})
    get_filename_component(src_name ${src} NAME_WE)
    add_custom_command(
        OUTPUT ${BUILD_DIR}/${src_name}.o
        COMMAND ${CMAKE_ASM_COMPILER} ${CMAKE_ASM_FLAGS} ${src} -o ${BUILD_DIR}/${src_name}.o
        DEPENDS ${src}
    )
    list(APPEND OBJECTS ${BUILD_DIR}/${src_name}.o)
endforeach()

# 链接目标
add_custom_command(
    OUTPUT ${BUILD_DIR}/kernel.bin
    COMMAND x86_64-elf-ld -m elf_i386 -Ttext 0xc0001500 -e main -o ${BUILD_DIR}/kernel.bin ${OBJECTS}
    DEPENDS ${OBJECTS}
)

# 添加dd命令
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/bochs/hd60M.img
    COMMAND dd if=${BUILD_DIR}/kernel.bin of=${CMAKE_SOURCE_DIR}/bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc
    DEPENDS ${BUILD_DIR}/kernel.bin
)

# 创建目标来运行dd命令
add_custom_target(RunDD ALL DEPENDS ${CMAKE_SOURCE_DIR}/bochs/hd60M.img)

# 添加构建目标
add_custom_target(BuildKernel ALL DEPENDS ${BUILD_DIR}/kernel.bin)
