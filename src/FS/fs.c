#include "constructors.h"


//global variables
char mounted = FALSE;   // mounted starts false, is true when mount succeeds
static volumeBootRecord *vmb = NULL;    // the struct for storing the transient data of the VBR
static fatTable *table = NULL;  // the struct for storing the transient data of the FAT
static rootDirectory *rootDir = NULL;   //the struct for storing the transient data of the directory
static dataRegion *storage = NULL;  //the struct for storing the transient data for each data block
static fileSystem *fs = NULL;   // the struct for storing all other transient structs for ease of use
static descriptor *fds[MAX_OPEN_FILES];  //list of file descriptors for open files
static int filedes = -1;    // global variable for storing the file descriptor for the open disk file (hard storage)


// processing functions

//error handler fn for C system errors
char handleError_p(char *errMsg) {
    perror(errMsg);
    sleep(SLEEP_USECS);
    return ERR;
}

//error handler fn for internal logic errors
char handleError(char *errMsg) {
    fprintf(stderr, "%s\n", errMsg);
    usleep(SLEEP_USECS);
    return ERR;
}

/*
 * write the data of the volume boot record to a given file
 * returns 1 on success and -1 on failure
 * Does not need to take in the vmb as it is a global variable
 * could potentially lose the int fd param as well since there is a global filedes
*/
int writeVolumeBootRecord(int fd) {

    char bytes = 4; //each value in the VBR is at most 4 bytes

    if (write(fd, &(vmb->fsId), bytes) == ERR) return handleError_p("writeVolumeBootRecord - could not write id");
    if (write(fd, &(vmb->blockSize), bytes) == ERR)
        return handleError_p("writeVolumeBootRecord - could not write block size");
    if (write(fd, &(vmb->maxFiles), bytes) == ERR)
        return handleError_p("writeVolumeBootRecord - could not write max files");

    return SUC;
}

/*
 * write the data of the File Allocation Table to a given file
 * returns 1 on success and -1 on failure
 * Does not need to take in the FAT as it is a global variable
 * could potentially lose the int fd param as well since there is a global filedes
*/
int writeFAT(int fd) {
    char entryBytes = 1;    //each entry in the table is a single byte
    int offsetLocation = lseek(fd, FAT_REGION_OFST, SEEK_SET);
    if (offsetLocation == ERR) return handleError_p("writeFAT - could not shift fd");

    char *fat = table->table;
    for (int i = 0; i <= FAT_TABLE_SIZE; i++) {
        char current = fat[i];
        if (write(fd, &current, entryBytes) == ERR) return handleError_p("writeFAT - couldn't write index");
    }

    return SUC;
}

/*
 * write the data of the root (only) directory to a given file
 * returns 1 on success and -1 on failure
 * Does not need to take in the dir as it is a global variable
 * could potentially lose the int fd param as well since there is a global filedes
*/
int writeDir(int fd) {
    int offsetLocation = lseek(fd, ROOT_DIR_OFST, SEEK_SET);
    if (offsetLocation == ERR) return handleError_p("writeDir - could not move to directory offset");

    for (int i = 0; i < MAX_ENTRIES; i++) {
        file *current = rootDir->files[i];

        if (write(fd, current->name, FILE_NAME_SIZE) == ERR)
            return handleError_p("writeDir - could not write file entry name");
        if (write(fd, &(current->fatIndex), FILE_METADATA_SIZE) == ERR)
            return handleError_p("writeDir - could not write file fat index");
        if (write(fd, &(current->size), FILE_METADATA_SIZE) == ERR)
            return handleError_p("writeDir - could not write file size");

    }

    return SUC;
}

/*
 * write the data of data region to a given file
 * returns 1 on success and -1 on failure
 * Does not need to take in the data region struct as it is a global variable
 * could potentially lose the int fd param as well since there is a global filedes
*/
int writeDataRegion(int fd) {
    int offsetLocation = lseek(fd, DATA_REGION_OFST, SEEK_SET);
    if (offsetLocation == ERR) return handleError_p("writeDataRegion - could not move to data region offset");

    for (int i = FIRST_FAT_INDEX; i < MAX_ENTRIES; i++) {
        block *current = storage->blocks[i];
        if (write(fd, &(current->data), BLOCK_SIZE) == ERR)
            return handleError_p("writeDataRegion - could not write block");
    }

    return SUC;
}

