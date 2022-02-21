//
// Created by pjs on 2021/6/15.
//

#include <types.h>
#include "lib.h"

#define TERMINAL_BUF_SIZE 256

char buf[TERMINAL_BUF_SIZE];


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

void parse_line(){

}
#define COMPARE(cmd) memcmp(buf,cmd,sizeof(cmd)-1)

_Noreturn void shell() {
    printf("> ");
    u16_t i = 0;
    while (1) {
        i++;
        gets(buf, 1);
        if (buf[i] == ' '){
            if (COMPARE("clear")){

            } else if (COMPARE("ls")){

            } else if (COMPARE("cat")){

            }else if(COMPARE("echo")){

            }else if (COMPARE("ln")){

            } else if(COMPARE("rm")){

            }else if (COMPARE("wc")){

            }else if (COMPARE("kill")){

            }else if(COMPARE("mkdir")){

            }
        }
    }
}

int main() {
    cls();
    hello();
    shell();
    return 0;
}


