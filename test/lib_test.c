#include "qstring.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define MAX_STR_LENGTH 40
#define CHAR_MIN 'a'
#define CHAR_MAX 'z'

//  g 开头函数用于生成数据,调用测试代码
//  t 开头函数为实际测试代码
char *random_string();

int random_int();

void g_strlen(int num);

void t_strlen(char *str);

void g_strcat_strncat(int num);

void t_strcat(char *dest1, char *dest2, char *tail);

void t_strncat(char *dest1, char *dest2, char *tail);

int main() {
    g_strlen(10);
    g_strcat_strncat(10);
    return 0;
}

char *random_string() {
    int len = random_int();
    char *string = malloc(sizeof(char) * (len + 1));
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

void g_strlen(int num) {
    int i = 0;
    srand(i);
    for (; i < num; i++) {
        char *string = random_string();
        t_strlen(string);
        free(string);
    }
}

void t_strlen(char *str) {
    assert(q_strlen(str) == strlen(str));
    printf("t_strlen|str: %s\n", str);
}

void g_strcat_strncat(int num) {
    int i = 0;
    srand(i);
    char *tail = "tail";

    for (; i < num; i++) {
        char *s1, *s2;
        s1 = random_string();
        size_t size = sizeof(char) * (strlen(tail) + strlen(s1) + 1);

        s1 = realloc(s1, size);
        s2 = malloc(size);
        strcpy(s2, s1);

        t_strcat(s1, s2, tail);
        t_strncat(s1, s2, tail);

        free(s1);
        free(s2);
    }
}

void t_strcat(char *dest1, char *dest2, char *tail) {
    strcat(dest1, tail);
    q_strcat(dest2, tail);
    printf("strcat| str1: %s,str2: %s\n", dest1, dest2);
    assert(strcmp(dest1, dest2) == 0);
}

void t_strncat(char *dest1, char *dest2, char *tail) {
    int n = strlen(tail) - 1;
    strncat(dest1, tail, n);
    q_strncat(dest2, tail, n);
    printf("strncat| str1: %s,str2: %s\n", dest1, dest2);
    assert(strcmp(dest1, dest2) == 0);
}