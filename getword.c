/*
 * Brenda Tran
 * Program 2
 * Professor Carroll
 * CS570
 * Due: Feb 28th
 *
 * getword.c takes in a reference to an array by pointer and parses the array based on special cases. Space, newline, # sign, EOF, metacharacters and backslash are the special cases and
 * are defined below. A parameter was also set for how many characters that could be taken from user input and parsed.
 */
#include "getword.h"

int getword(char *w) {
    int input, charCount = 0, backslash = 0;

    while (1) {
        input = getchar();
        // PARAMETER - after the 254 character was been read in the last character will be put back in the array by ungetc
        // a null terminator will be added to end the array and the amoutn of characters will be returned
        if (charCount == 254) {
            ungetc(input, stdin);
            w[charCount] = '\0';
            return charCount;
        }
        //BACKSLASH - a flag that is triggered if the input is equal to a backslash followed by retrieving the next character from input
        if (input == '\\') {
            backslash = 1;
            input = getchar();
        }

        if (input != ' ') {
            //METACHARACTERS INCLUDING # - any of the 5 characters will essentially return -1 if found without a backslash and the character by itself in a character array
            if (input == '<' || input == '>' || input == '|' || input == '&' || input == '#') {
                //if the backslash was flagged before a metacharacter was found, the mateacharacter will be treated as a regular character and added to the character count
                if (backslash == 1) {
                    w[charCount++] = input;
                    backslash = 0;

                    //if the # was found and there are no characters found before the # then the # will be be returned by itself in a character array with -1 as the count
                    //if there was not any characters then the # is treated as a regular character and is accounted for in the count and added in the character array
                } else if (input == '#') {
                    if (charCount == 0) {
                        w[charCount++] = input;
                        w[charCount] = '\0';
                        return -1;
                    } else {
                        w[charCount++] = input;
                    }
                    //if a pipe is found and there are no characters before it add the pipe into the character array
                    //if there are characters before it, the characters are put back by ungetc and the character count is returned
                    //the next input is taken from user input to see if there is an &
                    // |& is a special case were it should return both metacharacters and -2 rather than return both individually
                    // the  & must follow directly after the |, and no backslash is found between or before the metacharacters for this to occur
                    // if there is a backslash before both than the first metacharacter is treated as a regular character and the & will be treated as a regular metacharacter

                } else if (input == '|') {
                    if (charCount == 0) {
                        w[charCount++] = input;
                    } else {
                        ungetc(input, stdin);
                        return charCount;
                    }
                    input = getchar();
                    if (input == '&') {
                        w[charCount++] = input;
                        w[charCount] = '\0';
                        return -2;
                    } else {
                        ungetc(input,stdin);
                        w[charCount] = '\0';
                        return -1;
                    }
                    //returns characters if metacharacters are found in between words
                    //the metacharacters are return by itself with -1
                } else if (charCount != 0) {
                    ungetc(input, stdin);
                    w[charCount] = '\0';
                    return charCount;
                    //if the metacharacter is found at the beginning of the word than the metacharacter is returned by itself with -1
                } else {
                    w[charCount++] = input;
                    w[charCount] = '\0';
                    return -1;
                }

                //NEWLINE: If the input is a new line and the charCount is 0, no characters have been found before the new line,
                //then the array slot is null terminated, returning an empty character array and a -10 is returned.
                // if the charCount is not 0, meaning there were characters that were found prior to the new line, then the last character
                //is put back into the array followed by null to print the word into the array and return the amount of characters in the printed word

            } else if (input == '\n') {
                if (charCount == 0) {
                    w[charCount] = '\0';
                    return -10;
                }
                ungetc(input, stdin);
                w[charCount] = '\0';
                return charCount;

                //EOF: If EOF is found and charCount is 0, no characters were found before the EOF signal was found, return
                //an empty character array by null terminating the poisiton in the array and return 0
                // if there were characters found prior to EOF, the last character will be put back followed by a null in order
                //for the word to be print and the character count ot be returned
            } else if (input == EOF) {
                if (charCount == 0) {
                    w[charCount] = '\0';
                    return 0;
                }
                ungetc(input, stdin);
                w[charCount] = '\0';
                return charCount;

            } else {
                w[charCount++] = input;
            }
            //SPACES: This deals with spaces in between words. If there is a space and the charcount is not 0, than there is a word that has been taken in thus
            // the space is null terminates and the charcount is returned for the word
            //if there is a backslash before the space then the space is treated as a regular character and added into the word and the count is incremented

        }
        if (input == ' ') {
            if (backslash == 0 && charCount != 0) {
                w[charCount++] = '\0';
                return charCount;

            }
            if(backslash == 1)
                w[charCount++] = input;
                backslash = 0;
        }



    }

}
