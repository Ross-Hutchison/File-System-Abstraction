#include "editor.h"

int main() {
    char res;
    startup(10, 10);

    while(TRUE) {   //replace with poll
        char inpt = '\0';
        res = read(STDIN_FILENO, &inpt, 1);
        if(res > 0) {
            // printf("%c\n", inpt);   //DEBGUG PRINT
            if(inpt == CTRL_CHAR(']')) {
                clearWhole();
                break;
            }
            else if(inpt == ESC_CHAR) {
                char second;
                char third;
                if(read(STDIN_FILENO, &second, 1) <= 0) clearLine(inpt);
                else if(read(STDIN_FILENO, &third, 1) <= 0) clearLine(inpt);
                else if(second == '[') {
                    switch(third) {
                        case 'A':
                            cursorUp(1);
                            break;
                        case 'B':
                            cursorDown(1);
                            break;
                        case 'C':
                            cursorRight(1);
                            break;
                        case 'D':
                            cursorLeft(1);
                            break;
                    }
                }
            }
            else writeChar(inpt);
        }
    }
    return 0;
}