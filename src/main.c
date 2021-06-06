#include <stdio.h>
#include <string.h>
#include "fs.h"

int main() {
    char *store = "a.txt";
    if(make_fs(store) == ERR) return ERR;
    if(mount_fs(store) == ERR) return ERR;

    int fd1 = fs_create("file1");

    char *test1 = "The Heart Relentless beats to protect the skin of the world we understand; it is the Principle of The Thunderskin, The Velvet, The Lionsmith, and The Sister-and-Witch, of life, preservation, protection, union, and the drumbeat and dance that must never cease. It is unstoppable in the face of adversity, and its followers are often characterized by relentless cheerfulness and obsessive determination";

    fs_write(fd1, test1, 401);

    printDirectory();
    printDataRegion();

    return SUC;
}
