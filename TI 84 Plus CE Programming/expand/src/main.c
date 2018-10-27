#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <keypadc.h>

#define MAX_TERMS 50
#define MAX_STRING 50
#define MAX_VARIABLES 10
#define MAX_POLY 10
#define MAX_EXPANSION 200
#define MAX_RESULT 2000


struct term {
    int coefficient;
    char variables[MAX_VARIABLES+1]; //one extra index for sentinel
    int powers[MAX_VARIABLES+1]; //when last index gets used it means there are too many variables and program should exit with error
};

struct poly {
    struct term *start;
    uint8_t length;
    uint8_t multiplicity;
};

const char numChars[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,  '-',  0,   0,   0,   0,   0,  '-', '3', '6',
   '9',  0,   0,   0,   0,   0,  '2', '5', '8',  0,
    0,   0,   0,  '0', '1', '4', '7',  0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0
};
const char *alphaChars = "\0\0\0\0\0\0\0\0\0\0\0WRMH\0\0\0\0VQLG\0\0\0ZUPKFC\0\0YTOJEB\0\0XSNIDA\0\0\0\0\0\0\0\0";
uint8_t alpha = 0;

void delChar(char *ptr) {
    while (*ptr) {
        *ptr = *(ptr++);
    }
}

void moveCursorRight() {
    unsigned int row;
    unsigned int column;
    os_GetCursorPos(&row, &column);
    os_SetCursorPos(row, column+1);
}

void moveCursorLeft() {
    unsigned int row;
    unsigned int column;
    os_GetCursorPos(&row, &column);
    os_SetCursorPos(row, column-1);
}

void drawLine(uint8_t line, char *polyString, char *multString, uint8_t cursorPos, uint8_t *scroll) {
    int cursorDispPos;
    char *buffer;
    if (cursorPos>=0) {
        cursorDispPos = cursorPos - *scroll + 1;
        if (cursorDispPos==23) {
            *scroll = cursorPos - 21;
            cursorDispPos = cursorPos - *scroll + 1;
        }
        else if (cursorDispPos<1) {
            *scroll = cursorPos;
            cursorDispPos = cursorPos - *scroll + 1;
        }
    }
    else {
        if (cursorPos==-2) {
            cursorDispPos=24;
        }
        else {
            cursorDispPos=25;
        }
    }
    buffer = (char *)malloc(27*sizeof(char));
    memset(buffer, ' ', 26);
    buffer[26] = '\0';
    buffer[0] = '(';
    buffer[23] = ')';
    strncpy(buffer+1, polyString+*scroll, 22);
    strncpy(buffer+24, multString, 2);
    os_SetCursorPos(line, 0);
    os_PutStrFull(buffer);
    os_SetCursorPos(line, cursorDispPos);
}

void dispString(char *string) {
    char *buffer;
    int offset;
    uint8_t atBottom;
    int i;
    
    uint8_t key;
    uint8_t lastKey;
    uint8_t changed;

    os_DisableCursor();
    
    buffer = (char *)malloc(261*sizeof(char));
    buffer[260]='\0';
    offset = 0;
    atBottom = 0;
    for (i=0; i<260; i++) {
        char c = string[26*offset+i];
        buffer[i] = c;
        if (!c) {
            atBottom = 1;
            break;
        }
    }
    os_SetCursorPos(0,0);
    os_PutStrFull(buffer);
    
    key = 0;
    lastKey = 0;
    while (1) {
        lastKey = key;
        key = os_GetCSC();
        if (!key || lastKey==key) {continue;} //if no key is pressed or key is not changed, continue
        
        changed = 0;
        if (key == sk_Up) { //switch between poly and multiplicity input
            if (offset>0) {
                offset--;
                atBottom=0;
                changed=1;
            }
        }
        else if (key == sk_Down) {
            if (!atBottom) {
                offset++;
                changed=1;
            }
        }

        if (changed) {
            for (i=0; i<260; i++) {
                char c = string[26*offset+i];
                buffer[i] = c;
                if (!c) {
                    atBottom = 1;
                    break;
                }
            }
            os_SetCursorPos(0,0);
            os_PutStrFull(buffer);
        }
    }
}

