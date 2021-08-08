#include <stdio.h>
#include <string.h>
#include "fs.h"

int main() {
    char *store = "a.txt";
    if(make_fs(store) == ERR) return ERR;
    if(mount_fs(store) == ERR) return ERR;

    int fd1 = fs_create("file1");
    int fd2 = fs_create("file2");

    int test1 = 12;
    char test2[] = {'a', 'b', 'c', 'd'};
    char *test3 = "The Heart Relentless beats";
    long test4 = 1234567891011121314;

    fs_write(fd1, &test1, 2);
    fs_write(fd2, test2, 4);
    fs_write(fd1, test3, 26);
    fs_write(fd2, &test4, 19);

    printDirectory();
    printDataRegion();

    return SUC;
}
