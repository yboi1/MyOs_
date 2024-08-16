#include "string.h" 
#include "global.h"
#include "debug.h"



// 将 dst_起始的size个字节设为value
void memset(void* dst_, uint8_t value, uint32_t size){
    ASSERT(dst_ != NULL);
    uint8_t *dst = (uint8_t *)dst_;
    while(size-- >0) {
        *dst++ = value;
    } 
}

// 将sec_起始的size个字节复制到dst_
void memcpy(void* dst_, const void* src_, uint32_t size) {
    ASSERT(dst_ != NULL && src_ != NULL);
    uint8_t* dst = dst_;
    const uint8_t* src = src_;
    while( (size--) > 0) {
        *(dst++) = *(src++);
    }
    return;
}

// 内存比较函数 大于:1 小于:2 等于:0
int memcmp(const void* a_, const void*b_ , uint32_t size) {
    const char* a = a_;
    const char* b = b_;
    ASSERT(a!=NULL && b!=NULL);
    while(size-- > 0) {
        if(*a!=*b){
            return *a>*b ? 1 : -1;
        }
        a++;
        b++;
    }
    return 0;
}

// 将字符串src复制到dst  返回目标地址
char* strcpy(char* dst_, const char* src_) {
    ASSERT(dst_!=NULL && src_!=NULL);
    char* r = dst_;
    while((*dst_++ = *src_++));
    return r;
}

// 返回字符串长度 通过偏移地址计算
uint32_t strlen(const char* str){
    ASSERT(str!=NULL);
    const char* p = str;
    while(*p++);
    return (p-str-1);
}

// 字符串比较函数 大于1 小于-1 等于0
int8_t strcmp(const char* a, const char* b) {
    ASSERT(a!=NULL && b!=NULL);
    while(*a!=0 && *a==*b){
        a++;
        b++;
    }
    return *a<*b ? -1 : *a > *b;
}

char* strchr(const char* str, const uint8_t ch) {
    ASSERT(str != NULL);
    while(*str!=0){
        if(*str==ch){
            return (char*) str; // 地址
        }
        str++;
    }
    return NULL;
}

// 从后向前查找字符串str 中首次出现字符ch的地址
char* strrchr(const char* str, const uint8_t ch){
    ASSERT(str!=NULL);
    const char* last_char = NULL;
    while(*str!=0){
        if(*str==ch){
            last_char = str;
        }
        str++;
    }
    return (char*) last_char;
}

// 字符串拼接 将stc拼接到dst后, 返回dst地址
char* strcat(char* dst_, const char* src_) {
    ASSERT(dst_!=NULL && src_ != NULL );
    char* str = dst_;
    while(*str++);
    --str;
    while((*str++ = *src_++));
    return dst_;
}

// 返回字符串str中查找字符ch出现的次数
uint32_t strchrs(const char* str, uint8_t ch) {
    ASSERT(str != NULL);
    uint32_t ch_cnt = 0;
    const char* p = str;
    while(*p != 0) {
        if(*p == ch){
            ch_cnt++;
        }
        p++;
    }
    return ch_cnt;
}

