#include <stdio.h>
#include <stddef.h>
#include <dirent.h>

#include <sys/stat.h>

static void _display_file(const char *dir, const struct dirent *d) {
    /* @todo: Use dynamic buffer, and snprintf */
    char buff[256];
    snprintf(buff, sizeof(buff), "%s/%s", dir, d->d_name);
    
    struct stat s;
    if(stat(buff, &s)) {
        return;
    }

    /* @todo Check if directory */
    printf("-%c%c%c%c%c%c%c%c%c %u %d %s\n",
           ((s.st_mode&0400)?'r':'-'), ((s.st_mode&0200)?'w':'-'), ((s.st_mode&0100)?'x':'-'),
           ((s.st_mode&0040)?'r':'-'), ((s.st_mode&0020)?'w':'-'), ((s.st_mode&0010)?'x':'-'),
           ((s.st_mode&0004)?'r':'-'), ((s.st_mode&0002)?'w':'-'), ((s.st_mode&0001)?'x':'-'),
           /* @todo Suport long long in lambda-lib */
           s.st_ino, (int)s.st_size, d->d_name);
}

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
        _display_file(dirname, d);
    }


    return 0;
}