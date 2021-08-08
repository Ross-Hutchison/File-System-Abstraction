#include "constructors.h"

//constructor for a new file
file *new_file(char *name, int16_t id, int16_t size){
    file *this = calloc(1, FILE_ENTRY_SIZE);
    strcpy(this->name, name);
    this->size = size;  //maybe find instead of add
    this->fatIndex = id;//maybe find instead of add

    return this;
}

//constructor for a new block
block *new_block(char *data) {
    block *this = calloc(1, BLOCK_SIZE);
    if(this == NULL) return NULL;
    strcpy(this->data, data);

    return this;
}

//constructor for a new file descriptor
descriptor *new_descriptor(file *reps, int mode, int fp) {
    descriptor *this = calloc(1, sizeof(descriptor));    //no mandatory fixed size
    if(this == NULL) return NULL;
    this->represents = reps;
    this->mode = mode;
    this->fp = fp;

    return this;
}

//constructor for the volume boot record
volumeBootRecord *new_VBR() {
    volumeBootRecord *this = calloc(1, VOLUME_RECORD_SIZE);
    if(this == NULL) return NULL;
    this->fsId = IDENT;
    this->blockSize = BLOCK_SIZE;
    this->maxFiles = MAX_ENTRIES;

    return this;
}

//constructor for the FAT
fatTable *new_FAT() {
    fatTable *this = calloc(1, FAT_TABLE_SIZE);

    //set up the values of an empty fat table
    this->table[0] = '\0'; //the number 0 represents the reserved block (0 != '0)
    for(int i = FIRST_FAT_INDEX; i <= LAST_FAT_INDEX; i++) {
        this->table[i] = '0';   //character '0' represents a free space
    }
    this->nextFreeSlot = -1; // cannot be sure if a loaded FAT has free space or not, start at error and update when loading
    this->storing = 0;  //keeps track of the number of full indexes

    return this;
}

//constructor for the root Dir
rootDirectory *new_rootDir() {
    rootDirectory *this = calloc(1, DIRECTORY_SIZE);

    // for each entry in the directory initialise an empty file struct
    for(int i = 0; i < MAX_ENTRIES; i++) {
        this->files[i] = new_file("", -1, -1);  //array of pointers, only data is written to disk fle
    }
    this->storing = 0;
    this->nextFreeSlot = -1; // cannot be sure if a loaded dir has free space or not, start at error and update when loading

    return this;
}

//constructor for the data region
dataRegion *new_dataRegion() {
    dataRegion *this = calloc(1, DATA_REGION_SIZE);

    //for each block in the data block region   (one block per FAT index)
    for(int i = 0; i < FAT_TABLE_SIZE; i++) {
        this->blocks[i] = new_block(""); //array of pointers only data is written to disk file
    }

    return this;
}

//constructor for the file system
fileSystem *new_fileSystem(volumeBootRecord *vBoot, fatTable *fat, rootDirectory *dir, dataRegion *blocks) {
    fileSystem *this = calloc(1, FILE_SYSTEM_SIZE);
    this->vmb = vBoot;
    this->table = fat;
    this->dir = dir;
    this->storage = blocks;

    return this;
}

// ----------freeing methods----------

//frees the memory allocated to the given file struct
void free_file(file *toFree) {
    free(toFree);
    toFree = NULL;
}

//frees the memory allocated to the given block struct
void free_block(block *toFree) {
    free(toFree);
    toFree = NULL;
}

/*
 * frees the memory allocated to the given file descriptor struct
 * doesn't free the internal file as it will be freed with the directory
*/
void free_descriptor(descriptor *toFree) {
    free(toFree);
    toFree = NULL;
}

//frees the memory allocated to the given volume boot record
void free_VBR(volumeBootRecord *toFree) {
    free(toFree);
    toFree = NULL;
}

//frees the memory allocated to the given File Allocation Table
void free_FAT(fatTable *toFree) {
    free(toFree);
    toFree = NULL;
}

/*
 * frees the memory allocated to the given rootDirectory
 * first frees all of the directory's files
 * then frees the directory itself
 */
void free_rootDirectory(rootDirectory *toFree) {
    for(int i = 0; i < MAX_ENTRIES; i++) {
        free(toFree->files[i]);
    }
    free(toFree);
    toFree = NULL;
}

/*
 * frees the memory allocated to the given data region
 * first frees all of the blocks
 * then frees the data region itself
 */
void free_dataRegion(dataRegion *toFree) {
    for(int i = 0; i < FAT_TABLE_SIZE; i++) {
        free(toFree->blocks[i]);
    }
    free(toFree);
    toFree = NULL;
}

/*
 * frees the memory allocated to the given file system
 * first frees the volume boot record
 * then the FAT
 * then the directory
 * then the data region
 * then the file system itself
 */
void free_fileSystem(fileSystem *toFree) {
    free_VBR(toFree->vmb);
    toFree->vmb = NULL;

    free_FAT(toFree->table);
    toFree->table = NULL;

    free_rootDirectory(toFree->dir);
    toFree->dir = NULL;

    free_dataRegion(toFree->storage);
    toFree->storage = NULL;

    free(toFree);
    toFree = NULL;
}

