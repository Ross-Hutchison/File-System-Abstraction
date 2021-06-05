#ifndef CONSTRUCTORS_H
#define CONSTRUCTORS_H

#include "fs.h"
#include <errno.h>
#include <string.h>

//----------type definitions----------

//struct to define the files stored in the root directory
typedef struct file {
    char name[FILE_NAME_SIZE];
    int16_t fatIndex;
    int16_t size;
}file;

//struct to define a block of data
typedef struct block {
    char data[BLOCK_SIZE];
}block;

typedef struct descriptor {
    file *represents;
    int mode;
    off_t fp; //offset for where in the set of data blocks it is, updated whenever read/write are used or the file is cleared
}descriptor;

//----------Section Structs----------

// a struct to define the structure of the volume boot record of the fs
typedef struct volumeBootRecord {
    int32_t fsId;
    int32_t blockSize;
    int32_t maxFiles;
}volumeBootRecord;

//a struct to define the structure of the File Allocation Table
typedef struct fatTable {
    u_char table[FAT_TABLE_SIZE]; //char is a single byte
    int nextFreeSlot; //used to keep track of where the next free index is
    int storing;
}fatTable;

//struct to define the structure of the root directory
typedef struct rootDirectory {
    file *files[MAX_ENTRIES];
    int storing; //used for a quick capacity check;
    int nextFreeSlot; //used to keep track of where the next free slot is in the directory
}rootDirectory;

//struct to define the structure of the Data Region
typedef struct dataRegion {
    //a char is a single byte, so this is a variable that can store enough bytes for a block * the number of blocks
    block *blocks[FAT_TABLE_SIZE];
}dataRegion;

//struct to define the structure of the file system
typedef struct fileSystem {
    volumeBootRecord *vmb;
    fatTable *table;
    rootDirectory *dir;
    dataRegion *storage;
}fileSystem;


// ----------constructor methods----------

//constructor for a new file
file *new_file(char *name, int16_t id, int16_t size);

//constructor for a new block;
block *new_block(char *data);

//constructor for a new file descriptor
descriptor *new_descriptor(file *reps, int mode, int fp);

//constructor for the volume boot record
volumeBootRecord *new_VBR();

//constructor for the FAT
fatTable *new_FAT();

//constructor for the root Dir
rootDirectory *new_rootDir();

//constructor for the data region
dataRegion *new_dataRegion();

//constructor for the file system
fileSystem *new_fileSystem(volumeBootRecord *vmb, fatTable *table, rootDirectory *dir, dataRegion *storage);

// ----------freeing methods----------

void free_file(file *toFree);
void free_block(block *toFree);
void free_descriptor(descriptor *toFree);
void free_VBR(volumeBootRecord *toFree);
void free_FAT(fatTable *toFree);
void free_rootDir(rootDirectory *toFree);
void free_dataRegion(dataRegion *toFree);
void free_fileSystem(fileSystem *toFree);

#endif