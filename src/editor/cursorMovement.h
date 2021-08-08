#ifndef MOVEMENT_H
#define MOVEMENT_H

#include "editor.h"

//Directional functions for the terminal cursor, bounded by length of editor space MAYBE REMOVE FROM HEADER, NOT TO BE CALLED BY NON-TERMINAL FILES
char cursorLeft(LINE_LEN distance);
char cursorRight(LINE_LEN distance);
char cursorUp(LINE_MAX distance);
char cursorDown(LINE_MAX distance);

//Directional function that will move the cursor to a specific coordinate as long as it is in bounds
char cursorTo(LINE_LEN x, LINE_MAX y);

//Directional functions for the terminal cursor, bounded by where characters are
char nextChar();    //move to the next character in the current line or next line if at end of current and there is a next
char previousChar(); //move to the previous character in the current line or the previous line if at the start of current and there is a previous
char nextLine();    //move to the next line if there is one
char previousLine();    //move to the previous line if there is one

#endif
