#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <sys/wait.h>

#include <shell.h>

const char *_default_prompt = SHELL_PROMPT_STR;
const char *_prompt_var     = "PS1";

static void _handle_command(const char *cmd);

void shell_loop(void) {
    char cmd[256];

    setenv(_prompt_var, _default_prompt, 0);

    const char *prompt = getenv(_prompt_var);
    puts(prompt);
    while(1) {
        // TODO: Replace gets, as it is unsafe
        gets(cmd);
        if(strlen(cmd) > 0) {
            putchar('\n'); /* Workaround for '\n' character not being echoed right away via serial. */
            _handle_command(cmd);

            prompt = getenv(_prompt_var);
            puts(prompt);
        }
    }
}


/**
 * Splits a string into a list of the command and arguments. Modifies `str`
 * in-place.
 *
 * TODO: Allow use of special characters (including spaces) using a `\`
 *
 * @param str String to split
 *
 * @return List of pointers to string, terminated by a NULL pointer.
 */
static char **_split_command(const char *str) {
    /* @todo This could also be allocated if we use three runs */
#define MAX_PARAMS 32
    static char *params[MAX_PARAMS + 1];
    unsigned param_cnt = 0;

    /* @todo Should probably use malloc */
#define MAX_ENVLEN 32
    char env_str[MAX_ENVLEN];

#define RUN_ALLOC 0
#define RUN_REAL  1
    for(unsigned run = 0; run < 2; run++) {
#define SPLITFLAGS_INSTR        (    0x03)
#define SPLITFLAGS_INSTR_SINGLE (1UL << 0)
#define SPLITFLAGS_INSTR_DOUBLE (1UL << 1)
#define SPLITFLAGS_ENVVAR       (1UL << 2)
        unsigned flags     = 0;
        unsigned param     = 0;
        size_t   len       = 0;
        unsigned env_start = 0;

        for(unsigned i = 0; i < strlen(str); i++) {
            if(flags & SPLITFLAGS_INSTR) {
                if((flags & SPLITFLAGS_INSTR_SINGLE) && (str[i] == '\'')) {
                    flags &= ~SPLITFLAGS_INSTR_SINGLE;
                } else if((flags & SPLITFLAGS_INSTR_DOUBLE) && (str[i] == '"')) {
                    flags &= ~SPLITFLAGS_INSTR_DOUBLE;
                }

                /* @todo Support variables within strings */
            } else if(flags & SPLITFLAGS_ENVVAR) {
                /* @todo Support variable at end of string */
                if((str[i] != '_') && !isalnum(str[i])) {
                    size_t envlen = i - env_start;
                    if(envlen >= MAX_ENVLEN) {
                        printf("ERROR: Environment variable too long!\n");
                        goto FAILURE;
                    }
                    memcpy(env_str, &str[env_start], envlen);
                    env_str[envlen] = 0;
                    const char *env = getenv(env_str);
                    if(env == NULL) {
                        printf("ERROR: Environment variable %s not found!\n", env_str);
                        goto FAILURE;
                    }
                    if(run == RUN_REAL) {
                        strcpy(&params[param][len], env);
                    }
                    len += strlen(env);

                    flags &= ~SPLITFLAGS_ENVVAR;
                }
            }

            if(str[i] == '$') {
                flags |= SPLITFLAGS_ENVVAR;
                env_start = i + 1;
            } else if(isspace(str[i]) && !(flags & SPLITFLAGS_INSTR)) {
                if(run == RUN_ALLOC) {
                    params[param] = malloc(len+1);
                    if(params[param] == NULL) {
                        printf("ERROR: Could not allocate memory for parameter!\n");
                        goto FAILURE;
                    }
                    param_cnt++;
                } else if(run == RUN_REAL) {
                    params[param][len] = 0;
                }

                param++;
                len = 0;

                if(param >= MAX_PARAMS) {
                    printf("ERROR: Too many parameters!\n");
                    goto FAILURE;
                }

                while(isspace(str[i+1])) {
                    i++;
                }
            } else if((str[i] == '\\') && str[i+1]) {
                i++;
                if(run == RUN_REAL) {
                    params[param][len] = str[i];
                }
                len++;
            } else if((str[i] == '\'') && !(flags & SPLITFLAGS_INSTR)) {
                flags |= SPLITFLAGS_INSTR_SINGLE;
            } else if((str[i] == '\"') && !(flags & SPLITFLAGS_INSTR)) {
                flags |= SPLITFLAGS_INSTR_DOUBLE;
            } else if(!(flags & SPLITFLAGS_ENVVAR)) {
                if(run == RUN_REAL) {
                    params[param][len] = str[i];
                }
                len++;
            }
        }

        if(run == RUN_ALLOC) {
            params[param] = malloc(len+1);
            if(params[param] == NULL) {
                printf("ERROR: Could not allocate memory for parameter!\n");
                goto FAILURE;
            }
        } else if(run == RUN_REAL) {
            params[param][len] = 0;
        }

        params[param+1] = NULL;
    }

    return params;

FAILURE:
    for(unsigned i = 0; i < param_cnt; i++) {
        free(params[i]);
    }
    return NULL;
}

static int _execute_command(char **args) {
    char *cmd = args[0];

    if(cmd == NULL) {
        printf("ERR: No command found.\n");
        return 1;
    }

    int i = 0;
    while(args[++i]) {
        if(*args[i] == 0) break;
    }

    pid_t pid = fork();

    if(pid == -1) {
        printf("Error while forking!\n");
        return 1;
    } else if(pid == 0) { // Child process
        execvp(cmd, args);

        printf("`execve` returned, something went wrong!\n");
        exit(1);
    } else { // Parent process
        pid_t child = wait(NULL);
        if(child == -1) {
            printf("Something went wrong while waiting.\n");
        } else {
            /* @todo Get child's return status */
        }
    }

    return 0;
}

static void _handle_command(const char *str) {
    char **split = _split_command(str);

    if(split) {
        _execute_command(split);

        for(unsigned i = 0; split[i] != NULL; i++) {
            free(split[i]);
        }
    }
}