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
#define DELETE 127

//typedefs for easy changing of numeric sizes for editor variables
typedef uint8_t LINE_LEN;
typedef uint16_t LINE_MAX;


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
    struct for representing the actual editor
    contains:
        The pointer to a line (acts as array of lines)
        an unsgned 8-bit int represeting what character is being looked at in the current line
        an unsigned 16-bit int representing the current line being looked at
*/
typedef struct editor_t {
    line_t *lines;
    LINE_LEN currentChar;
    LINE_LEN lineLength;
    LINE_MAX currentLine;
    LINE_MAX maxLines;
    LINE_MAX inUse;
    LINE_LEN cursorX;
    LINE_MAX cursorY;
} editor_t;


//function for setting terminal to raw mode MAYBE REMOVE FROM HEADER, NOT TO BE CALLED BY NON_TERMINAL FILES
void setRaw();

//function for resetting terminal to standard canonical mode MAYBE REMOVE FROM HEADER, NOT TO BE CALLED BY NON_TERMINAL FILES
void setCanon();

//function to deal with errors that require a shutdown MAYBE REMOVE FROM HEADER, NOT TO BE CALLED BY NON_TERMINAL FILES
void handleFatalError(char *msg);

//function called when the editor starts
void startup(LINE_LEN lineLength, LINE_MAX maxLines);

//constructor for a new editor struct
editor_t *new_editor(LINE_LEN lineLength, LINE_MAX maxLines); 
void free_editor(editor_t *toFree);

//Directional functions for the terminal cursor, bounded by length of editor space MAYBE REMOVE FROM HEADER, NOT TO BE CALLED BY NON-TERMINAL FILES
char cursorLeft(LINE_LEN distance);
char cursorRight(LINE_LEN distance);
char cursorUp(LINE_MAX distance);
char cursorDown(LINE_MAX distance);
char cursorTo(LINE_LEN x, LINE_MAX y);

//Directional functions for the terminal cursor, bounded by where characters are
char nextChar();    //move to the next character in the current line or next line if at end of current and there is a next
char previousChar(); //move to the previous character in the current line or the previous line if at the start of current and there is a previous
char nextLine();    //move to the next line if there is one
char previousLine();    //move to the previous line if there is one

//function that adds a new blank line after the current line and moves to it (if there is space to do so)
void handleNewLine();

//function that deletes a charachter or line
char handleDelete();

//function for writing a char to output 
void writeChar(char inpt);

//function that clears the current line 
void clearLine();

//function that clears the whole editor
void clearWhole();

#endif