uint8_t likeTerms(struct term *a, struct term *b) {
    uint8_t i;
    uint8_t j;
    for (i=0; i<MAX_VARIABLES; i++) {
        //check if number of terms is same
        if (a->variables[i]=='\0') {
            if (b->variables[i]=='\0') {break;}
            else {return 0;}
        }
        else if (b->variables[i]=='\0') {
            break;
        }
    }

    //loop through variables and find if they match
    for (i=0; a->variables[i]!='\0'; i++) {
        uint8_t found = 0;
        for (j=0; b->variables[j]!='\0'; j++) {
            if (a->variables[i] == b->variables[j]) {
                if (!(a->powers[i] == b->powers[j])) { //different exponents
                    return 0;
                }
                found = 1;
                break;
            }
        }
        if (!found) {
            return 0;
        }
    }
    return 1;
}

uint8_t scanPoly(struct term *terms, char *inputStr, uint8_t maxTerms) {
    struct term *current = terms;
    uint8_t numTerms = 0;
    //0 = find coefficient
    //1 = find power of variable
    uint8_t phase = 0;
    uint8_t numVariables = 0;
    int8_t sign = 1;
    char *itr;
    uint8_t i;
    uint8_t j;
    current->coefficient = 0;
    
    //check for leading negative sign
    if (*inputStr=='-') {
        sign = -1;
        inputStr++;
    }

    for (itr = inputStr; *itr != '\0'; itr++ ) {
        uint8_t numVal = *itr - '0';

        if (numVal>=0 && numVal <=9) { //is digit
            if (phase==0) {
                current->coefficient *= 10;
                current->coefficient += numVal;
            }
            else {
                current->powers[numVariables-1] *= 10;
                current->powers[numVariables-1] += numVal;
            }
        }
        else if (*itr != '+' && *itr != '-') { //is letter
            current->variables[numVariables] = *itr;
            current->powers[numVariables] = 0;
            numVariables++;
            if (numVariables>MAX_VARIABLES) {
                //printf("term has too many variables\n");
                exit(1);
            }
            if (phase==0) {phase=1;}
        }
        else { //is plus sign or minus sign
            uint8_t foundLikeTerm;
            //change coefficients and powers of 0 to 1
            if (current->coefficient == 0) {
                current->coefficient = 1;
            }
            for (j=0; current->variables[j]!='\0'; j++) {
                if (current->powers[j] == 0) {
                    current->powers[j] = 1;
                }
            }
            current->coefficient *= sign; //set sign
            current->variables[numVariables] = '\0'; //add sentinel
            if (*itr == '+') { sign = 1;  }
            else             { sign = -1; }
            //add new term to list or combine if like term exists
            foundLikeTerm = 0;
            for (i=0; i<numTerms; i++) {
                if (likeTerms(&terms[i],current)) {
                    terms[i].coefficient += current->coefficient;
                    foundLikeTerm = 1;
                    break;
                }
            }
            if (!foundLikeTerm) {
                current++;
                numTerms++;
                if (numTerms == maxTerms) { //prevent term list from overflowing
                    //printf("too many terms\n");
                    exit(1);
                }
            }
            numVariables = 0;
            phase = 0;
            terms[numTerms].coefficient = 0;
        }
    }

    //change coefficients and powers of 0 to 1
    if (current->coefficient == 0) {
        current->coefficient = 1;
    }
    for (uint8_t j=0; current->variables[j]!='\0'; j++) {
        if (current->powers[j] == 0) {
            current->powers[j] = 1;
        }
    }
    current->coefficient *= sign; //set sign
    current->variables[numVariables] = '\0'; //add sentinel
    //add new term to list or combine if like term exists
    uint8_t foundLikeTerm = 0;
    for (uint8_t i=0; i<numTerms; i++) {
        if (likeTerms(&terms[i],current)) {
            terms[i].coefficient += current->coefficient;
            foundLikeTerm = 1;
            break;
        }
    }
    if (!foundLikeTerm) {
        numTerms++;
    }

    return numTerms;
}

struct term multiplyTerms(struct term *a, struct term *b) { //non destructive
    struct term newTerm;
    newTerm.coefficient = a->coefficient;
    //find number of variables of term a and copies variables and powers to new term
    uint8_t numVariables;
    for (numVariables=0; a->variables[numVariables]!='\0'; numVariables++) {
        newTerm.variables[numVariables] = a->variables[numVariables];
        newTerm.powers[numVariables] = a->powers[numVariables];
    }
    newTerm.variables[numVariables] = '\0';

    newTerm.coefficient = a->coefficient * b->coefficient; //multiply coefficients

