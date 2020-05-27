#include <stdio.h>
#include <string.h>

#include <shell.h>

const char *prompt_txt = SHELL_PROMPT_STR;

void shell_loop(void) {
    char cmd[256];

    puts(prompt_txt);
    while(1) {
        gets(cmd);
        if(strlen(cmd) > 0) {
            printf("\nCommand: [%s]\n\n", cmd);
            puts(prompt_txt);
        }
    }
}