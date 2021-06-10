#ifndef EDITOR_H
#define EDITOR_H

//libraries to use
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

//constants to use
#define TRUE 1
#define FALSE 0
#define MAX_LINE_L 10  //max number of characters per line (should be 196)
#define MAX_LINES 10 // max number of lines per file (should be much larger)

/*
    struct for representing a single line
    contains:
        a character array for storing the line's contents
        an unsigned 8 bit int for storing the current length (allows more than max length so enough)
*/
typedef struct line_t {
    char text[MAX_LINE_L];
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
} editor_t;

#endif