    for (uint8_t j=0; b->variables[j]!='\0'; j++) {
        uint8_t found = 0;
        for (uint8_t i=0; newTerm.variables[i]!='\0'; i++) { //find if a has the same variable
            if (newTerm.variables[i] == b->variables[j]) {
                newTerm.powers[i] += b->powers[j];
                found = 1;
                break;
            }
        }
        if (!found) { //add new variable to a;
            if (numVariables>=MAX_VARIABLES) {
                //printf("Too many variables");
                exit(1);
            }
            newTerm.variables[numVariables] = b->variables[j];
            newTerm.powers[numVariables] = b->powers[j];
            numVariables++;
        }
    }
    
    //add sentinel
    newTerm.variables[numVariables] = '\0';
    
    return newTerm;
}

void expandExpr(struct poly *polynomials, uint8_t numPoly, struct term *expandedTerms, uint8_t *numExpandedTerms, uint8_t polyIndex, uint8_t multiplicityCount, struct term *mult) {
    //detecting if reached last polynomial
    if (polyIndex == numPoly) {
        if (*numExpandedTerms > MAX_EXPANSION) { //prevent term list from overflowing
            //printf("too many terms\n");
            exit(1);
        }

        struct term *newTerm = expandedTerms + *numExpandedTerms;
        newTerm->coefficient = mult->coefficient;
        //find number of variables of term a and copy variables and powers to new term
        uint8_t numVariables;
        for (numVariables=0; mult->variables[numVariables]!='\0'; numVariables++) {
            newTerm->variables[numVariables] = mult->variables[numVariables];
            newTerm->powers[numVariables] = mult->powers[numVariables];
        }
        newTerm->variables[numVariables] = '\0';
        
        //add new term to list or combine if like term exists
        uint8_t foundLikeTerm = 0;
        for (uint8_t i=0; i<*numExpandedTerms; i++) {
            if (likeTerms(&expandedTerms[i],newTerm)) {
                expandedTerms[i].coefficient += newTerm->coefficient;
                foundLikeTerm = 1;
                break;
            }
        }
        if (!foundLikeTerm) {
            (*numExpandedTerms)++;
        }
        return;
    }

    //prepare for next polynomial
    multiplicityCount++;
    struct poly *currentPoly = polynomials + polyIndex;
    if (multiplicityCount == currentPoly->multiplicity) { //move to next polynomial if multiplicity is satisfied
        polyIndex++;
        multiplicityCount=0;
    }

    //recursively multiply out polynomial
    for (uint8_t i=0; i<currentPoly->length; i++) {
        struct term newMult = multiplyTerms(mult, currentPoly->start + i);
        expandExpr(polynomials, numPoly, expandedTerms, numExpandedTerms, polyIndex, multiplicityCount, &newMult);
    }
}

