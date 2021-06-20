#include "editor.h"

int main() {
    char res;
    startup(10, 10);

    while(TRUE) {
        char inpt = '\0';
        res = read(STDIN_FILENO, &inpt, 1);
        if(res > 0) {
            if(inpt == CTRL_CHAR('}')) {
                clearWhole();
                break;
            }
            else if(inpt == '{') clearLine();
            else writeChar(inpt);
        }
    }
    return 0;
}