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

void g_strcat(int num);

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

void g_strcat(int num) {
    int i = 0;
    srand(i);
    char *tail = "tail";
    char *s1, *s2, *string;

    for (; i < num; i++) {
        string = random_string();
        size_t size = sizeof(char) * (strlen(tail) + strlen(string) + 1);

        s1 = malloc(size);
        assert(s1 != NULL);
        s2 = malloc(size);
        assert(s2 != NULL);

        strcpy(s1, string);
        strcpy(s2, string);

        t_strcat(s1, s2, tail);

        free(s1);
        free(s2);
        free(string);
    }
}

void t_strcat(char *dest1, char *dest2, char *tail) {
    strcat(dest1, tail);
    q_strcat(dest2, tail);
//    printf("strcat| str1: %s,str2: %s\n", dest1, dest2);
    assert(strcmp(dest1, dest2) == 0);
}

void t_strncat(char *dest1, char *dest2, char *tail) {
    size_t n = strlen(tail) - 1;
    strncat(dest1, tail, n);
    q_strncat(dest2, tail, n);
//    printf("strncat| str1: %s,str2: %s\n", dest1, dest2);
    assert(strcmp(dest1, dest2) == 0);
}