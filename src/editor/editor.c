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
void cursorLeft(uint8_t distance) {
    if(!state.started) return;
    if(state.editor->currentChar > 0 && distance <= state.editor->currentChar) {
        int bytes = 4 + MAX_LINE_L_DIGITS;
        char *move = calloc(bytes, sizeof(char));
        snprintf(move, bytes, "\x1b[%dD", distance);

        state.editor->currentChar -= distance;
        write(STDIN_FILENO, move, bytes);
        free(move);
    }
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
void cursorRight(uint8_t distance) {
    if(!state.started) return;
    int tillEnd = (state.editor->lineLength - 1) - state.editor->currentChar;

    if(tillEnd > 0 && distance <= tillEnd) {
        int bytes = 4 + MAX_LINE_L_DIGITS;
        char *move = calloc(bytes, sizeof(char));
        snprintf(move, bytes, "\x1b[%dC", distance);

        state.editor->currentChar += distance;
        write(STDIN_FILENO, move, bytes);
        free(move);
    }
}

/*
    Function for moving the cursor up by some number of rows
    takes in the editor to update it and perform checks

    Checks that there is a non-zero distance between the current line and the start of the 
    text writing space, if there is and the distance the cursor is to be moved by is within that 
    amount of space then moves the cusor using an ANSI string before updating editor state
*/
void cursorUp(uint16_t distance) {
    if(!state.started) return;
    
    if(state.editor->currentLine > 0 && distance <= state.editor->currentLine) {
        int bytes = 4 + MAX_LINE_L_DIGITS;
        char *move = calloc(bytes, sizeof(char));
        snprintf(move, bytes, "\x1b[%dA", distance);

        state.editor->currentLine -= distance;
        write(STDIN_FILENO, move, bytes);
        free(move);
    }
}

/*
    Function for moving the cursor down by some number of rows
    takes in the editor to update it and perform checks

    Checks that there is a non-zero distance between the current line and the end of the 
    text writing space, if there is and the distance the cursor is to be moved by is within that 
    amount of space then moves the cusor with an ANSI string before updating editor state 
*/
void cursorDown(uint16_t distance) {
    if(!state.started) return;
    int toBottom = (state.editor->maxLines - 1) - state.editor->currentLine;

    if(toBottom > 0 && distance <= toBottom) {
        int bytes = 4 + MAX_LINE_L_DIGITS;
        char *move = calloc(bytes, sizeof(char));
        snprintf(move, bytes, "\x1b[%dB", distance);

        state.editor->currentLine += distance;
        write(STDIN_FILENO, move, bytes);
        free(move);
    }
}

/*
    Function for moving the cursor forwards by a character if this is possible

    Checks that there is another charachter in the current line and if so uses cursorRight
    other conditions applied are:


*/
void nextChar() {
    int curChar = state.editor->currentChar;
    int curLine = state.editor->currentLine;
    int lineUsed = state.editor->lines[curLine].len - 1; //remove 1 to make it index
    if(curChar <= lineUsed) {
        cursorRight(1);
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
    }
    if(state.editor->currentChar >= state.editor->lineLength) {
        if(state.editor->currentLine != state.editor->maxLines - 1) {
            cursorDown(1);
            cursorLeft(state.editor->lineLength);
            char blank = ' ';
            if(state.editor->lines[state.editor->currentLine].len == 0) {
                write(STDOUT_FILENO, &blank, 1);
                write(STDOUT_FILENO, "\x1b[1D", 4);
            }
        }
        else cursorLeft(1);
    }
}

/*
    Function that moves part of the current line to the next line 
    if there is space to do so
*/
void nextLine() {
    printf("AAAAAAAAAA\n");
    
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
    this->lineLength = lineLength;
    this->maxLines = maxLines;
    this->freeLines = maxLines;
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
    Function that returns the cursor to the start of the text section
*/
void toStart() {
    if(!state.started) return;
    cursorLeft(state.editor->currentChar);
    cursorUp(state.editor->currentLine);
}

/*
    Function that is called as part of clearLine to 
    reset the state related to the given line line

    If the editor has started and a valid line has been given then 
    use memset to reset the contents of the line and reset the length to 0
*/
int state_clearLine(uint16_t toClear) {
    if(!state.started) return ERR;
    else if(toClear < 0 || toClear > state.editor->maxLines) return ERR;
    memset(state.editor->lines[toClear].text, 0x0, state.editor->lineLength);
    state.editor->lines[toClear].len = 0;
    return SUC;
}

/*
    Function that clears the whole terminal
    
    Moves the terminal back to the topmost corner 
    then clears from this point to the end
    then calls the state_clearLine function on every line 
    to reset the in memory struct
*/
void clearWhole() {
    toStart();
    write(STDOUT_FILENO, "\x1b[2J", 4);
    for(int i = 0; i < state.editor->maxLines; i++) state_clearLine(i); 
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
    Function for performing a proper shutdown of the terminal
    resets the shell to canonical mode then clears the terminal contents
*/
void shutdown() {
    if(!state.started) return;
    setCanon();
    state.started = FALSE;
    clearWhole();
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