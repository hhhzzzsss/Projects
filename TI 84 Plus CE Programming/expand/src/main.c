#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <stdio.h>
#include <stdlib.h>

#include <keypadc.h>

#define MAX_TERMS 50
#define MAX_VARIABLES 10
#define MAX_POLY 10
#define MAX_EXPANSION 200
#define MAX_STRING 50

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

struct term ONE = {.coefficient = 1, .variables[0] = '\0'};
const char *numChars = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,  '-',  0,   0,   0,   0,   0,  '-', '3', '6',
   '9',  0,   0,   0,   0,   0,  '2', '5', '8',  0,
    0,   0,   0,  '0', '1', '4', '7',  0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0
};
const char *alphaChars = "\0\0\0\0\0\0\0\0\0\0\0WRMH\0\0\0\0VQLG\0\0\0ZUPKFC\0\0YTOJEB\0\0XSNIDA\0\0\0\0\0\0\0\0";
uint8_t alpha = 0;

uint8_t likeTerms(struct term *a, struct term *b) {
    for (uint8_t i=0; i<MAX_VARIABLES; i++) {
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
    for (uint8_t i=0; a->variables[i]!='\0'; i++) {
        uint8_t found = 0;
        for (uint8_t j=0; b->variables[j]!='\0'; j++) {
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
    current->coefficient = 0;
    
    //check for leading negative sign
    if (*inputStr=='-') {
        sign = -1;
        inputStr++;
    }

    for (char *itr = inputStr; *itr != '\0'; itr++ ) {
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
                printf("term has too many variables\n");
                exit(1);
            }
            if (phase==0) {phase=1;}
        }
        else { //is plus sign or minus sign
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
            if (*itr == '+') { sign = 1;  }
            else             { sign = -1; }
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
                current++;
                numTerms++;
                if (numTerms == maxTerms) { //prevent term list from overflowing
                    printf("too many terms\n");
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
                printf("Too many variables");
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
            printf("too many terms\n");
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
    uint8_t scanning = 1;
    uint8_t key = 0;
    uint8_t lastKey = 0;
    char mults[MAX_POLY][2]; //max 2 digit multiplicity
    memset(mults, 1, MAX_POLY*sizeof(char));
    char buffer[MAX_POLY][MAX_STRING+1];
    int8_t cursorPos = 0;
    uint8_t lineNum;
    char c;
    while (scanning) {
        lastKey = key;
        key = os_GetCSC();
        if (!key || lastKey==key) {continue;} //if no key is pressed or key is not changed, continue
        
        c='\0';
        if (key == sk_Alpha && cursorPos>=0) { //if alpha key is pressed and you aren't entering a multiplicity
            alpha = 1-alpha;
        }
        else if (key == sk_Enter) { //switch between poly and multiplicity input
            if (cursorPos>=0) {
                cursorPos = -2; //max 2 digit multiplicity
            }
            else {
                cursorPos = 0;
            }
            alpha = 0;
        }
        else if (key == sk_Clear) {
        }
        //arrow keys
        else if (key == sk_Up) {
            if (lineNum>0) {
                lineNum--;
                cursorPos = 0;
                alpha = 0;
            }
        }
        else if (key == sk_Down) {
            if (lineNum<MAX_POLY-1) {
                lineNum++;
                cursorPos = 0;
                alpha = 0;
            }
        }
        else if (key == sk_Left) {
            if (cursorPos > 0 || cursorPos == -1) {
                cursorPos--;
                alpha = 0;
            }
        }
        else if (key == sk_Right) {
            if (buffer[lineNum][cursorPos] && cursorPos < MAX_STRING && cursorPos != -1) {
                cursorPos++;
                alpha = 0;
            }
        }
        else if (alpha) {
            c = alphaChars[key];
        }
        else {
            c = numChars[key];
        }
        
        if (cursorPos<MAX_STRING && c) {
            if (!buffer[lineNum][cursorPos]) { //if at end of string, set next index to be sentinel
                buffer[lineNum][cursorPos+1] = '\0';
            }
            buffer[lineNum][cursorPos] = c;
            cursorPos++;
            alpha = 0;
        }
    }

    char *inputStr1 = "a+b";
    char *inputStr2 = "a-b";
    char *inputStr3 = "1";
    struct term terms[MAX_TERMS];
    struct poly polynomials[MAX_POLY];
    uint8_t numTerms = 0;
    uint8_t numPoly = 0;
    
    polynomials[numPoly].start = terms+numTerms;
    numTerms += ( polynomials[numPoly].length = scanPoly(terms+numTerms, inputStr1, MAX_TERMS-numTerms) );
    polynomials[numPoly].multiplicity = 1;
    numPoly++;

    polynomials[numPoly].start = terms+numTerms;
    numTerms += ( polynomials[numPoly].length = scanPoly(terms+numTerms, inputStr2, MAX_TERMS-numTerms) );
    polynomials[numPoly].multiplicity = 1;
    numPoly++;

    polynomials[numPoly].start = terms+numTerms;
    numTerms += ( polynomials[numPoly].length = scanPoly(terms+numTerms, inputStr3, MAX_TERMS-numTerms) );
    polynomials[numPoly].multiplicity = 1;
    numPoly++;

    struct term expandedTerms[MAX_EXPANSION];
    uint8_t numExpandedTerms = 0;
    expandExpr(polynomials, numPoly, expandedTerms, &numExpandedTerms, 0, 0, &ONE);

    /*struct term testTerm;
    testTerm.coefficient = 2;
    testTerm.variables[0] = 'a';
    testTerm.variables[1] = 'b';
    testTerm.variables[2] = '\0';
    testTerm.powers[0] = 1;
    testTerm.powers[1] = 2;

    multiplyTerms(&terms[0], &testTerm);*/

    for (uint8_t i=0; i<numTerms; i++) {
        if (terms[i].coefficient == -1) {
            printf("-");
        }
        else if (terms[i].coefficient != 1) {
            printf("%d", terms[i].coefficient);
        }
        for (uint8_t j=0; terms[i].variables[j] != '\0'; j++) {
            printf("%c", terms[i].variables[j]);
            if (terms[i].powers[j] != 1) {
                printf("%d", terms[i].powers[j]);
            }
        }
        printf("\n");
    }
    printf("\n");

    for (uint8_t i=0; i<numExpandedTerms; i++) {
        if (expandedTerms[i].coefficient == 0) {
            continue;
        }
        else if (expandedTerms[i].coefficient == -1) {
            printf("-");
        }
        else if (expandedTerms[i].coefficient != 1) {
            printf("%d", expandedTerms[i].coefficient);
        }
        for (uint8_t j=0; expandedTerms[i].variables[j] != '\0'; j++) {
            printf("%c", expandedTerms[i].variables[j]);
            if (expandedTerms[i].powers[j] != 1) {
                printf("%d", expandedTerms[i].powers[j]);
            }
        }
        printf("\n");
    }
}
