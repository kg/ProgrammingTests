#include "common.h"
#include "string.h"
#include <exception>

// Efficiently reverse the order of words within a null-terminated string, using as little additional storage space as possible.
void reverse_words (char * sentence) {
    const size_t sentenceLength = strlen(sentence);
    size_t firstWordLength;

    // First we reverse the entire string so that the words are in the opposite of their original order.
    // While doing this we also identify the location of the break between each word, and we replace
    //  the space separating the words with a byte representing the length of the next word.
    {
        const size_t numSwaps = sentenceLength / 2;
        unsigned currentWordStart = 0;
        size_t currentWordLength = 0;

        for (unsigned i = 0; i < sentenceLength; i++) {
            char ch = sentence[i];

            if (i < numSwaps) {
                const unsigned j = (sentenceLength - 1) - i;
                sentence[i] = sentence[j];
                sentence[j] = ch;
                ch = sentence[i];
            }

            if (ch == ' ') {
                if (currentWordLength > 0xFF)
                    throw std::exception("Found a word with more than 256 characters in it.");
                
                // We can't store the length of the first word inside the string, so we store it in a local.
                if (currentWordStart == 0)
                    firstWordLength = currentWordLength;
                else
                    sentence[currentWordStart] = (char)currentWordLength;

                currentWordStart = i;
                currentWordLength = 0;
            } else {
                currentWordLength++;
            }
        }

        // The sentence probably doesn't have a trailing space, so we need to write out the length of the last word.
        if (currentWordStart == 0)
            firstWordLength = currentWordLength;
        else
            sentence[currentWordStart] = (char)currentWordLength;
    }

    // Now we do a second pass through the sentence, reversing each individual word. The length information that
    //  we computed during the first pass will simplify this.
    {
        unsigned i = 0;
        size_t currentWordLength = firstWordLength;

        while ((i + currentWordLength) <= sentenceLength) {
            reverseCharactersInPlace(sentence + i, currentWordLength);

            i += currentWordLength;
            if (i >= sentenceLength)
                break;

            currentWordLength = (size_t)sentence[i];
            sentence[i] = ' ';
            i++;
        }
    }
}

// Reverse a sequence of characters in-place
void reverseCharactersInPlace (char * chars, size_t count) {
    const size_t steps = count / 2;

    for (unsigned i = 0; i < steps; i++) {
        const unsigned j = (count - 1) - i;
        char temp = chars[i];
        chars[i] = chars[j];
        chars[j] = temp;
    }
}