#include <stdio.h>
#include <string.h>

#include <shell.h>

const char *prompt_txt = SHELL_PROMPT_STR;

static void handle_command(char *cmd);

void shell_loop(void) {
    char cmd[256];

    puts(prompt_txt);
    while(1) {
        gets(cmd);
        if(strlen(cmd) > 0) {
            handle_command(cmd);
            puts(prompt_txt);
        }
    }
}

#define MAX_PARAMS 32 //!< Maximum number of parameters (including command) to support

/**
 * Splits a string into a list of the command and arguments. Modifies `str`
 * in-place.
 * 
 * TODO: Handle multiple spaces, and differing whitespace
 * TODO: Take into account quotation marks
 * TODO: Allow use of special characters (including spaces) using a `\`
 * TODO: Make thread safe (e.g. using malloc)
 * 
 * @param str String to split
 * 
 * @return List of pointers to string, terminated by a NULL pointer.
 */
static char **split_command(char *str) {
    static char *params[MAX_PARAMS + 1];
    params[0] = str;
    int param_idx = 1;

    while(*str != '\0') {
        if(*str == ' ') {
            *str = '\0';
            str++;
            params[param_idx++] = str;
            if(param_idx >= MAX_PARAMS) {
                break;
            }
        } else {
            str++;
        }
    }

    params[param_idx] = NULL;
    return params;
}

static void handle_command(char *str) {
    char **split = split_command(str);

    char *cmd = split[0];

    if(cmd == NULL) {
        printf("ERR: No command found.\n");
        return;
    }

    printf("Command: [%s]\n", cmd);
    while(++split) {
        if(*split == 0) break;
        printf("  - Argument: [%s]\n", *split);
    }

    // TODO: Actually handle command
}