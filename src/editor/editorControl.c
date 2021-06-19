#include "editor.h"

int main() {
    char res;
    editor_t *editor = new_editor(10,10);
    startup(editor);

    while(TRUE) {
        char inpt = '\0';
        res = read(STDIN_FILENO, &inpt, 1);
        if(res > 0) {
            if(inpt == '.') {
                clearWhole(editor);
                break;
            }
            else if(inpt == ',') clearLine(editor);
            else writeChar(editor, inpt);
        }
    }
    return 0;
}