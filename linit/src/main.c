#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define LINIT_VERSION "v0.0.1"

void show_motd();

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    show_motd();
    puts("linit "LINIT_VERSION", built "__DATE__"\n\n");

    return 0;
}