/*
 * Initialises the File System with default data
 * VMB starts with it's values from the header
 * The FAT is initialised as completely free
 * The directory is initialised with a set of empty file entries
 * The data region is assigned a series of NULL values
 */
int initialise_fs(int fd) {
    //initialise the VMB as a new (empty) struct and write to "disk"
    vmb = new_VBR();
    if (vmb == NULL) return ERR;
    if (writeVolumeBootRecord(fd) == ERR) return ERR;

    //initialise the FAT as a new (empty) struct and write to "disk"
    table = new_FAT();
    if (table == NULL) return ERR;
    if (writeFAT(fd) == ERR) return ERR;

    //initialise the directory as a new (empty) struct and write to disk
    rootDir = new_rootDir();
    if (rootDir == NULL) return ERR;
    if (writeDir(fd) == ERR) return ERR;

    //initialise the data region as a new (empty) struct and write to disk
    storage = new_dataRegion();
    if (storage == NULL) return ERR;
    if (writeDataRegion(fd) == ERR) return ERR;

    return SUC;
}

/*
 * When a file system is created it:
 *  - creates a file with the given name to save persistent data -using create_fs
 *  - opens that file
 *  - creates the volume boot record and writes that data to the file
 *  - creates the fat table and writes that data to the file (at it's offset)
 *  - closes the file
 */
int make_fs(char *store_name) {
    int flags = O_WRONLY | O_CREAT | O_TRUNC;
    int mode = 0777;

    int fd = open(store_name, flags, mode);

    if (fd <= 0) return handleError_p("make_fs - could not create or open store file");
    else {

        initialise_fs(fd);

        close(fd);
        return SUC;
    }
}

/*
 * function for loading the data from the "disk" file into transient storage (struct)
 * checks the file system is mounted
 * then reads the contents of the volume boot record
 * then if they are read correctly writes those values into the vmb struct
 * if vmb is not initialised then it will initialise it
 */
int load_volumeBoot() {
    if (!mounted) return handleError("load_volumeBoot - file system not mounted");
    //move the pointer to the offset fot the volume boot
    if (lseek(filedes, VOLUME_RECORD_OFST, SEEK_SET) == ERR)
        return handleError_p("load_volumeBoot - could not move to volume boot offset");

    char bytes = 4;
    int32_t id;
    int32_t blockSize;
    int32_t maxFiles;

    if (read(filedes, &id, bytes) == ERR) return handleError_p("load_volumeBoot - could not read id");
    if (read(filedes, &blockSize, bytes) == ERR) return handleError_p("load_volumeBoot - could not read block size");
    if (read(filedes, &maxFiles, bytes) == ERR) return handleError_p("load_volumeBoot - could not read max files");

    if (vmb == NULL) vmb = new_VBR();

    vmb->fsId = id;
    vmb->blockSize = blockSize;
    vmb->maxFiles = maxFiles;

    return SUC;
}

/*
 * Loads the value of the FAT stored on "disk" to the in memory struct
 */
int load_fat() {
    if (!mounted) return handleError("load_fat - file system not mounted");

    //move the pointer to the offset fot the volume boot
    if (lseek(filedes, FAT_REGION_OFST, SEEK_SET) == ERR)
        return handleError_p(
                "load_fat - could not move to FAT offset");

    if (table == NULL) table = new_FAT();

    char bytes = 1;
    char current;

    for (int i = FIRST_FAT_INDEX; i <= LAST_FAT_INDEX; i++) {
        if (read(filedes, &current, bytes) == ERR) return handleError_p("load_fat - cannot read fat index");
        table->table[i] = current;

        //find the first free index (stays as -1 (ERR) if one does not exist)
        if (current == '0') {
            if (table->nextFreeSlot == -1 || table->nextFreeSlot > i) table->nextFreeSlot = i;
        } else table->storing++;  //if index is not free then it is full, plus the counter
    }

    return SUC;
}

/*
 * stores the directory data on the "disk" file into the transient struct
 */
