#include "editor.h"

int main() {
    char active = TRUE;
    setRaw();
    uint8_t count = 0;
    char res;
    while(active) {
        char inpt = '\0';
        res = read(STDIN_FILENO, &inpt, 1);
        if(res > 0) {
            char* back = (char*)calloc(3 + MAX_LINE_NUM_SIZE, sizeof(char));
            write(STDOUT_FILENO, &inpt, 1);
            count++;
            if(inpt == '.') {
                write(STDOUT_FILENO, "\x1b[1D", 4);
                write(STDOUT_FILENO, "\x1b[0K", 4);
                write(STDOUT_FILENO, "\n", 1);
                sprintf(back, "\x1b[%dD", count);
                write(STDOUT_FILENO, back, strlen(back));
                break;
            }
            if(inpt == ',') {
                sprintf(back, "\x1b[%dD", count);
                write(STDOUT_FILENO, back, strlen(back));
                write(STDOUT_FILENO, "\x1b[2K", 4);
                count = 0;
            }
            if(count == MAX_LINE_L) {
                write(STDOUT_FILENO, "\n", 1);
                sprintf(back, "\x1b[%dD", MAX_LINE_L);
                write(STDOUT_FILENO, back, strlen(back));
                count = 0;
            }
        }
    }
    return 0;
}