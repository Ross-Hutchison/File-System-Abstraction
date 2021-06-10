# File-System-Abstraction
FAT-8 file system abstraction written in C

Starting Idea
-------------------------------------------
After fixing some bugs from the file system the end goal is to improve it adding several  new features such as 
a proper directory structure, and a text based UI for interacting with it. This UI should include a 
basic text editor which will be run in a separate terminal window when a file is to be written or editied

This is a basic idea and during implementation new ideas will be added and implemented as plans change

Reminder
-------------------------------------------

In order to test the file system move the FS folder a basic check can be run using the $make check command in terminal, this test can be edited by altering the main.c file
the makefile will by default simply compile the basic test and remove the object files without running the test, the smae process can be performed in the editor folder to test the 
text editor

some potentially confusing commands in the makefile are explained here:

clean: removes all object files and executables
halfClean: removes all object files but keeps executables (used as part of default make command to keep src clean)
clearView: runs the shell $clear command (used to make test results more obvious by clearing the terminal beforehand)

Progress
-------------------------------------------
Fix busted File Allocation Table: Done needs checked
Fix compiler errors: Done
Fix memory leaks: TODO (may god have mercy)
Clean FS code: TODO
Create Editor control loop for input reading: In progress
Create in-memory buffer for file contents: Structs created no usage yet
Set up commands controlling editor: TODO
Set up file writing: TODO
Set up file navigation (move where text is edited): TODO
Set up file reading and editing: TODO
Set up child mode (take CLAs and return instead of writing): TODO
Set up text UI for FS: TODO
Set up FS executing editor in second shell as child): TODO
Set up handling of child return data: TODO

Possible Extensions
-------------------------------------------
Improve directory structure


