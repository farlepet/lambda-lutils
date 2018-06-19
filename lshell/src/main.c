#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define LSHELL_VERSION "v0.0.1"


const char *prompt_txt = "lshell>";

void show_motd();

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    show_motd();

    puts("lshell "LSHELL_VERSION", built "__DATE__"\n\n");
    
    char cmd[256];

    puts(prompt_txt);
    while(1) {
        gets(cmd);
        if(strlen(cmd) > 0) {
            printf("\nCommand: [%s]\n\n", cmd);
            puts(prompt_txt);
        }
    }

    for(;;);

    return 0;
}

#define MOTD_BUFF_LEN 32

void show_motd() {
    int fd = open("/etc/motd", O_RDONLY);
    if(fd < 0) return;

    char motd_buff[MOTD_BUFF_LEN];
    int  len;

    while((len = read(fd, motd_buff, MOTD_BUFF_LEN)) > 0) {
        write(STDOUT_FILENO, motd_buff, len);
    }

    putchar('\n');
    close(fd);
}
