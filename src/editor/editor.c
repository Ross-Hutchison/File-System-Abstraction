#include "editor.h"

state_t state;


void handleFatalError(char *msg, int status) {
    memcpy(state.exitMsg, msg, EXIT_MSG_LEN);
    state.exitMsg[EXIT_MSG_LEN] = '\0';
    state.exitCode = status;
    exit(status);
}

/*
    Constructor fn for editor
    takes in the line length and number of lines 
    then uses this to initialise these and set the current char, line, and freeLines values 
    then it allocates memory for the array of lines and for each line will
    assign memory for the internal string
*/
editor_t *new_editor(LINE_LEN lineLength, LINE_MAX maxLines) {
    editor_t *this = (editor_t*)calloc(1, sizeof(struct editor_t));
    this->currentChar = 0;
    this->currentLine = 0;
    this->lineLength = lineLength;
    this->maxLines = maxLines;
    this->inUse = 1;
    this->lines = (line_t*)calloc(maxLines, sizeof(struct line_t));
    for(int i = 0; i < maxLines; i++) {
        this->lines[i].text = (char*)calloc(lineLength + 1, sizeof(char));
    }

    return this;
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
    Resets the shell to canonical mode
    If the terminal is not in RAW mode then it returns immediately
    Otherwise the STDIN is set to its original value before it
    was last put into RAW mode, then the isRaw gloabal variable is updated
*/
void setCanon() {
    if( !(state.isRaw && state.started) ) return;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &state.orig_stdin) == ERR) handleFatalError("Could not reset to cannonical mode", INV);
    state.isRaw = FALSE;
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
    struct termios ctrl;
    if(tcgetattr(STDIN_FILENO, &state.orig_stdin) == ERR) handleFatalError("Could not get intitial stdin data", INV);
    if(tcgetattr(STDIN_FILENO, &ctrl) == ERR) handleFatalError("Could not get intitial stdin data", INV);

    ctrl.c_lflag &= ~(ECHO|ICANON|ISIG|IEXTEN);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("setRAW - Could not set local flags", INV);

    ctrl.c_iflag &= ~(IXON|IXOFF|ICRNL|BRKINT|ISTRIP|INPCK);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("setRAW - Could not set input flags", INV);

    ctrl.c_oflag &= ~(OPOST);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("setRAW - Could not set output flags", INV);

    ctrl.c_cflag |= CS8;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("setRAW - Could not set cflags flags", INV);

    ctrl. c_cc[VMIN] = 0;
    ctrl.c_cc[VTIME] = 1;  //every 0.1 seconds (measured in deciseconds - 10^-1 - )
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctrl) == ERR) handleFatalError("setRAW - Could not set read timeout", INV);

    state.isRaw = TRUE;
}

/*
    Function for performing a proper shutdown of the terminal
    resets the shell to canonical mode then clears the terminal contents
*/
void shutdown() {
    if(!state.started) return;
    setCanon();
    write(STDOUT_FILENO, "\x1b[2J", 4);
    char outBytes = EXIT_MSG_LEN + 65+ 1; //"---------------\nMoth has closed with status XXX\nMSG\n---------------"
    char *output = calloc(outBytes, sizeof(char)); 
    snprintf(output, outBytes, "---------------\nMoth has closed with status %d\n%s\n---------------\n", state.exitCode, state.exitMsg);
    write(STDOUT_FILENO, output, outBytes);
    free_editor(state.editor);
    free(state.exitMsg);
    free(state.openFile);
    free(output);
    if(state.storage != NULL) fclose(state.storage);
}

/*
    Function for handling the initial startup of the editor
    sets the terminal to RAW mode and prints the initial terminal ~s 
    at the start of each line
*/
void startup(LINE_LEN lineLength, LINE_MAX maxLines) {
    atexit(shutdown);
    state.started = TRUE;
    state.isRaw = FALSE;
    state.editor = new_editor(lineLength, maxLines);
    state.cursorX = 0;      //treat start of editor as (0,0) right as x++ and down as y++
    state.cursorY = 0;
    state.exitMsg = calloc(EXIT_MSG_LEN + 1, sizeof(char));
    state.exitCode = 0;
    state.openFile = calloc(MAX_FILENAME_LEN + 1, sizeof(char));
    setRaw();
    char emptyLine = '~';
    char blank = ' ';
    for(int i = 0; i < state.editor->maxLines; i++) {
        write(STDOUT_FILENO, &emptyLine, 1);
        write(STDOUT_FILENO, "\x1b[1D", 4); //move one back, can't use cursorLeft since currentChar is not updated (not writing ~ to memory)
        write(STDOUT_FILENO, "\x1b[1B", 4); //move one down
    }

    char *up = (char*)calloc(1, maxLines);
    char upBytes = 4 + MAX_LINES_DIGITS;
    snprintf(up, upBytes,"\x1b[%dA", maxLines);
    write(STDOUT_FILENO, up, upBytes);

    write(STDOUT_FILENO, &blank, 1);    //remove first ~
    write(STDOUT_FILENO, "\x1b[1D", 4); //move one back, can't use cursorLeft since currentChar is not updated (not writing ~ to memory)
    
    free(up);
}