int load_directory() {
    if (!mounted) return handleError("load_directory - file system not mounted");

    //move the file pointer to the directory's offset
    if (lseek(filedes, ROOT_DIR_OFST, SEEK_SET) == ERR)
        return handleError_p("load_directory - could not move to directory offset");

    if (rootDir == NULL) rootDir = new_rootDir();

    char name[FILE_NAME_SIZE];
    int16_t index;
    int16_t size;


    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (read(filedes, &name, FILE_NAME_SIZE) == ERR)
            return handleError_p("load_directory - cannot read file struct name");
        if (read(filedes, &index, FILE_METADATA_SIZE) == ERR)
            return handleError_p("load_directory - cannot read file struct index");
        if (read(filedes, &size, FILE_METADATA_SIZE) == ERR)
            return handleError_p("load_directory - cannot read file struct size");

        //create new file object and add it to the directory
        file *insert = new_file(name, index, size);
        rootDir->files[i] = insert;

        //if the file exists (non-default struct) then increase the storage count
        //otherwise check to see if a new firstFreeINdex should be assigned
        if (strcmp(insert->name, "") != 0) rootDir->storing++;
        else if (rootDir->nextFreeSlot == -1 || rootDir->nextFreeSlot > i) rootDir->nextFreeSlot = i;

    }

    return SUC;
}

/*
 * Loads the data from the "disk" file's data region to the transient in memory structs
 */
int load_dataRegion() {
    if (!mounted) return handleError("load_dataRegion - file system not mounted");

    if (lseek(filedes, DATA_REGION_OFST, SEEK_SET) == ERR) return ERR;

    if (storage == NULL) storage = new_dataRegion();
    char val[BLOCK_SIZE];

    for (int i = 0; i < FAT_TABLE_SIZE; i++) {
        if (read(filedes, val, BLOCK_SIZE) == ERR) return handleError_p("load_dataRegion - couldn't read block");
        storage->blocks[i] = new_block(val);
    }

    return SUC;
}

/*
 *  Function for setting the global file system struct to represent the in memory data
 *  calls all the other load functions and then creates a new file system using them
 *  then returns 0 (or -1 if it fails at any point)
 */
int load_fileSystem() {
    if (!mounted) return handleError("load_fileSystem - file system not mounted");

    if (load_volumeBoot() == ERR) return ERR;
    if (load_fat() == ERR) return ERR;
    if (load_directory() == ERR) return ERR;
    if (load_dataRegion() == ERR) return ERR;

    fs = new_fileSystem(vmb, table, rootDir, storage);
    if (fs != NULL) return SUC;
    else return ERR;
}

/*
 * syncs the transient data in the structs to the
 * hard memory in the file
 * meaning that all data represented in the
 * file corresponds to the process data
 *
 * then calls the posix sync function to sync the
 * hard data file changes to the device's main file system
 *
 * returns Success if everything is synced otherwise returns an error
 */
int fs_sync() {
    if (!mounted) handleError("cannot sync file system - file system has not been mounted");

    //sync the volume boot record - not necessary but safety
    if (writeVolumeBootRecord(filedes) == ERR)
        return handleError("cannot sync file system - could not sync volume boot record");

    if (writeFAT(filedes) == ERR)
        return handleError("cannot sync file system - could not sync FAT");

    if (writeDir(filedes) == ERR)
        return handleError("cannot sync file system - could not sync directory");

    if (writeDataRegion(filedes) == ERR)
        return handleError("cannot sync file system - could not sync data region");

    sync(); //sync is always successful

    return SUC;
}


/*
 * checks the file system is mounted and the specified file is open
 * then frees the specified file's file descriptor sets array index to NULL
 * then calls sync to write the updated file to memory
 *
 * if all checks pass returns 0 else returns -1
 */
int fs_close(int fd) {
    if (!mounted) return handleError("cannot close file - file system is not mounted");
    if (fds[fd] == NULL) return handleError("cannot close file - file is not open");

    //remove the file descriptor
    free_descriptor(fds[fd]);
    fds[fd] = NULL;

    if (fs_sync() == ERR) return handleError("could not sync file");

    return SUC;
}

/*
 * Mounts an existing file system
 * this allows it's data to be read and edited
 * it does this by:
 * - attempting to open the file
 * - reading the volume boot record
 * - isolating the initial value
 * - comparing it to the stored ident int
 * - if successfully mounted call the load functions to initialise the structs
 * - if one fails un-mount the system
 * (for now just return -1 and set values but after unmount is initialised then can use it)]
 *
 */
