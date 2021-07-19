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
#define INV 1
#define MAX_LINE_L_DIGITS 2
#define MAX_LINES_DIGITS 2 //number of digits of the max line length
#define CTRL_CHAR(c) ((c) & 0x1f) //Not mine - https://viewsourcecode.org/snaptoken/kilo/03.rawInputAndOutput.html

//Control Sequences and keys
#define ESC_CHAR '\x1b'
#define CARRIAGE_RETURN 13

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
    uint8_t lineLength;
    uint16_t currentLine;
    uint16_t maxLines;
    uint16_t inUse;
    uint8_t cursorX;
    uint16_t cursorY;
} editor_t;

//function for setting terminal to raw mode MAYBE REMOVE FROM HEADER, NOT TO BE CALLED BY NON_TERMINAL FILES
void setRaw();

//function for resetting terminal to standard canonical mode MAYBE REMOVE FROM HEADER, NOT TO BE CALLED BY NON_TERMINAL FILES
void setCanon();

//function to deal with errors that require a shutdown MAYBE REMOVE FROM HEADER, NOT TO BE CALLED BY NON_TERMINAL FILES
void handleFatalError(char *msg);

//function called when the editor starts
void startup(uint8_t lineLength, uint16_t maxLines);

//constructor for a new editor struct
editor_t *new_editor(uint8_t lineLength, uint16_t maxLines); 
void free_editor(editor_t *toFree);

//Directional functions for the terminal cursor, bounded by length of editor space MAYBE REMOVE FROM HEADER, NOT TO BE CALLED BY NON-TERMINAL FILES
char cursorLeft(uint8_t distance);
char cursorRight(uint8_t distance);
char cursorUp(uint16_t distance);
char cursorDown(uint16_t distance);
char cursorTo(uint8_t x, uint16_t y);
void toStart(); //REDUNDANT REMOVE WHEN CURSOR_TO IS COMP

//Directional functions for the terminal cursor, bounded by where characters are
char nextChar();    //move to the next character in the current line or next line if at end of current and there is a next
char previousChar(); //move to the previous character in the current line or the previous line if at the start of current and there is a previous
char nextLine();    //move to the next line if there is one
char previousLine();    //move to the previous line if there is one

//function for writing a char to output 
void writeChar(char inpt);

//function that clears the current line 
void clearLine();

//function that clears the whole editor
void clearWhole();

#endif