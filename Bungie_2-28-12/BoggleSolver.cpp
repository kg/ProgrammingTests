#include "common.h"
#include <direct.h>
#include <stdio.h>
#include <sys/stat.h>

namespace Boggle {
    Node::Node (bool validWord)
        : isValidWord(validWord) 
    {
        memset(children, NODE_NONE, sizeof(NodeIndex) * 26);
    }
    
    NodeIndex Dictionary::allocateNode (bool validWord) {
        NodeIndex result = nodes.size();
        nodes.push_back(Node(validWord));
        return result;
    }

    Dictionary::Dictionary (const char * dictionaryPath) {
        // Allocate node 0 to be the root
        nodes.reserve(16);
        nodes.push_back(Node(false));

        FILE * dictionaryFile;
        char * dictionaryBuffer;
        struct stat stat;

        dictionaryFile = fopen(dictionaryPath, "r");
        if (!dictionaryFile) {
            throw std::exception("Failed to open dictionary");
        }

        if (fstat(fileno(dictionaryFile), &stat)) {
            throw std::exception("Failed to get dictionary info");
        }

        try {
            dictionaryBuffer = new char[stat.st_size];
            fread(dictionaryBuffer, stat.st_size, 1, dictionaryFile);
            fclose(dictionaryFile);

            unsigned currentWordStart = 0;
            for (unsigned i = 0; i < stat.st_size; i++) {
                char ch = dictionaryBuffer[i];

                if ((ch == '\n') || (ch == '\r') || (ch == '\0')) {
                    size_t currentWordLength = i - currentWordStart;
                    if (currentWordLength)
                        addWord(dictionaryBuffer + currentWordStart, currentWordLength);

                    currentWordStart = i + 1;
                }
            }

            size_t currentWordLength = stat.st_size - currentWordStart;
            if ((currentWordLength + currentWordStart < stat.st_size) && (currentWordLength))
                addWord(dictionaryBuffer + currentWordStart, currentWordLength);
        } catch (...) {
            delete[] dictionaryBuffer;
            throw;
        }
    }

    NodeIndex Dictionary::addWord (const char * word, size_t wordLength) {
        // Start at the root
        NodeIndex currentIndex = 0;

        for (unsigned i = 0; i < wordLength; i++) {
            char ch = word[i];
            int index = ch - 'a';
            if ((index > 25) || (index < 0))
                throw std::exception("Found a character outside of the range a-z");

            if (currentIndex >= nodes.size())
                throw std::exception("Malformed trie");

            NodeIndex nextIndex = nodes[currentIndex].children[index];
            if (nextIndex == NODE_NONE)
                nextIndex = nodes[currentIndex].children[index] = allocateNode(i == wordLength - 1);

            currentIndex = nextIndex;
        }

        return currentIndex;
    }
}