int mount_fs(char *store_name) {
    char vmbBytes = 4; //each volume boot record value is at most 4 bytes (if they are not 4 bytes the rest are NULL)
    int flags = O_RDWR;
    int fd = open(store_name, flags);
    if (fd == ERR) return handleError_p("mount_fs - cannot open file");

    int32_t ident;
    int32_t bs;
    int32_t mf;
    size_t readOut;

    if ((readOut = read(fd, &ident, vmbBytes)) == ERR) return handleError_p("mount_fs - cannot read from file");
    if (readOut == ERR) return handleError_p("mount_fs - could not read boot record");

    if ((readOut = read(fd, &bs, vmbBytes)) == ERR) return handleError_p("mount_fs - cannot read from file");
    if (readOut == ERR) return handleError_p("mount_fs - could not read boot record");

    if ((readOut = read(fd, &mf, vmbBytes)) == ERR) return handleError_p("mount_fs - cannot read from file");
    if (readOut == ERR) return handleError_p("mount_fs - could not read boot record");

    //check the file has a matching identifier to the header file macro
    if (IDENT != ident) return handleError_p("mount_fs - invalid Identifier");

    //These two should probably lead to changes being made, but that would require global vars instead of macros so do it later
    if (BLOCK_SIZE != bs) return handleError_p("mount_fs - file's block size different from Macro");
    if (MAX_ENTRIES != mf) return handleError_p("mount_fs - file's max files is different from Macro");

    //assign global variables
    mounted = TRUE;
    filedes = fd;

    //load the disk memory into transient storage
    load_fileSystem();

    return SUC;
}

/*
 * This function syncs the process structs (transient data)
 * with the hard file data meaning that all changes made to the file
 * using the structs will be written back to the file
 *
 * if syncing fails so will mount
 * otherwise mount will then close any open file descriptors
 * then close the underlying hard disk file
 * final step is to rest global variables and free structs
 * if all operations succeed it will return 0
 * otherwise it will return -1 and a relevant message
 */
int umount_fs() {
    //sync the process data to the file
    if (fs_sync() == ERR) return handleError("could not fully sync file system - no process changes made");

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        descriptor *current = fds[i];
        if (current != NULL) {
            fs_close(i);    //close and free the file descriptors
        }
    }

    //finally close the underlying file
    if (close(filedes) == ERR) return handleError_p("could not un-mount file system");

    //if all checks passed then reset the global variables
    mounted = FALSE;
    filedes = -1;

    //then free the transient data structs
    free_fileSystem(fs);

    return SUC;
}

//finds either a specific index and returns the value stored there
int fat_findFreeIndex(int iterateFrom) {
    if (!mounted) return handleError("cannot access FAT of non-mounted file system");

    if (iterateFrom < FIRST_FAT_INDEX || iterateFrom > LAST_FAT_INDEX)
        return handleError("fat_findFreeIndex - index out of bounds");

    for (int i = iterateFrom; i <= LAST_FAT_INDEX; i++) {
        char current = table->table[i];
        if (current == '0') return i;
    }
    return ERR;
}

/*
 * Iterative loop to find the next free space for a file
 * after the passed in index
 *
 * often used after adding a file to the previous firstFreeIndex
 * to look for the next free index after the last is filled
 */
int dir_findFreeIndex(int iterateFrom) {
    if (!mounted) return handleError("cannot access directory of non-mounted file system");

    if (iterateFrom < 0 || iterateFrom > MAX_ENTRIES) return handleError("dir_findFreeIndex - index out of bounds");

    for (int i = iterateFrom; i < MAX_ENTRIES; i++) {
        file *current = rootDir->files[i];
        if (strcmp(current->name, "") == 0) return i;
    }
    return ERR;
}

/*
 * takes in an index value
 * if valid returns the value stored in the FAT at that index
 */
char fat_findByIndex(int index) {
    if (!mounted) return handleError("cannot access FAT of non-mounted file system");

    if (index > LAST_FAT_INDEX || (index != -1 && index < FIRST_FAT_INDEX))
        return handleError("fat index out of bounds");

    return table->table[index];
}

/*
 * Search the directory struct for a file with a name matching the search
 */
int dir_search(char *name) {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        file *current = rootDir->files[i];
        if (strcmp(current->name, name) == 0) return i; //file at index i is what we are searching for
    }
    return ERR; //file with name not found
}

/*
 * Takes in a value and a block address, if the address is valid
 * writes it directly into the transient struct
 * value = "" can be used to clear
 */
int writeBlock(int index, char *value) {
    if (index < FIRST_FAT_INDEX || index > LAST_FAT_INDEX)
        return handleError("cannot write block - index out of bounds");

    else if (strcpy(storage->blocks[index]->data, value) == NULL)
        return handleError("cannot write block - writing error");

    return SUC;
}

/*
 * Recursively frees all FAT indexes in a chain starting from the given index
 * Also clears the data blocks that they represent
 */
