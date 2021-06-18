//
// Created by pjs on 2021/6/9.
//

#ifndef QUARKOS_TERMINAL_H
#define QUARKOS_TERMINAL_H

#include <types.h>

void terminal_init();


#define assertk(condition) do{\
    if (!(condition)) {     \
        printfk("\nassert error: %s: %s: %u\n",__FILE__,__FUNCTION__,__LINE__); \
        panic();                        \
    }\
}while(0)


#define error(string) {\
    printfk("\n%s: %s: %s: %u\n",string,__FILE__,__FUNCTION__,__LINE__); \
    panic();                        \
}


void printfk(char *__restrict str, ...);

void put_char(char c);

void put_strings(const char *data, size_t size);

void put_string(const char *data);

int32_t k_gets(char *buf, int32_t len);

void k_puts(char *buf, int32_t len);

void terminal_clear();

#ifdef TEST
#define test_start   printfk("test start: %s\n",__FUNCTION__);
#define test_pass    printfk("test pass : %s: %s: %u\n",__FILE__,__FUNCTION__,__LINE__);
#endif // TEST

#endif //QUARKOS_TERMINAL_H
