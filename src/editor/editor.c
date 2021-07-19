#include "editor.h"

struct state {
    struct termios orig_stdin;
    char isRaw;
    char started;
    struct editor_t *editor;
}state;


void handleFatalError(char *msg) {
    perror(msg);
    exit(-1);
}

/*
    Function that shifts the cursor left by a given distance
    takes in the editor to update it and perform checks

    allocates enough memory for the ANSI string
    then checks if it should move back part of or all of the line
    it performs this update and then modifies the ditor state
*/
char cursorLeft(uint8_t distance) {
    if(!state.started || distance == 0) return INV;
    if(state.editor->cursorX > 0 && distance <= state.editor->cursorX) {
        int bytes = 4 + MAX_LINE_L_DIGITS;
        char *move = calloc(bytes, sizeof(char));
        snprintf(move, bytes, "\x1b[%dD", distance);

        state.editor->cursorX -= distance;
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
char cursorRight(uint8_t distance) {
    if(!state.started || distance == 0) return INV;
    int tillEnd = (state.editor->lineLength - 1) - state.editor->cursorX;

    if(tillEnd > 0 && distance <= tillEnd) {
        int bytes = 4 + MAX_LINE_L_DIGITS;
        char *move = calloc(bytes, sizeof(char));
        snprintf(move, bytes, "\x1b[%dC", distance);

        state.editor->cursorX += distance;
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
char cursorUp(uint16_t distance) {
    if(!state.started || distance == 0) return INV;
    
    if(state.editor->cursorY > 0 && distance <= state.editor->cursorY) {
        int bytes = 4 + MAX_LINE_L_DIGITS;
        char *move = calloc(bytes, sizeof(char));
        snprintf(move, bytes, "\x1b[%dA", distance);

        state.editor->cursorY -= distance;
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
char cursorDown(uint16_t distance) {
    if(!state.started || distance == 0) return INV;
    int toBottom = (state.editor->maxLines - 1) - state.editor->cursorY;

    if(distance > 0 && toBottom > 0 && distance <= toBottom) {
        int bytes = 4 + MAX_LINE_L_DIGITS;
        char *move = calloc(bytes, sizeof(char));
        snprintf(move, bytes, "\x1b[%dB", distance);

        state.editor->cursorY += distance;
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
char cursorTo(uint8_t x, uint16_t y) {
    int xDist = (x - state.editor->cursorX);
    int yDist = (y - state.editor->cursorY);
    char res;

    if(xDist < 0) res = cursorLeft( (xDist * -1) );
    else res = cursorRight(xDist);

    if(res == ERR) handleFatalError("System has tried to move cursor out of bounds\n");

    if(yDist < 0) res = cursorUp( (yDist * -1) );
    else res = cursorDown(yDist);

    if(res == ERR) handleFatalError("System has tried to move cursor out of bounds\n");

    return SUC;
}

/*
    Function that is called as part of clearLine to 
    reset the state related to the given line line

    If the editor has started and a valid line has been given then 
    sets the length of that line to 0 meaning that it will be treated 
    as a blank line
*/
int state_clearLine(uint16_t toClear) {
    if(!state.started) return INV;
    else if(toClear < 0 || toClear > state.editor->maxLines) return ERR;
    // memset(state.editor->lines[toClear].text, '\0', state.editor->lines[toClear].len);
    state.editor->lines[toClear].len = 0;
    return SUC;
}


/*
    Function that clears editor state for every line
*/
void state_clearWhole() {
    for(int i = 0; i < state.editor->maxLines; i++) state_clearLine(i);
}

/*
    Function that clears the whole terminal
    
    Moves the terminal back to the topmost corner 
    then clears from this point to the end
*/
void clearWhole() {
    cursorTo(0,0);
    write(STDOUT_FILENO, "\x1b[0J", 4);
}

/*
    Function that clears the current line and moves back to the start of it

    moves the cursor back by the number of characters
    then clears from that point to the end of the current line
    and resets the editor state
*/
void clearLine() {
    if(!state.started) return;
     cursorLeft(state.editor->currentChar);
     write(STDOUT_FILENO, "\x1b[2K", 4);
     state.editor->currentChar = 0;
     state_clearLine(state.editor->currentLine);
}

/*
    Function which re-renders the terminal display 
    clears the output which moves us back to the start of the text space 
    then iterates over each in-memory line and prints it
    then returns the cursor to where the last character was written.
*/
void rerenderOutput() {
    clearWhole();
    for(int i = 0; i < state.editor->maxLines; i++) {
        cursorTo(0, i);
        line_t current = state.editor->lines[i];
        if(current.len > 0) {
            write(STDOUT_FILENO, current.text, current.len);
            state.editor->cursorX = current.len;
        }
        else if(i >= state.editor->inUse) {
            write(STDOUT_FILENO, "~", 1);
            state.editor->cursorX = 1;
        }
    }
    cursorTo(state.editor->currentChar, state.editor->currentLine);
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

    if(curChar == state.editor->lineLength - 1) {
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
    if(curChar > 0 && curChar <= len) {
        cursorLeft(1);
        state.editor->currentChar--;
        return SUC;
    }
    else return ERR;
}

/*
    Function for adding a new line (shift everythign below down and add to line below current) 

    checks the number of lines in use 
    if there are less used lines than available can add a new line
    shift down (right in array terms) starting from the lowest in use line and moving back to currentLine + 1
    reset output by clearing and re-printing 
    make current line the newly "added" line
*/
char addLine() {
    if(!state.started) return INV;
    uint16_t using = state.editor->inUse;
    uint16_t available = state.editor->maxLines - using;
    if(available < 1) return ERR;
    else {
        uint16_t curLine = state.editor->currentLine;
        uint16_t lookingAt = curLine + 1;
        uint16_t last = using - 1;  // - 1 to get index
        for(int i = last; i > curLine; i--) {
            strcpy(state.editor->lines[i + 1].text, state.editor->lines[i].text);
            state.editor->lines[i + 1].len = strlen(state.editor->lines[i + 1].text);
        }
        state_clearLine(lookingAt);
        state.editor->inUse++;
        nextLine();
        return SUC;
    }
    
}

/*
    Function for writing with the editor

    Takes in the character that is being written it writes it to output and the in memory structure
    it then updates the editor state to represent this new character
*/
void writeChar(char inpt) {
    if(!state.started) return;
    if(state.editor->currentChar != state.editor->lineLength) {
        write(STDOUT_FILENO, &inpt, 1);
        uint8_t curChar = state.editor->currentChar;
        uint16_t curLine = state.editor->currentLine;
        state.editor->lines[curLine].text[curChar] = inpt;
        if(state.editor->currentChar >= state.editor->lines[curLine].len) state.editor->lines[curLine].len++; //if not overwriting a char increase length
        state.editor->currentChar++;
        state.editor->cursorX++;
    }
    if(state.editor->currentChar >= state.editor->lineLength) {
        if(state.editor->currentLine != state.editor->maxLines - 1) {
            state.editor->currentLine++;
            state.editor->currentChar = 0;
            cursorTo(0, state.editor->currentLine);
            if(state.editor->lines[state.editor->currentLine].len == 0) {
                write(STDOUT_FILENO, " ", 1);
                state.editor->cursorX++;
                cursorLeft(1);
            }
        }
        else nextChar();
    }
}

/*
    Function for handling a new line being created in the middle of an existing line 

    Starts by storing the current line and character for later use
    The calls addLine to perform the shifting needed to generate the new blank line 
    If addLine succeeds then it will shift the data of the old line from the old character onwards down to the next line 
    It will then update the editor state to reflect this change in line lengths
    Finally it will re-render the output
*/
void handleNewLine() {
    uint8_t oldCurChar = state.editor->currentChar;
    uint16_t oldCurLine = state.editor->currentLine;

    if(addLine() == SUC) {
        uint16_t curLine = state.editor->currentLine;
        uint8_t moved = 0;
        uint8_t prevLen = state.editor->lines[oldCurLine].len;
        for(int i = oldCurChar, j = 0; i < prevLen; i++, j++) {
            state.editor->lines[curLine].text[j] = state.editor->lines[oldCurLine].text[i];
            moved++;
        }
        state.editor->lines[oldCurLine].len -= moved;
        state.editor->lines[curLine].len += moved;

        rerenderOutput();
    }
}

/*
    Constructor fn for editor
    takes in the line length and number of lines 
    then uses this to initialise these and set the current char, line, and freeLines values 
    then it allocates memory for the array of lines and for each line will
    assign memory for the internal string
*/
editor_t *new_editor(uint8_t lineLength, uint16_t maxLines) {
    editor_t *this = calloc(1, sizeof(struct editor_t));
    this->currentChar = 0;
    this->currentLine = 0;
    this->cursorX = 0;      //treat start of editor as (0,0) right as x++ and down as y++
    this->cursorY = 0;  
    this->lineLength = lineLength;
    this->maxLines = maxLines;
    this->inUse = 1;
    this->lines = (line_t*)calloc(maxLines, sizeof(struct line_t));
    for(int i = 0; i < maxLines; i++) {
        this->lines[i].text = (char*)calloc(lineLength, sizeof(char));
    }

    return this;
}

/*
    Resets the shell to canonical mode
    If the terminal is not in RAW mode then it returns immediately
    Otherwise the STDIN is set to its original value before it
    was last put into RAW mode, then the isRaw gloabal variable is updated
*/
void setCanon() {
    if( !(state.isRaw && state.started) ) return;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &state.orig_stdin) == ERR) handleFatalError("Could not reset to cannonical mode");
    state.isRaw = FALSE;
}

/*
    Function for freeing a line
    both the line pointer and it's text are calloced so must be freed 
*/
void free_line(line_t toFree) {
    free(toFree.text);
}

/*
    Function for freeing an editor struct and it's attached lines 
*/
void free_editor(editor_t *toFree) {
    for(int i = 0; i < toFree->maxLines; i++) {
        free_line(toFree->lines[i]);
    }
    free(toFree->lines);
    free(toFree);
}

/*
    Function for performing a proper shutdown of the terminal
    resets the shell to canonical mode then clears the terminal contents
*/
void shutdown() {
    if(!state.started) return;
    setCanon();
    clearWhole();
    state.started = FALSE;
    free_editor(state.editor);
}

/*
    Creates a termios struct and uses it to
    put the terminal into raw mode to allow proper input processing 
    If the terminal is already in RAW mode then it instantly returns
    Othereise it updates the original terminal var and begins the modification

    Assigns the original stdin attributes to the origin struct for resetting later
    Assigns the stdin attributes to the control struct for modification
    Sets the c_lflag to what it was but without some flags (apply bitwise & against itself and negation of the mask of these flags, like +=)
        Removes terminal echo
        Removes canonical mode processing
        Removes interupt and quit signals
        Removes Input processing
        Removes flow control, more input processing and misc flags for parity and signals
        Removes output processing
        Sets terminal size to be CS8
        Sets a mininmum of 0 bytes per read() call and adds a 100 ms timeout
    Assigns the modified control struct's paramaters to stdin altering it
    Set the isRaw variable to true
*/
void setRaw() {
    if(state.isRaw || !state.started) return;
    atexit(shutdown);
    struct termios ctrl;
    if(tcgetattr(STDIN_FILENO, &state.orig_stdin) == ERR) handleFatalError("Could not get intitial stdin data");
    if(tcgetattr(STDIN_FILENO, &ctrl) == ERR) handleFatalError("Could not get intitial stdin data");

    ctrl.c_lflag &= ~(ECHO|ICANON|ISIG|IEXTEN);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("setRAW - Could not set local flags");

    ctrl.c_iflag &= ~(IXON|IXOFF|ICRNL|BRKINT|ISTRIP|INPCK);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("setRAW - Could not set input flags");

    ctrl.c_oflag &= ~(OPOST);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("setRAW - Could not set output flags");

    ctrl.c_cflag |= CS8;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("setRAW - Could not set cflags flags");

    ctrl. c_cc[VMIN] = 0;
    ctrl.c_cc[VTIME] = 1;  //every 0.1 seconds (measured in deciseconds - 10^-1 - )
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("setRAW - Could not set read timeout");

    state.isRaw = TRUE;
}


/*
    Function for handling the initial startup of the editor
    sets the terminal to RAW mode and prints the initial terminal ~s 
    at the start of each line
*/
void startup(uint8_t lineLength, uint16_t maxLines) {
    state.started = TRUE;
    state.isRaw = FALSE;
    state.editor = new_editor(lineLength, maxLines);
    setRaw();
    char emptyLine = '~';
    char blank = ' ';
    for(int i = 0; i < state.editor->maxLines; i++) {
        write(STDOUT_FILENO, &emptyLine, 1);
        write(STDOUT_FILENO, "\x1b[1D", 4); //move one back, can't use cursorLeft since currentChar is not updated (not writing ~ to memory)
        cursorDown(1); //move one down
    }
    cursorUp(state.editor->maxLines - 1);
    write(STDOUT_FILENO, &blank, 1);    //remove first ~
    write(STDOUT_FILENO, "\x1b[1D", 4); //move one back, can't use cursorLeft since currentChar is not updated (not writing ~ to memory)
}