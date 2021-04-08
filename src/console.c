//
// Created by pjs on 2021/4/8.
//

#include "types.h"
#include "drivers/keyboard.h"
#include "klib/qlib.h"
#include "sched/timer.h"

// sched_init 调用后才能使用
char kb_getchar() {
    uint8_t c;
    q_clear(&kb_buf);
    do {
        c = q_pop(&kb_buf);
        assertk(ms_sleep(10));
    } while (c == KB_NULL);
    vga_put_char(c);
    return c;
}