int recursiveFATClear(int index) {
    //index check
    if (index < FIRST_FAT_INDEX || index > LAST_FAT_INDEX)
        return handleError("cannot clear FAT index - index out of bounds");

    else if (table->table[index] == '\0') {  // base case
        table->table[index] = '0';
        table->storing--;   //decrease the count of full blocks
        int res = writeBlock(index, "");
        if (res == ERR) {
            table->table[index] = '\0';  //undo deletion on error
            table->storing++;   //undo count decrease on error
        }
        if (index < table->nextFreeSlot)
            table->nextFreeSlot = index; //if removed index lower than lowest free then replace
        return res;

    } else {  //recursive case
        char next = table->table[index]; //take next block address from FAT
        int res = recursiveFATClear(next);
        if (res != ERR) {
            table->table[index] = '0';   //if last was cleared then clear this
            table->storing--; //decrease the count of taken blocks
            int wrote = writeBlock(index, "");
            if (wrote == ERR) {
                table->table[index] = next;  //undo deletion on error
                table->storing++;   //undo decrement to full index count on error
            }
            if (index < table->nextFreeSlot)
                table->nextFreeSlot = index; //if removed index lower than lowest free then replace
            return wrote;
        } else return ERR;
    }
}

/*
 * Takes in a file name and checks the open file descriptors
 * if there is at least one open file descriptor for the name then it
 * is open  and the function returns true, otherwise returns false
 */
int fileIsOpen(char *name) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        descriptor *current = fds[i];
        if (current != NULL) {
            char *check = current->represents->name;
            if (strcmp(check, name) == 0) return TRUE;
        }
    }
    return FALSE;
}

/*
 * goes through the array of open file descriptors and adds
 * the newly created one returning it's index
 * if there is no space returns error
 *
 * if the file already exists and is open for writing then
 * any request to open it again for writing is disallowed
 */
int addDescriptor(descriptor *toAdd) {
    int forWrite = (toAdd->mode == O_WRONLY);
    char *toAddName = toAdd->represents->name;
    int insertIndex = -1;

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        descriptor *current = fds[i];
        if (current == NULL) {   //replace insert with the first instance of a NULL entry
            if (insertIndex == -1) insertIndex = i;
        }
            // prevents two instances of a file being opened for writing
        else if ((strcmp(current->represents->name, toAddName) == 0) && current->mode == O_WRONLY && forWrite)
            return handleError("cannot open file - only one instance of a file can be open for writing at once");
    }
    if (insertIndex != ERR) {
        fds[insertIndex] = toAdd;
        return insertIndex;  //if it was added return the fd
    }

    return handleError("cannot open file - max open file descriptors reached");
}

/*
 * function for overwriting a file
 * Takes in the name of the file to overwrite
 * if it does not exist then throws an error (this should not occur it exists to prevent misuse)
 * Otherwise it checks if the file is open and if so will throw an error as open files cannot be
 * overwritten (with this method due to the affect it would have on file descriptors)
 * If the file exits but is not open then it will be cleared and replaced with an empty file
 * of the same name
 */
int overwriteFile(char *name) {
    int changeIndex = dir_search(name);
    if (changeIndex == ERR) return handleError("cannot overwrite file - file does not exists");
    if (fileIsOpen(name))
        return handleError(
                "cannot create file - file exists and cannot be overwritten: has at least one open file descriptor ");


    file *toChange = rootDir->files[changeIndex];
    int fatIndex = toChange->fatIndex;
    recursiveFATClear(fatIndex);
    rootDir->files[changeIndex] = new_file(name, fatIndex, 0);

    //create file descriptor for opened file and find int representation
    off_t fileDataLocation = DATA_REGION_OFST + (changeIndex * BLOCK_SIZE); //data region + blocks move past to start
    descriptor *desc = new_descriptor(toChange, O_WRONLY, fileDataLocation);

    //adds it to the global array and returns either an error or the int used to locate it
    int fd = addDescriptor(desc);
    return fd;
}

/*
 * Function for creating a new file using the given name
 * First checks the name is valid and that the system is mounted
 * Then checks that the directory has space for a new file to be created
 *
 * If these checks pass then it will check if a file with the name already exists
 * if one does it will attempt to overwrite it (see overwriteFile(char *name)
 * otherwise it will check that there is space in the FAT (i.e. in memory)
 * to store the new file throwing an error if there is not
 * it will then make this free entry the end of a chain and
 * find a new lowestFreeIndex
 *
 * We then create a new file and check that the value of
 * the directory's nextFreeSlot is valid before assigning the file there
 * and finding the next lowest free spot
 *
 * The final step is to create and return a file descriptor for the newly created file
 * in essence create opens the file for reading after making it
 *
 * The function returns either the file descriptor of the file or -1 if it or the file descriptor
 * cannot be created. If the file descriptor cannot be created the file still exist
 * but must have open called on it in order to be used.
 */