int main() {
    os_ClrHome();
    os_EnableCursor();

    char mults[MAX_POLY][3]; //max 2 digit multiplicity, one extra for sentinel
    char buffer[MAX_POLY][MAX_STRING+1]; //buffer for string input
    for (int i=0; i<MAX_POLY; i++) {
        mults[i][0] = '\0';
        buffer[i][0] = '\0';
    }

    uint8_t key = 0;
    uint8_t lastKey = 0;
    int8_t cursorPos = 0;
    uint8_t scroll = 0;
    uint8_t lineNum = 0;
    char c;
    drawLine(lineNum, buffer[lineNum], mults[lineNum], cursorPos, &scroll);
    while (1) {
        lastKey = key;
        key = os_GetCSC();
        if (!key || lastKey==key) {continue;} //if no key is pressed or key is not changed, continue
        
        c='\0'; //reset character, and if it isn't set no character is entered (like if the clear button is pressed)
        if (key == sk_Alpha && cursorPos>=0) { //if alpha key is pressed and you aren't entering a multiplicity
            alpha = 1-alpha;
        }
        else if (key == sk_Enter) { //switch between poly and multiplicity input
            if (cursorPos>=0) {
                cursorPos = -2; //max 2 digit multiplicity
            }
            else {
                cursorPos = 0;
                if (lineNum<MAX_POLY-1) {
                    lineNum++;
                }
            }
            alpha = 0;
        }
        else if (key == sk_Clear) {
            if (cursorPos>=0) {
                cursorPos=0;
                buffer[lineNum][0] = "\0";
            }
            else {
                cursorPos = -2;
                mults[lineNum][0] = "\0";
            }
            drawLine(lineNum, buffer[lineNum], mults[lineNum], cursorPos, &scroll);
        }
        else if (key == sk_Del) {
            if (cursorPos>=0) {
                delChar(&buffer[lineNum][cursorPos]); //delChar does nothing if value at the pointer is '\0'
            }
            else {
                delChar(&mults[lineNum][cursorPos+2]);
            }
            drawLine(lineNum, buffer[lineNum], mults[lineNum], cursorPos, &scroll);
        }
        else if (key == sk_Graph) {
            break;
        }
        else if (key == sk_Power) {
            exit(1);
        }
        //arrow keys
        else if (key == sk_Up) {
            if (lineNum>0) {
                cursorPos = 0;
                drawLine(lineNum, buffer[lineNum], mults[lineNum], cursorPos, &scroll);
                lineNum--;
                os_SetCursorPos(lineNum, 1);
                alpha = 0;
            }
        }
        else if (key == sk_Down) {
            if (lineNum<MAX_POLY-1) {
                cursorPos = 0;
                drawLine(lineNum, buffer[lineNum], mults[lineNum], cursorPos, &scroll);
                lineNum++;
                os_SetCursorPos(lineNum, 1);
                alpha = 0;
            }
        }
        else if (key == sk_Left) {
            if (cursorPos > 0 || cursorPos == -1) {
                cursorPos--;
                moveCursorLeft();
                alpha = 0;
            }
        }
        else if (key == sk_Right) {
            if (buffer[lineNum][cursorPos] && cursorPos < MAX_STRING && cursorPos != -1) {
                cursorPos++;
                moveCursorRight();
                alpha = 0;
            }
        }
        else if (alpha) {
            c = alphaChars[key];
        }
        else {
            c = numChars[key];
        }
        
        if (c) { //if there's a character
            if (cursorPos<MAX_STRING && cursorPos>=0) { //if not at max length and in poly input
                if (!buffer[lineNum][cursorPos]) { //if at end of string, set next index to be sentinel
                    buffer[lineNum][cursorPos+1] = '\0';
                }
                buffer[lineNum][cursorPos] = c;
                cursorPos++;
                alpha = 0;
            }
            else if (cursorPos<0 && cursorPos != -1) { //if not at max length and in multiplicity input
                if (!mults[lineNum][cursorPos+2]) { //if at end of string, set next index to be sentinel
                    mults[lineNum][cursorPos+3] = 1;
                }
                mults[lineNum][cursorPos] = c;
                cursorPos++;
            }
            drawLine(lineNum, buffer[lineNum], mults[lineNum], cursorPos, &scroll);
        }
    }
    
    struct term ONE;
    ONE.coefficient = 1;
    ONE.variables[0] = '\0'

    struct term terms[MAX_TERMS];
    struct poly polynomials[MAX_POLY];
    uint8_t numTerms = 0;
    uint8_t numPoly = 0;
   
    for (int i=0; i<MAX_POLY; i++) {
        polynomials[numPoly].start = terms+numTerms;
        numTerms += ( polynomials[numPoly].length = scanPoly(terms+numTerms, buffer[i], MAX_TERMS-numTerms) );
        if (mults[i][0]=='\0') {
            polynomials[numPoly].multiplicity = 1;
        }
        else if (mults[i][1]=='\0') {
            polynomials[numPoly].multiplicity = mults[i][0]-'0';
        }
        else {
            polynomials[numPoly].multiplicity = (mults[i][0]-'0')*10 + (mults[i][1]-'0');
        }
        numPoly++;
    }

    struct term expandedTerms[MAX_EXPANSION];
    uint8_t numExpandedTerms = 0;
    expandExpr(polynomials, numPoly, expandedTerms, &numExpandedTerms, 0, 0, &ONE);
    
    char result[MAX_RESULT];
    int scanPos = 0;
    for (uint8_t i=0; i<numExpandedTerms; i++) {
        if (expandedTerms[i].coefficient == 0) {
            continue;
        }
        else if (expandedTerms[i].coefficient == -1) {
            scanPos += sprintf(result+scanPos,"-");
        }
        else if (expandedTerms[i].coefficient != 1) {
            scanPos += sprintf(result+scanPos, "%d", expandedTerms[i].coefficient);
        }
        for (uint8_t j=0; expandedTerms[i].variables[j] != '\0'; j++) {
            scanPos += sprintf(result+scanPos, "%c", expandedTerms[i].variables[j]);
            if (expandedTerms[i].powers[j] != 1) {
                scanPos += sprintf(result+scanPos, "%d", expandedTerms[i].powers[j]);
            }
        }
        if (scanPos > MAX_RESULT-50) { //approaching buffer limit
            //printf("result too long\n");
            exit(1);
        }
        //printf("\n");
    }
}
