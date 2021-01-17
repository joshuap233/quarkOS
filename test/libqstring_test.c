#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tool.h"
#include "qstring.h"


void g_strlen(int num);

void g_strcat(int num, void (t_test)(char *, char *, char *));

void g_strcmp();

void t_strlen(char *str);


void t_strcat(char *dest1, char *dest2, char *tail);

void t_strncat(char *dest1, char *dest2, char *tail);

void t_strcmp(char *s1, char *s2, bool equal);

int main() {
    test_init();
    g_strlen(10);
    g_strcat(10, t_strcat);
    g_strcat(10, t_strncat);
    g_strcmp();
    return 0;
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

void g_strcat(int num, void (t_test)(char *, char *, char *)) {
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

        t_test(s1, s2, tail);

        free(s1);
        free(s2);
        free(string);
    }
}

void t_strcat(char *dest1, char *dest2, char *tail) {
    strcat(dest1, tail);
    q_strcat(dest2, tail);
    printf("strcat| str1: %s,str2: %s\n", dest1, dest2);
    assert(strcmp(dest1, dest2) == 0);
}

void t_strncat(char *dest1, char *dest2, char *tail) {
    size_t n = strlen(tail) - 1;
    strncat(dest1, tail, n);
    q_strncat(dest2, tail, n);
    printf("strncat| str1: %s,str2: %s\n", dest1, dest2);
    assert(strcmp(dest1, dest2) == 0);
}

void g_strcmp() {
    t_strcmp("test", "test", true);
    t_strcmp("", "", true);
    t_strcmp("test123", "test123", true);
    t_strcmp("test12", "tdet123", false);
    t_strcmp("testd12", "est123", false);
}

void t_strcmp(char *s1, char *s2, bool equal) {
    assert(q_strcmp(s1, s2) == equal);
}