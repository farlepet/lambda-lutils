#include <stdio.h>
#include <stddef.h>
#include <dirent.h>

int command_ls(int argc, char **argv) {
    char *dirname = ".";
    if(argc > 1) {
        /* @todo Allow flags */
        dirname = argv[1];
    }

    DIR *dir = opendir(dirname);

    if(dir == NULL) {
        printf("ERR: Could not open '%s' as a directory.\n", dirname);
    }
    
    struct dirent *d;
    while((d = readdir(dir)) != NULL) {
        /* TODO: More advanced operations */
        /* @note There is currently a printf bug where a character directly
         * after a format specifier won't be printed if it's the end of the
         * format string */
        printf("%s \n", d->d_name);
    }


    return 0;
}