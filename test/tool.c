//
// Created by pjs on 2021/1/6.
//
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "tool.h"

#define MAX_STR_LENGTH 40
#define CHAR_MIN 'a'
#define CHAR_MAX 'z'

char *random_string() {
    int len = random_int();
    char *string = malloc(sizeof(char) * (len + 1));
    assert(string != NULL);
    for (int i = 0; i < len; ++i) {
        string[i] = rand() % (CHAR_MAX - CHAR_MIN + 1) + CHAR_MIN;
    }
    string[len] = '\0';
    return string;
}

int random_int() {
    int len = rand() % MAX_STR_LENGTH;
    return len;
}
