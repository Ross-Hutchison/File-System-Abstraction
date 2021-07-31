#include "InputOutput.h"

extern state_t state;

/*
    Function that is called as part of clearLine to 
    reset the state related to the given line line

    If the editor has started and a valid line has been given then 
    sets the length of that line to 0 meaning that it will be treated 
    as a blank line
*/
int state_clearLine(LINE_MAX toClear) {
    if(!state.started) return INV;
    else if(toClear < 0 || toClear > state.editor->maxLines) return ERR;
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
    Function for re-rendering a specific line of the output
*/
void rerenderLine(LINE_MAX line) {
    cursorTo(0, line);
    write(STDOUT_FILENO, "\x1b[2K", 4);
    LINE_LEN length = state.editor->lines[line].len;
    write(STDOUT_FILENO, state.editor->lines[line].text, length);
    state.cursorX += length;
    cursorTo(state.editor->currentChar, state.editor->currentLine);
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
            state.cursorX = current.len;
        }
        else if(i >= state.editor->inUse) {
            write(STDOUT_FILENO, "~", 1);
            state.cursorX = 1;
        }
    }
    cursorTo(state.editor->currentChar, state.editor->currentLine);
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
    LINE_MAX using = state.editor->inUse;
    LINE_MAX available = state.editor->maxLines - using;
    if(available < 1) return ERR;
    else {
        LINE_MAX curLine = state.editor->currentLine;
        LINE_MAX lookingAt = curLine + 1;
        LINE_MAX last = using - 1;  // - 1 to get index
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
        LINE_LEN curChar = state.editor->currentChar;
        LINE_MAX curLine = state.editor->currentLine;
        state.editor->lines[curLine].text[curChar] = inpt;
        if(state.editor->currentChar >= state.editor->lines[curLine].len) state.editor->lines[curLine].len++; //if not overwriting a char increase length
        state.editor->currentChar++;
        state.cursorX++;
    }
    if(state.editor->currentChar >= state.editor->lineLength) {
        if(state.editor->currentLine != state.editor->maxLines - 1) {
            state.editor->currentLine++;
            state.editor->currentChar = 0;
            cursorTo(0, state.editor->currentLine);
            if(state.editor->currentLine > state.editor->inUse - 1) {
                state.editor->inUse++;
                write(STDOUT_FILENO, " ", 1);
                state.cursorX++;
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
    LINE_LEN oldCurChar = state.editor->currentChar;
    LINE_MAX oldCurLine = state.editor->currentLine;

    if(addLine() == SUC) {
        LINE_MAX curLine = state.editor->currentLine;
        LINE_LEN moved = 0;
        LINE_LEN prevLen = state.editor->lines[oldCurLine].len;
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
    Function for removing the currently looked at char
*/
char removeChar() {
    LINE_MAX curLine = state.editor->currentLine;
    LINE_LEN curChar = state.editor->currentChar;
    LINE_LEN length = state.editor->lines[curLine].len;

    if(curChar <= 0 || curChar > length) handleFatalError("removeChar called with invalid currentChar", INV);
    else {
        if(curChar == length) previousChar();
        else {  //removing a char in middle of liner
            curChar = --state.editor->currentChar;
            for(int i = curChar; i < length - 1; i++) {
                state.editor->lines[curLine].text[i] = state.editor->lines[curLine].text[i + 1];
            }
        }
    }
    state.editor->lines[curLine].len--;  //removing the last character
    return SUC;
}

/*
    Function for handling the removal of an empty line or joining of two

    line = line curently having parts shifted TO
    c = point on line that new char is being inserted at
    c2 = character from next line being moved
*/
char removeOrJoinLine() {
    LINE_LEN curChar = state.editor->currentChar;
    LINE_MAX curLine = state.editor->currentLine;
    LINE_LEN srcLen = state.editor->lines[curLine].len;
    if(curChar != 0 || curLine < 1) return ERR;
    
    LINE_MAX line = curLine - 1;
    LINE_LEN dst = state.editor->lines[line].len;
    LINE_LEN src = 0;
    LINE_LEN switched = 0;
    
    for(; dst < state.editor->lineLength; dst++) {
        if(src == srcLen) break;
        state.editor->lines[line].text[dst] = state.editor->lines[curLine].text[src];
        switched++;
        src++;
    }
    state.editor->lines[line].len += switched;
    state.editor->lines[curLine].len -= switched;

    if(state.editor->lines[curLine].len == 0) {
        state.editor->inUse--;
        for(int i = curLine; i < state.editor->maxLines - 1; i++) {
            strcpy(state.editor->lines[i].text, state.editor->lines[i + 1].text);
            state.editor->lines[i].len= state.editor->lines[i + 1].len;
        }
    }
    else {
        for(int c2 = src ; c2 < srcLen; c2++) {
            state.editor->lines[curLine].text[c2 - switched] = state.editor->lines[curLine].text[c2];
        }
    }
    return SUC;
}

/*
    Function that handles the delete operation either removing a char 
    removing a line, or taking everything that fits from one line and putting it on the line above
*/
char handleDelete() {
    LINE_LEN curChar = state.editor->currentChar;
    LINE_MAX curLine = state.editor->currentLine;

    if(curChar < 0 || curChar > state.editor->lines[curLine].len) return ERR;
    else if(curChar == 0) { //removing a line or joining two together
        removeOrJoinLine();
        rerenderOutput();
        LINE_LEN endOfPrev = state.editor->lines[curLine - 1].len - 1;
        cursorTo(endOfPrev, curLine - 1);
        state.editor->currentChar = endOfPrev;
        state.editor->currentLine = curLine - 1;
    }
    else {  //removing a charachter
        removeChar();
        rerenderLine(curLine);
    }
    return SUC;
}