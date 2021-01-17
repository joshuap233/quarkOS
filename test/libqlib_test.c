#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tool.h"
#include "qstring.h"
#include "qlib.h"


void g_itoa(int num);

void t_itoa(uint32_t num);

int main() {
    test_init();
    g_itoa(10);
    return 0;
}

void g_itoa(int count) {
    for (int i = 0; i < count; i++) {
        t_itoa(random_int());
    }
}

void t_itoa(uint32_t num) {
    char str[33], str2[33];
    q_itoa(num, str);
    int length = snprintf(NULL, 0, "%d", num);
    snprintf(str2, length + 1, "%d", num);
    assert(strcmp(str, str2) == 0);
    printf("str: %s\n", str);
}

void g_hex(){

}