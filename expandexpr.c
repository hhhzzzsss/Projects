
struct term {
    int coefficient;
    char variables[5]; //max 4 variables, one extra for sentinel
    int powers[5]; //when fifth index gets used it means there are too many variables and program should exit with error
};

int likeTerms(struct term *a, struct term *b) {
    for (int i=0; i<5; i++) {
        //check if number of terms is same
        if (a[i]=='\0') {
            if (b[i]=='\0') {break;}
            else {return 0;}
        }
        else if (b[i]=='\0') {
        }
    }
    for (char *itr_a = a; &itr_a != '\0', itr_a++) {
        int found = 0;

    }
}

int main() {
    char *inputStr = "2x^2y+3x^2y";
    struct term terms[100];
    int numTerms = 0;
    
    struct term *current = terms;
    //0 = find coefficient
    //1 = find power of variable
    int phase = 0;
    int numVariables;
    current->coefficient = 0;

    for (char *itr = inputStr, &itr != '\0'; itr++ ) {
        int numVal = &itr - '0';

        if (numVal>=0 && numVal <=9) {
            if (phase==0) {
                current->coefficient *= 10;
                current->coefficient += numVal;
            }
            else {
                current->powers[numVariables-1] *= 10;
                current->powers[numVariables-1] += numVal;
            }
        }
        else if (&itr != '+') {
            current->variables[numVariables] = &itr;
            current->power[numVariables] = 0;
            numVariables++;
            if (numVariables>4) {
                printf("term has too many variables\n");
                exit(1);
            }
            printf("new variable: %c\n", &itr);
            if (phase==0) {phase=1;}
        }
        else {
            current->variables[numVariables] = '\0';
            numVariables = 0;
            phase = 0;
        }
    }
}
