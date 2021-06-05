#ifndef FS_H
#define FS_H

//required libraries
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

//result definitions
#define SUC 0
#define ERR -1
#define TRUE 1
#define FALSE 0

//fs creation definitions
#define overwrite "w"
#define append "a"

//volume record definitions
#define VOLUME_RECORD_SIZE 12    // size in bits of the volume record   (3 32 bit (4 bytes) values)
#define IDENT 7     // used to identify the fs when checking it is mounted
#define BLOCK_SIZE 25    // the size of each storage block
#define MAX_ENTRIES 3   // the maximum number of entries to be stored in the root (only) directory

//fat table definitions
#define FAT_TABLE_SIZE 10 // remember 0 and 255 are reserved (so 256 becomes 254)
#define FIRST_FAT_INDEX 0  //index of first non-reserved FAT index (technically 1 but that messes up the array)
#define LAST_FAT_INDEX (FAT_TABLE_SIZE - 1) //index of last non-reserved FAT index

//root directory definitions
#define FILE_ENTRY_SIZE 36     //last value is 2bytes (16 bit) offset from 0 by 34 bytes
#define FILE_NAME_SIZE 32
#define FILE_METADATA_SIZE 2
#define DIRECTORY_SIZE (MAX_ENTRIES * FILE_ENTRY_SIZE)  //max size (in bytes) of the root directory
#define MAX_OPEN_FILES MAX_ENTRIES

//data region definitions
    //can hold 256 blocks with 2 (0 and 254) reserved but blocks still present
#define DATA_REGION_SIZE (FAT_TABLE_SIZE * BLOCK_SIZE)  // data region is this many bytes large

//file system definitions
#define FILE_SYSTEM_SIZE (VOLUME_RECORD_SIZE + FAT_TABLE_SIZE + DIRECTORY_SIZE + DATA_REGION_SIZE)

//Location variables
#define VOLUME_RECORD_OFST 0   //offset from start of file to volume record
#define FAT_REGION_OFST (VOLUME_RECORD_SIZE)   //offset from start of file to fat region
#define ROOT_DIR_OFST (FAT_TABLE_SIZE + FAT_REGION_OFST)   //offset from start of file to root directory
#define DATA_REGION_OFST (ROOT_DIR_OFST + DIRECTORY_SIZE) // offset from start of file to data region

//error handling definition
#define SLEEP_USECS 500000

// ----------housekeeping methods----------

// function for generating file system
int make_fs(char *store_name);                                                                                    //done

//function for mounting the file system
int mount_fs(char *store_name);                                                                                   //done

//Function for syncing the on disk fs with the in memory fs
//int fs_sync();                                                                                                  //done

//Function for un-mounting the file system
int umount_fs();                                                                                   //done - needs tested

// ----------operations methods----------

//Function for creating the file system file
int fs_create(char *name);                                                                                        //done

//Function for deleting the file system
int fs_delete(char *name);                                                                                        //done

//Function for getting the size (in bytes) of the fs
int fs_filesize(char *name);

int fs_open(char *name, int mode);                                                                                //done

int fs_close(int fildes);                                                                                         //done

//Function for reading from the file system (stored as a file)
int fs_read(int fildes, void *buf, size_t nbyte);

//Function for writing to the file system   (stored as a file)
int fs_write(int fildes, void *buf, size_t nbyte);

//Function for setting the file position of the fs to the given offset                                            //done
int fs_lseek(int fildes, off_t offset);

// ----------Testing methods----------

void printVolumeBoot();
void printFAT();
void printDirectory();
void printDataRegion();
int incrementFileSize(char *name);
void printFileDescriptors();
int manualBlockSet(int fd, char *toSet);



#endif