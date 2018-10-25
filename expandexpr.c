#include <stdio.h>
#include <stdlib.h>
#define MAX_TERMS 50
#define MAX_VARIABLES 10
#define MAX_POLY 10
#define MAX_EXPANSION 200

struct term {
    int coefficient;
    char variables[MAX_VARIABLES+1]; //one extra index for sentinel
    int powers[MAX_VARIABLES+1]; //when last index gets used it means there are too many variables and program should exit with error
};

struct poly {
    struct term *start;
    int length;
    int multiplicity;
};

struct term ONE = {.coefficient = 1, .variables[0] = '\0'};

int likeTerms(struct term *a, struct term *b) {
    for (int i=0; i<MAX_VARIABLES; i++) {
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
    for (int i=0; a->variables[i]!='\0'; i++) {
        int found = 0;
        for (int j=0; b->variables[j]!='\0'; j++) {
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

int scanPoly(struct term *terms, char *inputStr, int maxTerms) {
    struct term *current = terms;
    int numTerms = 0;
    //0 = find coefficient
    //1 = find power of variable
    int phase = 0;
    int numVariables = 0;
    int sign = 1;
    current->coefficient = 0;
    
    //check for leading negative sign
    if (*inputStr=='-') {
        sign = -1;
        inputStr++;
    }

    for (char *itr = inputStr; *itr != '\0'; itr++ ) {
        int numVal = *itr - '0';

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
            for (int j=0; current->variables[j]!='\0'; j++) {
                if (current->powers[j] == 0) {
                    current->powers[j] = 1;
                }
            }
            current->coefficient *= sign; //set sign
            current->variables[numVariables] = '\0'; //add sentinel
            if (*itr == '+') { sign = 1;  }
            else             { sign = -1; }
            //add new term to list or combine if like term exists
            int foundLikeTerm = 0;
            for (int i=0; i<numTerms; i++) {
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
    for (int j=0; current->variables[j]!='\0'; j++) {
        if (current->powers[j] == 0) {
            current->powers[j] = 1;
        }
    }
    current->coefficient *= sign; //set sign
    current->variables[numVariables] = '\0'; //add sentinel
    //add new term to list or combine if like term exists
    int foundLikeTerm = 0;
    for (int i=0; i<numTerms; i++) {
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
    int numVariables;
    for (numVariables=0; a->variables[numVariables]!='\0'; numVariables++) {
        newTerm.variables[numVariables] = a->variables[numVariables];
        newTerm.powers[numVariables] = a->powers[numVariables];
    }
    newTerm.variables[numVariables] = '\0';

    newTerm.coefficient = a->coefficient * b->coefficient; //multiply coefficients

    for (int j=0; b->variables[j]!='\0'; j++) {
        int found = 0;
        for (int i=0; newTerm.variables[i]!='\0'; i++) { //find if a has the same variable
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

void expandExpr(struct poly *polynomials, int numPoly, struct term *expandedTerms, int *numExpandedTerms, int polyIndex, int multiplicityCount, struct term *mult) {
    //detecting if reached last polynomial
    if (polyIndex == numPoly) {
        struct term *newTerm = expandedTerms + *numExpandedTerms;
        newTerm->coefficient = mult->coefficient;
        //find number of variables of term a and copies variables and powers to new term
        int numVariables;
        for (numVariables=0; mult->variables[numVariables]!='\0'; numVariables++) {
            newTerm->variables[numVariables] = mult->variables[numVariables];
            newTerm->powers[numVariables] = mult->powers[numVariables];
        }
        newTerm->variables[numVariables] = '\0';
        
        //add new term to list or combine if like term exists
        int foundLikeTerm = 0;
        for (int i=0; i<*numExpandedTerms; i++) {
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
    for (int i=0; i<currentPoly->length; i++) {
        struct term newMult = multiplyTerms(mult, currentPoly->start + i);
        expandExpr(polynomials, numPoly, expandedTerms, numExpandedTerms, polyIndex, multiplicityCount, &newMult);
    }
}

int main() {
    char *inputStr1 = "a+b";
    char *inputStr2 = "a-b";
    char *inputStr3 = "1";
    struct term terms[MAX_TERMS];
    struct poly polynomials[MAX_POLY];
    int numTerms = 0;
    int numPoly = 0;
    
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
    int numExpandedTerms = 0;
    expandExpr(polynomials, numPoly, expandedTerms, &numExpandedTerms, 0, 0, &ONE);

    /*struct term testTerm;
    testTerm.coefficient = 2;
    testTerm.variables[0] = 'a';
    testTerm.variables[1] = 'b';
    testTerm.variables[2] = '\0';
    testTerm.powers[0] = 1;
    testTerm.powers[1] = 2;

    multiplyTerms(&terms[0], &testTerm);*/

    for (int i=0; i<numTerms; i++) {
        if (terms[i].coefficient == -1) {
            printf("-");
        }
        else if (terms[i].coefficient != 1) {
            printf("%d", terms[i].coefficient);
        }
        for (int j=0; terms[i].variables[j] != '\0'; j++) {
            printf("%c", terms[i].variables[j]);
            if (terms[i].powers[j] != 1) {
                printf("%d", terms[i].powers[j]);
            }
        }
        printf("\n");
    }
    printf("\n");

    for (int i=0; i<numExpandedTerms; i++) {
        if (expandedTerms[i].coefficient == 0) {
            continue;
        }
        else if (expandedTerms[i].coefficient == -1) {
            printf("-");
        }
        else if (expandedTerms[i].coefficient != 1) {
            printf("%d", expandedTerms[i].coefficient);
        }
        for (int j=0; expandedTerms[i].variables[j] != '\0'; j++) {
            printf("%c", expandedTerms[i].variables[j]);
            if (expandedTerms[i].powers[j] != 1) {
                printf("%d", expandedTerms[i].powers[j]);
            }
        }
        printf("\n");
    }
}
