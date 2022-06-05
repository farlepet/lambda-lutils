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

#define QUOTE_REPLACE  '\x1F' /* Quotes are replaced with this value, so they can be used next to an environment variable */
#define DOLLAR_REPLACE '\x01' /* Escaped '$' are replaced with this, so they do not trigger a variable repolacement */

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
static char **_args_split(const char *str) {
    /* @todo This could also be allocated if we use three runs */
#define MAX_PARAMS 32
    static char *params[MAX_PARAMS + 1];
    unsigned param_cnt = 0;

#define RUN_ALLOC 0
#define RUN_REAL  1
    for(unsigned run = 0; run < 2; run++) {
#define SPLITFLAGS_INSTR        (    0x03)
#define SPLITFLAGS_INSTR_SINGLE (1UL << 0)
#define SPLITFLAGS_INSTR_DOUBLE (1UL << 1)
        unsigned flags     = 0;
        unsigned param     = 0;
        size_t   len       = 0;

        for(unsigned i = 0; i < strlen(str); i++) {
            if((flags & SPLITFLAGS_INSTR) &&
               (((flags & SPLITFLAGS_INSTR_SINGLE) && (str[i] == '\'')) ||
                ((flags & SPLITFLAGS_INSTR_DOUBLE) && (str[i] == '"')))) {
                flags &= ~SPLITFLAGS_INSTR;
                if(run == RUN_REAL) {
                    params[param][len++] = QUOTE_REPLACE;
                }
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
                    if(str[i] == '$') {
                        params[param][len] = DOLLAR_REPLACE;
                    } else {
                        params[param][len] = str[i];
                    }
                }
                len++;
            } else if((str[i] == '\'') && !(flags & SPLITFLAGS_INSTR)) {
                flags |= SPLITFLAGS_INSTR_SINGLE;
                if(run == RUN_REAL) {
                    params[param][len++] = QUOTE_REPLACE;
                }
            } else if((str[i] == '\"') && !(flags & SPLITFLAGS_INSTR)) {
                flags |= SPLITFLAGS_INSTR_DOUBLE;
                if(run == RUN_REAL) {
                    params[param][len++] = QUOTE_REPLACE;
                }
            } else {
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

/**
 * @brief Replaces variables with their values, and cleans up arguments
 *
 * @param args Pointer to agument array
 *
 * @return int 0 on success, else non-zero
 */
static int _args_replace(char **args) {
    for(unsigned i = 0; args[i]; i++) {
        printf("arg[%u]: %s\n", i, args[i]);
    }
    /* @todo Should probably use malloc */
#define MAX_ENVLEN 32
    char env_name[MAX_ENVLEN+1];

    for(unsigned i = 0; args[i] != NULL; i++) {
        size_t arg_len = strlen(args[i]);
        for(size_t j = 0; j < arg_len; j++) {
            if(args[i][j] == '$') {
                /* Variable */
                for(size_t k = j+1; k < (arg_len+1); k++) {
                    if(args[i][k] != '_' && !isalnum(args[i][k])) {
                        size_t var_sz = k - (j+1);
                        if(var_sz > MAX_ENVLEN) {
                            printf("Environment variable too long!\n");
                            return -1;
                        }

                        memset(env_name, 0, sizeof(env_name));
                        memcpy(env_name, &args[i][j+1], var_sz);
                        char *env_val = getenv(env_name);
                        if(env_val == NULL) {
                            /* @todo Should this just be a null-replacement? */
                            printf("Environment variable not found!\n");
                            return -1;
                        }

                        arg_len = (arg_len - (var_sz + 1)) + strlen(env_val);
                        char *newarg = realloc(args[i], arg_len);
                        if(newarg == NULL) {
                            printf("Could not realloc argument!\n");
                            return -1;
                        }
                        args[i] = newarg;

                        memmove(&args[i][j+strlen(env_val)], &args[i][k], strlen(&args[i][k]));
                        memcpy(&args[i][j], env_val, strlen(env_val));
                        args[i][arg_len] = '\0';

                        j = j + strlen(env_val);
                        break;
                    }
                }
            } else if(args[i][j] == DOLLAR_REPLACE) {
                /* Just the dollar symbol */
                args[i][j] = '$';
            } else if(args[i][j] == QUOTE_REPLACE) {
                /* Delete character. strlen+1 to inlcude null terminator. */
                memcpy(&args[i][j], &args[i][j+1], strlen(&args[i][j+1])+1);
                arg_len--;
            }
        }
    }

    return 0;
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
    char **args = _args_split(str);

    if(args) {
        if(_args_replace(args)) {
            goto CMD_DONE;
        }
        _execute_command(args);

CMD_DONE:
        for(unsigned i = 0; args[i] != NULL; i++) {
            free(args[i]);
        }
    }
}