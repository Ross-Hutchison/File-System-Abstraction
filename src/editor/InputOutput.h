#ifndef IO_H 
#define IO_H

#include "editor.h"
#include "cursorMovement.h"

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

//function that deletes a charachter or line
char handleDelete();

#endif