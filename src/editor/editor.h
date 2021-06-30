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
#define CTRL_CHAR(c) ((c) & 0x1f) //Not mine - https://viewsourcecode.org/snaptoken/kilo/03.rawInputAndOutput.html
#define MAX_CTRL_SEQ 3 //maximum length of handled control sequence - https://www.encyclopedia.com/computing/dictionaries-thesauruses-pictures-and-press-releases/control-sequence
//Control Sequences
#define ESC_CHAR '\x1b'
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
void startup(uint8_t lineLength, uint16_t maxLines);

//constructor for a new editor struct
editor_t *new_editor(uint8_t lineLength, uint16_t maxLines); 
void free_editor(editor_t *toFree);

//Directional functions for the terminal cursor, includes editor for input checks
void cursorLeft(uint8_t distance);
void cursorRight(uint8_t distance);
void cursorUp(uint16_t distance);
void cursorDown(uint16_t distance);

//function for writing a char to output
void writeChar(char inpt);

//function that clears the current line
void clearLine();

//function that clears the whole editor
void clearWhole();

#endif