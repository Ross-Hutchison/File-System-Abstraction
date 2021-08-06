#include "cursorMovement.h"

extern state_t state;

/*
    Function that shifts the cursor left by a given distance
    takes in the editor to update it and perform checks

    allocates enough memory for the ANSI string
    then checks if it should move back part of or all of the line
    it performs this update and then modifies the ditor state
*/
char cursorLeft(LINE_LEN distance) {
    if(!state.started || distance == 0) return INV;
    if(state.cursorX > 0 && distance <= state.cursorX) {
        int bytes = 4 + MAX_LINE_L_DIGITS;
        char *move = calloc(bytes, sizeof(char));
        snprintf(move, bytes, "\x1b[%dD", distance);

        state.cursorX -= distance;
        write(STDIN_FILENO, move, bytes);
        free(move);
        return SUC;
    }
    else return ERR;
}

/*
    Function for moving the cursor right by a given distance
    takes in the editor to update it and perform checks

    Checks that there is a non-zero distance to the end of the line 
    and that the distance input is at most equal to this distance
    This means the cursot cannot be moved past the end of a line
    It then allocates enough memory for the ANSI string 
    and prints it moving the cursor before updating the state and 
    freeing the allocated string 
*/
char cursorRight(LINE_LEN distance) {
    if(!state.started || distance == 0) return INV;
    int tillEnd = (state.editor->lineLength - 1) - state.cursorX;

    if(tillEnd > 0 && distance <= tillEnd) {
        int bytes = 4 + MAX_LINE_L_DIGITS;
        char *move = calloc(bytes, sizeof(char));
        snprintf(move, bytes, "\x1b[%dC", distance);

        state.cursorX += distance;
        write(STDIN_FILENO, move, bytes);
        free(move);
        return SUC;
    }
    else return ERR;
}

/*
    Function for moving the cursor up by some number of rows
    takes in the editor to update it and perform checks

    Checks that there is a non-zero distance between the current line and the start of the 
    text writing space, if there is and the distance the cursor is to be moved by is within that 
    amount of space then moves the cusor using an ANSI string before updating editor state
*/
char cursorUp(LINE_MAX distance) {
    if(!state.started || distance == 0) return INV;
    
    if(state.cursorY > 0 && distance <= state.cursorY) {
        int bytes = 4 + MAX_LINE_L_DIGITS;
        char *move = calloc(bytes, sizeof(char));
        snprintf(move, bytes, "\x1b[%dA", distance);

        state.cursorY -= distance;
        write(STDIN_FILENO, move, bytes);
        free(move);
        return SUC;
    }
    else return ERR;
}

/*
    Function for moving the cursor down by some number of rows
    takes in the editor to update it and perform checks

    Checks that there is a non-zero distance between the current line and the end of the 
    text writing space, if there is and the distance the cursor is to be moved by is within that 
    amount of space then moves the cusor with an ANSI string before updating editor state 
*/
char cursorDown(LINE_MAX distance) {
    if(!state.started || distance == 0) return INV;
    int toBottom = (state.editor->maxLines - 1) - state.cursorY;

    if(distance > 0 && toBottom > 0 && distance <= toBottom) {
        int bytes = 4 + MAX_LINE_L_DIGITS;
        char *move = calloc(bytes, sizeof(char));
        snprintf(move, bytes, "\x1b[%dB", distance);

        state.cursorY += distance;
        write(STDIN_FILENO, move, bytes);
        free(move);
        return SUC;
    }
    else return ERR;
}

/*
    Function that moves the cursor
    to a specific (X,Y) without affecting the 
    current line and character of the editor
*/
char cursorTo(LINE_LEN x, LINE_MAX y) {
    int xDist = (x - state.cursorX);
    int yDist = (y - state.cursorY);
    char res;

    if(xDist < 0) res = cursorLeft( (xDist * -1) );
    else res = cursorRight(xDist);

    if(res == ERR) handleFatalError("System has tried to move cursor out of bounds", INV);

    if(yDist < 0) res = cursorUp( (yDist * -1) );
    else res = cursorDown(yDist);

    if(res == ERR) handleFatalError("System has tried to move cursor out of bounds", INV);

    return SUC;
}

/*
    Function that moves the cursor down by a line if this is possible

    Checks that the next line is within the range of in-use lines (i.e. there is at least one more in-use line than the currentLine)
    if so moves the cursor to that line
    Then checks if that next line has enough characters present that the currentChar is within the range of used characters
    If it does not it will determine the difference from the current character to the end of the line and move the cursor back that far
    It will decrease the currentCharacter to be the next free space in the line, unless that would take it over the line
*/
char nextLine() {
    int curLine = state.editor->currentLine;
    if(curLine < state.editor->inUse - 1) { //line 0 and 1 line in use means no next, line 0 and 2 in use means next
        cursorDown(1);
        state.editor->currentLine++;

        int newLen = state.editor->lines[curLine + 1].len;
        int curChar = state.editor->currentChar;
        if(newLen < curChar) {    //line not long enough to just move down
            int gap = curChar - newLen;
            cursorLeft(gap);
            state.editor->currentChar -= gap;
        }

        return SUC;
    }
    else return ERR;
}

/*
    Function for moving the cursor up to a previous line if this is possible
*/
char previousLine() {
    int curLine = state.editor->currentLine;
    if(curLine > 0) {
        cursorUp(1);
        state.editor->currentLine--;

        int nextLen = state.editor->lines[curLine - 1].len;
        int curChar = state.editor->currentChar;
        if(nextLen < curChar) {    //line not long enough to just move down
            int gap = curChar - nextLen;    //if on char 3 and next line is len 2 need to move to index 2 (spot after char at index 1)
            cursorLeft(gap);
            state.editor->currentChar -= gap; //currentChar becomes index of last char in new line
        }

        return SUC;
    }
    else return ERR;
}

/*
    Function for moving the cursor forwards by a character if this is possible

    Checks the current charachter is not the last charachter in use in the line
    If it is not, moves the cursor right and increases the currentCharacter
*/
char nextChar() {
    int curChar = state.editor->currentChar;
    int curLine = state.editor->currentLine;
    int lineUsed = state.editor->lines[curLine].len; //remove 1 for index, keep one so that it includes next free char

    if(curChar == lineUsed) {
        if(nextLine() == SUC) {
            cursorTo(0, state.editor->currentLine);
            state.editor->currentChar = 0;
            return SUC;
        }
        else return ERR;
    }
    else if(curChar < lineUsed) {
        cursorRight(1);
        state.editor->currentChar++;
        return SUC;
    }
    else return ERR;
}

/*
    Function for moving the cursor backwards by a charachter if this is possible 

    Checks that the current character is not at the start of a line 
    If this is the case then moves the cursor back by 1 and decreases currentChar
*/
char previousChar() {
    int curChar = state.editor->currentChar;
    int curLine = state.editor->currentLine;
    int len = state.editor->lines[curLine].len;
    if(curChar == 0) {
        if(previousLine() == SUC) {
            LINE_MAX new = state.editor->currentLine;
            LINE_LEN length = state.editor->lines[new].len;
            if(length != 0) length--; //if the line is not empty remove 1 to get the index
            cursorTo(length, new);
            state.editor->currentChar = length;
            return SUC;
        }
        else return ERR;
    }
    else if(curChar > 0 && curChar <= len) {
        cursorLeft(1);
        state.editor->currentChar--;
        return SUC;
    }
    else return ERR;
}