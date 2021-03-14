#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define CAT_BUFF_LEN 32

static int _cat_file(char *file);

int command_cat(int argc, char **argv) {
    if(argc == 1) {
        return _cat_file("--");
    } else {
        /* @todo Handle flags */
        for(int i = 1; i < argc; i++) {
            if(_cat_file(argv[i])) {
                return 1;
            }
        }
    }

    return 0;
}

static int _cat_file(char *file) {
    int fd;
    if(!strcmp(file, "--")) {
        /* Read from STDIN */
        fd = STDIN_FILENO;
    } else {
        int fd = open(file, O_RDONLY);
        if(fd < 0) return -1;
    }

    char   buff[CAT_BUFF_LEN];
    size_t len;

    while((len = read(fd, buff, CAT_BUFF_LEN)) > 0) {
        write(STDOUT_FILENO, buff, len);
    }

    close(fd);

    return 0;
}