int fs_create(char *name) {
    if (!mounted) return handleError("cannot create file - file system is unmounted");
    if (name == NULL || strcmp(name, "") == 0) {
        return handleError("cannot create file - invalid filename");
    }

    if (!mounted) return handleError("cannot create file - system not mounted");
    if (rootDir->storing == MAX_ENTRIES) return handleError("cannot create file - directory is full");


    int fd;
    if ((dir_search(name)) == ERR) {

        int16_t index;
        if ((index = table->nextFreeSlot) == ERR) return handleError("cannot create file - no free space in FAT");
        table->table[index] = '\0';
        table->storing++; //one less free index in FAT
        table->nextFreeSlot = fat_findFreeIndex(
                index); //as we added to the first free index, the next free index must be larger

        file *new = new_file(name, index, 0);

        int insert;
        if ((insert = rootDir->nextFreeSlot) == ERR)
            return handleError("cannot create file - cannot find first free index in directory");

        else {
            rootDir->files[insert] = new;
            rootDir->storing++;
            rootDir->nextFreeSlot = dir_findFreeIndex(insert);

            //new files are created and opened for writing, starting at file pointer 0
            descriptor *desc = new_descriptor(new, O_WRONLY, 0);
            fd = addDescriptor(desc);
        }

    } else fd = overwriteFile(name);

    return fd;
}

/*
 * Function that deletes a file takes in the name of the file to delete
 * checks the file system is mounted and that the file exists in the directory
 * then checks that the file is not open
 *
 * takes the file from the directory struct and gets its FAT index
 * it uses this index for the call to recursive clear that will free all of the FAT
 * indexes allocated to this file and the related data blocks. This may update the FAT's
 * nextFreeSlot value if an index lower than it's current value is freed.
 *
 * If this is completely successful it will then clear the directory index for itself
 * setting it's values to those that represent a free file struct
 * freeing that slot for another file
 * this may update the directory's nextFreeSlot value if an index lower than its current
 * value is freed up
 *
 * then calls sync to update the file and returns either 0 or -1 if a check failed
 */
int fs_delete(char *name) {
    if (!mounted) return handleError("cannot delete file - file system is unmounted");
    int dirIndex;
    if ((dirIndex = dir_search(name)) == ERR) return handleError("cannot delete file - file not found");
    if (fileIsOpen(name)) return handleError("cannot delete file - file is currently open");

    file *toRemove = rootDir->files[dirIndex];
    int fatIndex = toRemove->fatIndex;
    if (recursiveFATClear(fatIndex) == ERR) return ERR;

    rootDir->files[dirIndex] = new_file("", -1, -1);
    if (dirIndex < rootDir->nextFreeSlot) rootDir->nextFreeSlot = dirIndex;

    if (fs_sync() == ERR) return ERR;

    return SUC;
}

/*
 * checks the directory contains the file name entered are looking for
 * then creates a descriptor for the file and adds it to the table
 * returns the file descriptor of the opened file or -1 if it does not exist
 */
int fs_open(char *name, int mode) {
    if (!mounted) return handleError("cannot open file - file system not mounted");

    //for now only read and write modes are permitted
    if (mode != O_RDONLY && mode != O_WRONLY) return handleError("could not open file - provided invalid mode");

    //find the file in the dir
    int fileIndex = dir_search(name);
    if (fileIndex == ERR) return handleError("cannot open file - file not found");

    //create a new description and add it to the
    file *file = rootDir->files[fileIndex];
    int block = file->fatIndex; //each FAT index represents a block

    //just opened files start with their file pointer at the start of the file
    descriptor *dec = new_descriptor(file, mode, 0);

    int fd = addDescriptor(dec);
    return fd;

}

/*
 * while there is no space in the current block move to the next block in it's chain
 * if there is no next block try and assign a new block
 * either way find the space left in the block and assign in to the variable
 */
