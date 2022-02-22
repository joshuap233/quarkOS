//
// Created by pjs on 2021/6/15.
//

#include <types.h>
#include "lib.h"
#include "malloc.h"

#define TERMINAL_BUF_SIZE 256

char buf[TERMINAL_BUF_SIZE];
//实际可用大小为 255， 还有 1字节用于解析命令

void hello() {
    char space[] = "                ";
    printf("\n");
    printf("%s************************************************\n", space);
    printf("%s*                                              *\n", space);
    printf("%s*                                              *\n", space);
    printf("%s*              Welcome to Quark OS             *\n", space);
    printf("%s*                                              *\n", space);
    printf("%s*                                              *\n", space);
    printf("%s************************************************\n", space);
    printf("\n");
}
struct cmd{
    void *name;
    int32_t (*func)(void *args);
    struct cmd *next;
};

#define COMPARE(cmd) memcmp(buf,cmd,sizeof(cmd)-1)

void run_cmd(u16_t end){
    u16_t args;
    for (int i = 0; i < end; ++i) {
        if (buf[i] == ' '){
            buf[i] = '\0';
            args = i+1;
            break;
        }
    }
    buf[end] = '\0';

    if (COMPARE("clear")){
        cls();
    } else if (COMPARE("ls")){

    } else if (COMPARE("cat")){

    }else if(COMPARE("echo")){
        printf("%s\n",&buf[args]);
    }else if (COMPARE("ln")){

    } else if(COMPARE("rm")){

    }else if (COMPARE("wc")){

    }else if(COMPARE("mkdir")){

    }
}

_Noreturn void shell() {
    while (1) {
        u16_t i = 0;
        printf("> ");
        do {
            gets(buf+i, 1);
            i++;
            assert(i!=TERMINAL_BUF_SIZE-1);
        } while (buf[i-1]!='\n');

        puts("\n",1);
        if (i>1){
            run_cmd(i);
        }
    }
}

int main() {
#ifdef TEST
    test_malloc();
#endif //TEST

    cls();
    hello();
    shell();
    return 0;
}


