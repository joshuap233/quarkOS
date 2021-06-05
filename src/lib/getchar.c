//
// Created by pjs on 2021/6/5.
//

#include <drivers/keyboard.h>
#include <lib/qlib.h>
#include <sched/timer.h>
#include <lib/getchar.h>

char kb_getchar() {
    char c;
    q_clear(&kb_buf);
    do {
        c = q_pop(&kb_buf);
        assertk(ms_sleep(10));
    } while (c == KB_NULL);
    vga_put_char(c);
    return c;
}