size_t nextNonFullBlock(int *currentFatIndex, block **store) {
    size_t spaceLeft = 0;
    char currentFatValue;

    while (spaceLeft == 0) {
        currentFatValue = table->table[*currentFatIndex]; //take the value from the FAT at current index

        if (currentFatValue == '\0') {   //if end of chain
            if (table->nextFreeSlot == -1) return handleError("could not finish writing - no file space remaining");
            else {

                table->table[*currentFatIndex] = table->nextFreeSlot;    //assign old end of chain to point to new
                *currentFatIndex = table->nextFreeSlot;  //assign the current index to the newest FAT entry
                table->table[*currentFatIndex] = '\0';   //make it the end of the chain
                *store = storage->blocks[*currentFatIndex];   //move to this block
                table->nextFreeSlot = fat_findFreeIndex(*currentFatIndex);   //find the next free index in FAT

            }
        } else *store = storage->blocks[*currentFatIndex];

        //take the new store and update current size
        block *temp = *store;
        size_t dataSize = strlen(temp->data);
        spaceLeft = BLOCK_SIZE - dataSize;
    }
    return spaceLeft;
}

/*
 * Function for writing data to a file
 * takes in:
 *  it's file descriptor - used to find the file* object and other necessary data
 *  a buffer containing the data to write
 *  a size_t representing the number of bytes to read from that buffer
 *
 *  It will first check the file exists and is opened for writing
 *  then it will move to the file pointer and write UP TO
 *      the size of a block (keep track of how many bytes written)
 *  it will then update the file pointer
 *  if the block is not enough to store the buffer data
 *  find a new free block to write in
 *  if there is no free block inform user and exit with the number of bytes written
 *
 *  returns ERR instead of bytes written if:
 *      file does not exist
 *      file is not open
 *      file is not open for writing
 */
int fs_write(int fd, void *buffer, size_t nbytes) {
    //cast buffer to char array
    char *writeData = (char *) buffer;

    //get and check descriptor
    descriptor *desc;
    if ((desc = fds[fd]) == NULL) return handleError("cannot write file - file is not open");
    if (desc->mode != O_WRONLY) return handleError("cannot write file - file not open for writing");

    //now need to write from the buffer to the first dataBlock
    int writenTotal = 0; //the total number of bytes writen this call

    //gets the file from the descriptor and the start of it's fat chain
    file *writeTo = desc->represents;
    int currentIndex = writeTo->fatIndex;

    //gets the data block using the FAT index
    block *store = storage->blocks[writeTo->fatIndex];

    //checks how full this data block is - this will determine how much we can write
    size_t dataSize = strlen(store->data);
    size_t spaceLeft = BLOCK_SIZE - dataSize;

    //if space is 0 tries to move to the next block owned by this file
    if (spaceLeft == 0) spaceLeft = nextNonFullBlock(&currentIndex, &store);
    if (spaceLeft == -1) return writenTotal;

    size_t blockCounter = dataSize; //counter for writing the current block byte
    size_t bufferCounter = dataSize; //counter for writing the current buffer byte

    while (writenTotal != nbytes) {
        store->data[blockCounter] = writeData[bufferCounter];
        writenTotal++;
        desc->fp++;
        if(store->data[blockCounter] == '\0') desc->represents->size++;
        spaceLeft--;
        blockCounter++;
        bufferCounter++;


        if (spaceLeft == 0) spaceLeft = nextNonFullBlock(&currentIndex, &store);
        if (spaceLeft == -1) break;  //stop writing if there is no space
        blockCounter = strlen(store->data);
    }
    return writenTotal;
}

/*
 * function for moving to the next index in a chain to the
 * given index. Takes in the address of the index so it
 * can be updated without returning it
 *
 * returns the next block's address or NULL
 * if the end of the chain has been reached
 */
block *nextOwnedBlock(int *currentIndex) {
    char current = table->table[*currentIndex];
    if(current == '\0') {
        fprintf(stderr, "reached end of chain for this file\n");
        usleep(SLEEP_USECS);
        return NULL;
    }
    int blockLocation = *currentIndex; //store index of block
    *currentIndex = table->table[*currentIndex];    //make currentIndex next index in chain
    return storage->blocks[blockLocation];  //return found block
}

