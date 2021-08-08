#ifndef IO_H 
#define IO_H

#include "editor.h"
#include "cursorMovement.h"
#include <fcntl.h>

//function for writing a char to output 
void writeChar(char inpt);

//function that clears the current line 
void clearLine();

//function that clears the whole editor
void clearWhole();

//function that adds a new line into the middle of the editor space when possible, shifting all lower lines down by 1
char addLine();

//function that adds a new blank line after the current line and moves to it (if there is space to do so)
void handleNewLine();

//Function that clears and re-prints the whole of the output 
void rerenderOutput();

//Function for rerendering a specific line of the output 
void rerenderLine(LINE_MAX line);

//function that deletes a charachter or line
char handleDelete();
char removeChar();  //removes a character replacing it with a blank space and decementing the line's length 
char removeOrJoinLine();  //removes an empty line and shifts all lower lines up by one

//Function for checking if a file needs to be loaded (and loading if so)
char loadIfNeeded();

//function for setting the state data on the file being written to and/or read from
char setOpenFile(char * name);

//function for saving the file
char saveFile();

#endif