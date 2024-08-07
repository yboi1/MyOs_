#include "bitmap.h"
#include "stdint.h" 
#include "string.h" 
#include "print.h" 
#include "interrupt.h" 
#include "debug.h"

// 位图初始化
void bitmap_init(struct bitmap* btmp) {
    memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

// 判断bit_idx位是否为1
int bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx) {
    uint32_t byte_idx = bit_idx / 8;    // 向下取整用于所有数组下标
    uint32_t bit_odd = bit_idx % 8;     // 取余用于索引数组内的位
    return (btmp->bits[byte_idx] & BITMAP_MASK << bit_odd);
}

// 位图中申请连续cnt个位, 成功返回地址, 失败返回-1
int bitmap_scan(struct bitmap* btmp, uint32_t cnt) {
    uint32_t idx_byte = 0;
    /* ---先跳到第一个空闲的位置 */
    while((0xff == btmp->bits[idx_byte]) && idx_byte < btmp->btmp_bytes_len) {
        idx_byte++;
    }
    ASSERT(idx_byte < btmp->btmp_bytes_len);
    if(idx_byte == btmp->btmp_bytes_len) {
        return -1;  // 程序终止处
    }
    int idx_bit = 0;
    // 循环找到空闲位
    while((uint8_t) BITMAP_MASK << idx_bit & btmp->bits[idx_byte] ) {   // byte
        idx_bit++;
    }
    // ---
    
    int bit_idx_start = idx_byte * 8 + idx_bit;
    if(cnt == 1) {
        return bit_idx_start;
    }

    uint32_t bit_left = (btmp->btmp_bytes_len * 8 - bit_idx_start); // 从右向左遍历的

    uint32_t next_bit = bit_idx_start + 1;
    uint32_t count = 1;

    bit_idx_start = -1;
    while(bit_left-- > 0) {
        if(!(bitmap_scan_test(btmp, next_bit))) {  // 若next_bit 为0
            count++;
        } else {
            count = 0;  // 重新寻找
        }
        if(count == cnt) {
            bit_idx_start = next_bit - cnt + 1;
            break;
        }
        next_bit++;
    }
    return bit_idx_start;
}


void bitmap_set(struct bitmap *btmp, uint32_t bit_idx, int8_t value) {
    ASSERT((value==0) || (value==1));
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;

    if(value) {
        btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
    } else {
        btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);  // 移动后取反或者取反后移动都可
    }
}