/*
 * takes in the offset the block you are moving through and the fp you are moving
 * it then find how much space is in the current block and how much is left to reach the offset
 * then if the entire current block's data will perfectly reach the offset or not reach the offset
 * then the whole block's length can be added to the file pointer - return holds -
 * otherwise we go one byte at a tie until the offset is reached and return
 * the number of bytes forward we read - return read -
 * if the the offset is somehow not less than, equal to or greater than the
 * amount of data held in the file then it returns an error as maths has failed us
 *
 * originally only let you move o written bytes to re-set to this, make holds = strlen(current->data)
*/
off_t movePointer(off_t offset, block *current, off_t fp) {
    if(current == NULL) return handleError("cannot move pointer - invalid block received");
    size_t holds = BLOCK_SIZE;
    off_t remaining = offset - fp;
    int read = 0;

    if(remaining >= holds) {   //if we have space to add the entire block content to the fp
        return holds;
    }
    else if(remaining < holds) { //if we can only add some of the block to the fp
        while(read != offset) { //update the fp until you reach the
            read++;
        }
        return read;
    }
    return ERR; //should never reach here, all cases covered
}


/*
 * This function takes in a file descriptor and an offset and moves
 * the file pointer for the file by the offset (assumed to always be SEEK_SET)
 * so we move the file pointer directly to that offset
 * as this allows both forwards and backwards motion through the file
 *
 * checks that the system is mounted and the file descriptor
 * points to an open file then uses the fd to get the file pointer
 *
 * sets the pointer to 0 to signify the start of the file,
 * finds the file the descriptor represents and it's first fat index
 * then moves forward as many bytes are in their first block,
 * if that does not reach the offset it finds the next
 */
int fs_lseek(int fd, off_t offset) {
    if(offset < 0) return handleError("cannot move file pointer - invalid offset: negative value");
    if(!mounted) return handleError("cannot move file pointer - System is not mounted");
    if(fds[fd] == NULL) return handleError("cannot move file pointer - file i snot open");

    descriptor *desc = fds[fd];
    off_t fp = 0; //start from beginning of file
    file *toShift = desc->represents;
    int fatIndex = toShift->fatIndex;
    block *current = storage->blocks[fatIndex];

    fp += movePointer(offset, current, fp);

    while(fp != offset) {   //not yet reached desired offset in file
        current = nextOwnedBlock(&fatIndex);    //move to next block
        if(current == NULL) return handleError("cannot move file pointer - invalid offset: offset exceeds file memory");

        fp += movePointer(offset, current, fp);
    }

    //reached offset - update file descriptor and return offset
    desc->fp = fp;
    return fp;
}


//test functions

void printDirectory() {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if(rootDir != NULL) {
            file *current = rootDir->files[i];
            printf("file %d\n", i + 1);
            printf("name: %s\n", current->name);
            printf("index: %d\n", current->fatIndex);
            printf("size: %d\n", current->size);
        }
    }
    printf("----------\n");
}

void printVolumeBoot() {
    if(vmb != NULL) {
        printf("volume boot\n");
        printf("identifier number %d\n", vmb->fsId);
        printf("block size %d\n", vmb->blockSize);
        printf("max files %d\n", vmb->maxFiles);
        printf("----------\n");
    }
}

void printFAT() {
    for (int i = FIRST_FAT_INDEX; i <= LAST_FAT_INDEX; i++) {
        if(table != NULL) printf("FAT section %d (index %d) holds %c\n", i + 1, i, table->table[i]);

    }
    printf("----------\n");
}

void printDataRegion() {
    for (int i = 0; i < FAT_TABLE_SIZE; i++) {
        if(storage != NULL) printf("block %d holds: %s\n", i, storage->blocks[i]->data);
    }
    printf("----------\n");
}

int incrementFileSize(char *name) {
    int update = dir_search(name);

    if (update == ERR) return handleError("cannot increment file's size - file does not exist");

    else {
        rootDir->files[update]->size++;
        return SUC;
    }
}

int manualBlockSet(int fd, char *setTo) {
    if(strlen(setTo) > BLOCK_SIZE) return handleError("too large for single block");
    descriptor *d = fds[fd];
    file *f = d->represents;
    int index = f->fatIndex;
    block *b = storage->blocks[index];

    strcpy(b->data, setTo);

    return SUC;
}

void printFileDescriptors() {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        descriptor *current = fds[i];
        if (current != NULL) {
            printf("file descriptor %d\n", i);
            printf("represents file %s \n", current->represents->name);
            printf("starting at FAT index %d\n", current->represents->fatIndex);
            printf("and of size %d\n", current->represents->size);
            printf("opened with mode %d\n", current->mode);
            printf("with file pointer at byte %d of the disk file\n", current->fp);
            printf("--------------------\n");
        }
    }
    printf("/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\\n");
}