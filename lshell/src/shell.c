#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sys/wait.h>

#include <shell.h>

const char *prompt_txt = SHELL_PROMPT_STR;

static char *envp[] = {
    "PATH=/",
    NULL
};

static void handle_command(char *cmd);

void shell_loop(void) {
    char cmd[256];

    puts(prompt_txt);
    while(1) {
        // TODO: Replace gets, as it is unsafe
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

static int execute_command(char **args) {
    char *cmd = args[0];

    if(cmd == NULL) {
        printf("ERR: No command found.\n");
        return 1;
    }

    printf("Command: [%s]\n", cmd);
    while(++args) {
        if(*args == 0) break;
        printf("  - Argument: [%s]\n", *args);
    }

    // TODO: Check that command exists, and check all elements in PATH

    pid_t pid = fork();

    if(pid == -1) {
        printf("Error while forking!\n");
        return 1;
    } else if(pid == 0) { // Child process
        execve(cmd, args, envp);

        printf("`execve` returned, something went wrong!\n");
        //return 1;
        exit(1);
    } else { // Parent process
        // TODO: Wait on child, then return return value
        printf("Waiting for child (%d) to exit...\n", pid);
        pid_t child = wait(NULL);
        if(child == -1) {
            printf("Something went wrong while waiting.\n");
        } else {
            printf("Child (%d) exited.\n", child);
        }
    }

    return 0;
}

static void handle_command(char *str) {
    char **split = split_command(str);

    execute_command(split);
}