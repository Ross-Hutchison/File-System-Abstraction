#include "editor.h"

struct termios orig_stdin;
char isRaw = FALSE;


void handleFatalError(char *msg) {
    perror(msg);
    exit(-1);
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