#include "editor.h"

struct termios orig_stdin;
char isRaw = FALSE;


void handleFatalError(char *msg) {
    perror(msg);
    exit(-1);
}

editor_t *new_editor(uint8_t lineLength, uint16_t maxLines) {
    editor_t *this = calloc(1, sizeof(editor_t));
    this->currentChar = 0;
    this->currentLine = 0;
    this->lineLength = lineLength;
    this->maxLines = maxLines;
    this->lines = calloc(maxLines, sizeof(line_t));

    return this;
}

/*
    Resets the shell to canonical mode
    If the terminal is not in RAW mode then it returns immediately
    Otherwise the STDIN is set to its original value before it
    was last put into RAW mode, then the isRaw gloabal variable is updated
*/
void setCanon() {
    if(!isRaw) return;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_stdin) == ERR) handleFatalError("Could not reset to cannonical mode");
    isRaw = FALSE;
}

/*
    Function for performing a proper shutdown of the terminal
    resets the shell to canonical mode then clears the terminal contents
*/
void shutdown() {
    setCanon();
    write(STDOUT_FILENO, "\x1b[2J", 4);
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
    if(isRaw) return;
    atexit(shutdown);
    struct termios ctrl;
    if(tcgetattr(STDIN_FILENO, &orig_stdin) == ERR) handleFatalError("Could not enter cannonical mode");
    if(tcgetattr(STDIN_FILENO, &ctrl) == ERR) handleFatalError("Could not enter cannonical mode");

    ctrl.c_lflag &= ~(ECHO|ICANON|ISIG|IEXTEN);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("Could not enter cannonical mode");

    ctrl.c_iflag &= ~(IXON|ICRNL|BRKINT|ISTRIP|INPCK);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("Could not enter cannonical mode");

    ctrl.c_oflag &= ~(OPOST);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("Could not enter cannonical mode");

    ctrl.c_cflag |= CS8;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("Could not enter cannonical mode");

    ctrl. c_cc[VMIN] = 0;
    ctrl.c_cc[VTIME] = 1;  //every 0.1 seconds (measured in deciseconds - 10^-1 - )
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("Could not enter cannonical mode");

    isRaw = TRUE;
}

/*
    Function that shifts the cursor left by a given distance
    takes in the editor to update it and perform checks

    allocates enough memory for the ANSI string
    then checks if it should move back part of or all of the line
    it performs this update and then modifies the ditor state
*/
void cursorLeft(editor_t *editor, uint8_t distance) {
    int bytes = 4 + MAX_LINE_L_DIGITS;
    char *move = calloc(bytes, sizeof(char));
    if(distance < editor->currentChar) {
        snprintf(move, bytes, "\x1b[%dD", distance);
        editor->currentChar -= distance;
    }
    else {
        snprintf(move, bytes, "\x1b[%dD", editor->currentChar);
        editor->currentChar = 0;
    }

    write(STDIN_FILENO, move, bytes);
    free(move);
}

/*
    Function for moving the cursor right by a given distance
    takes in the editor to update it and perform checks

    Allocates enough memory for the ANSI string 
    then finds the distance from the current character to the end of the line
    if the distance to travel is less than this then travel that distance
    otherwise move that difference to the end of the line
    update the editor's current character respectively
*/
void cursorRight(editor_t *editor, uint8_t distance) {
    int bytes = 4 + MAX_LINE_L_DIGITS;
    char *move = calloc(bytes, sizeof(char));
    int tillEnd = editor->lineLength - editor->currentChar;

    if(distance < tillEnd) {
        snprintf(move, bytes, "\x1b[%dC", distance);
        editor->currentChar += distance;
    }
    else {
        snprintf(move, bytes, "\x1b[%dC", tillEnd);
        editor->currentChar = editor->lineLength;
    }
    write(STDIN_FILENO, move, bytes);
    free(move);
}

/*
    Function for moving the cursor up by some number of rows
    takes in the editor to update it and perform checks

    Allocates enough memory for the ANSI string 
    then checks the distance between the current line and the first line (0) 
    if this is less than the distance moves to the first line, otherwise
    moves that distance up 
*/
void cursorUp(editor_t *editor, uint16_t distance) {
    int bytes = 4 + MAX_LINE_L_DIGITS;
    char *move = calloc(bytes, sizeof(char));
    
    if(distance < editor->currentLine) {
        snprintf(move, bytes, "\x1b[%dA", distance);
        editor->currentLine -= distance;
    }
    else {
        snprintf(move, bytes, "\x1b[%dA", editor->currentLine);
        editor->currentLine = 0;
    }

    write(STDIN_FILENO, move, bytes);
    free(move);
}

/*
    Function for moving the cursor down by some number of rows
    takes in the editor to update it and perform checks

    Allocates enough memory for the ANSI string 
    then checks the distance between the current line and the last line 
    if this is less than the distance moves to the last line, otherwise
    moves that distance down 
*/
void cursorDown(editor_t *editor, uint16_t distance) {
    int bytes = 4 + MAX_LINE_L_DIGITS;
    char *move = calloc(bytes, sizeof(char));
    int toBottom = editor->maxLines - editor->currentLine;

    if(distance < toBottom) {
        snprintf(move, bytes, "\x1b[%dB", distance);
        editor->currentLine += distance;
    }
    else {
        snprintf(move, bytes, "\x1b[%dB", toBottom);
        editor->currentLine += toBottom;
    }
    write(STDIN_FILENO, move, bytes);
    free(move);
}

/*
    Function for writing with the editor

    takes in the editor and input and writes that output to the 
    screen, before updating the editor state
*/
void writeChar(editor_t *editor, char inpt) {
    write(STDOUT_FILENO, &inpt, 1);
    editor->currentChar++;
    if(editor->currentChar == editor->lineLength) {
        write(STDOUT_FILENO, "\n", 1);
        cursorLeft(editor, editor->lineLength);
        editor->currentLine++;
        editor->currentChar = 0;
    }
}


/*
    Function for handling the initial startup of the editor
    sets the terminal to RAW mode and prints the initial terminal ~s 
    at the start of each line
*/
void startup(editor_t *editor) {
    setRaw();
    for(int i = 0; i < editor->maxLines; i++) {
        writeChar(editor, '~');    //write tilde to screen
        cursorLeft(editor, 1); //move one back
        cursorDown(editor, 1); //move one down
    }
    cursorUp(editor, editor->maxLines);
}

/*
    Function that clears the current line and moves back to the start of it

    moves the cursor back by the number of characters
    then clears from that point to the end of the current line
    and resets the editor state
*/
void clearLine(editor_t *editor) {
     cursorLeft(editor, editor->currentChar);
     write(STDOUT_FILENO, "\x1b[2K", 4);
     editor->currentChar = 0;
}

/*
    Function that clears the whole editor display

    moves the terminal back to the topmost corner 
    then clears from this point to the end
*/
void clearWhole(editor_t *editor) {
    int backBytes = 3 + MAX_LINE_L_DIGITS;
    int upBytes = 3 + MAX_LINES_DIGITS;
    char *back = calloc(backBytes, sizeof(char));
    char *up = calloc(upBytes, sizeof(char));

    snprintf(back, backBytes, "\x1b[%dD", editor->currentChar);
    snprintf(up, upBytes, "\x1b[%dA", editor->currentLine);

    write(STDOUT_FILENO, back, backBytes);
    write(STDOUT_FILENO, up, upBytes);
}