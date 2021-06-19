#ifndef EDITOR_H
#define EDITOR_H

//libraries to use
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h> //POSIX stuff
#include <ctype.h> //good for checking for control chars etc.
#include <string.h>

//constants to use
#define TRUE 1
#define FALSE 0
#define ERR -1
#define SUC 0
#define MAX_LINE_L_DIGITS 2
#define MAX_LINES_DIGITS 2 //number of digits of the max line length
/*
    struct for representing a single line
    contains:
        a character array for storing the line's contents
        an unsigned 8 bit int for storing the current length (allows more than max length so enough)
*/
typedef struct line_t {
    char *text;
    uint8_t len;
}line_t;

/*
    struct for representing the editor as a whole
    contains:
        The pointer to a line (acts as array of lines)
        an unsgned 8-bit int represeting what character is being looked at in the current line
        an unsigned 16-bit int representing the current line being looked at
*/
typedef struct editor_t {
    line_t *lines;
    uint8_t currentChar;
    uint16_t currentLine;
    uint8_t lineLength;
    uint16_t maxLines;
} editor_t;

//function for setting terminal to raw mode
void setRaw();

//function for resetting terminal to standard canonical mode
void setCanon();

//function to deal with errors that require a shutdown
void handleFatalError(char *msg);

//function called when the editor starts
void startup(editor_t *editor);

//constructor for a new editor struct
editor_t *new_editor(uint8_t lineLength, uint16_t maxLines); 

//Directional functions for the terminal cursor, includes editor for input checks
void cursorLeft(editor_t *editor, uint8_t distance);
void cursorRight(editor_t *editor, uint8_t distance);
void cursorUp(editor_t *editor, uint16_t distance);
void cursorDown(editor_t *editor, uint16_t distance);

//function for writing a char to output
void writeChar(editor_t *editor, char inpt);

//function that clears the current line
void clearLine(editor_t *editor);

//function that clears the whole editor
void clearWhole(editor_t *editor);

#endif