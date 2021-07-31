#include "editor.h"
#include "cursorMovement.h"
#include "InputOutput.h"
#include <sys/poll.h>

int main() {
    char res;
    startup(10, 10);
    char nfds = 1;
    struct pollfd pfds[nfds];
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN; 
    
    while(TRUE) {
        if(poll(pfds, nfds, -1) != ERR) {
            if(pfds[0].revents & POLLIN) {
                char inpt = '\0';
                res = read(STDIN_FILENO, &inpt, 1);
                if(res > 0) {
                    // printf("%c - %d\n", inpt, inpt);   //DEBGUG PRINT
                    if(inpt == CTRL_CHAR(']')) break;
                    else if(inpt == CARRIAGE_RETURN) {
                        handleNewLine();
                    }
                    else if(inpt == DELETE) {
                        handleDelete();
                    }
                    else if(inpt == ESC_CHAR) {
                        char second;
                        char third;
                        if(read(STDIN_FILENO, &second, 1) <= 0) clearLine();
                        else if(read(STDIN_FILENO, &third, 1) <= 0) clearLine();
                        else if(second == '[') {
                            switch(third) {
                                case 'A':
                                    // cursorUp(1);
                                    previousLine();
                                    break;
                                case 'B':
                                    // cursorDown(1);
                                    nextLine();
                                    break;
                                case 'C':
                                // cursorRight(1);
                                    nextChar();
                                    break;
                                case 'D':
                                // cursorLeft(1);
                                    previousChar();
                                    break;
                            }
                        }
                    }
                    else writeChar(inpt);
                }
            }
        }
        else handleFatalError("Control loop has encountered a fatal error", 1);
    }
    handleFatalError("Moth closed successfully, have a nice day : )", 0);
}