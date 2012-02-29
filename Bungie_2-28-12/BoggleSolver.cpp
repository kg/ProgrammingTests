#include "common.h"
#include <direct.h>
#include <stdio.h>
#include <sys/stat.h>

const char * readEntireFile (const char * filePath, size_t & fileSize) {
    FILE * file;
    char * buffer;
    struct stat stat;

    file = fopen(filePath, "rb");
    if (!file)
        throw std::exception("Failed to open file");

    if (fstat(fileno(file), &stat)) {
        fclose(file);
        throw std::exception("Failed to get file info");
    }

    try {
        buffer = new char[stat.st_size + 1];
        size_t bytes_read = fread(buffer, 1, stat.st_size, file);
        buffer[stat.st_size] = 0;
        fclose(file);

        if (bytes_read != stat.st_size) {
            throw std::exception("Failed to read dictionary");
        }
    } catch (...) {
        delete[] buffer;

        throw;
    }

    fileSize = stat.st_size;
    return buffer;
}

namespace Boggle {
    const unsigned DEFAULT_DICTIONARY_SIZE = 1024;

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

    Dictionary::Dictionary (const char * dictionaryPath)
        : wordCount(0)
    {
        // Allocate node 0 to be the root
        nodes.reserve(DEFAULT_DICTIONARY_SIZE);
        nodes.push_back(Node(false));

        const char * dictionaryBuffer;
        size_t dictionaryLength;

        dictionaryBuffer = readEntireFile(dictionaryPath, dictionaryLength);

        unsigned currentWordStart = 0;
        for (unsigned i = 0; i < dictionaryLength; i++) {
            char ch = dictionaryBuffer[i];

            if ((ch == '\n') || (ch == '\r') || (ch == '\0')) {
                size_t currentWordLength = i - currentWordStart;
                if (currentWordLength)
                    addWord(dictionaryBuffer + currentWordStart, currentWordLength);

                currentWordStart = i + 1;
            }
        }

        size_t currentWordLength = dictionaryLength - currentWordStart;
        if ((currentWordLength + currentWordStart <= dictionaryLength) && (currentWordLength))
            addWord(dictionaryBuffer + currentWordStart, currentWordLength);

        delete[] dictionaryBuffer;
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
                // We need to evaluate nodes[currentIndex] again here because allocateNode may resize nodes
                nextIndex = nodes[currentIndex].children[index] = allocateNode(i == wordLength - 1);

            currentIndex = nextIndex;
        }

        wordCount++;
        return currentIndex;
    }

    Board::Board (unsigned _width, unsigned _height)
        : width(_width)
        , height(_height)
        , characters(new char[_width * _height])
    {
    }

    Board::~Board () {
        delete[] characters;
    }

    Board * Board::fromFile (const char * filename) {
        size_t boardStringLength;
        const char * boardString = readEntireFile(filename, boardStringLength);
        Board * result = Board::fromString(boardString, boardStringLength);
        delete[] boardString;
        return result;
    }

    Board * Board::fromString (const char * characters, size_t characterCount) {
        unsigned rowWidth = 0, numRows = 0, currentRowWidth = 0;

        for (unsigned i = 0; i < characterCount; i++) {
            char ch = characters[i];

            if (
                (ch == '\n') || (ch == '\r') || (ch == '\0')
            ) {
                if (currentRowWidth) {
                    numRows += 1;
                    if (rowWidth == 0)
                        rowWidth = currentRowWidth;
                    else if (rowWidth != currentRowWidth)
                        throw std::exception("Board has inconsistent row widths");
                }

                currentRowWidth = 0;
            } else {
                currentRowWidth += 1;
            }
        }

        if (currentRowWidth) {
            numRows += 1;
            if (rowWidth == 0)
                rowWidth = currentRowWidth;
            else if (rowWidth != currentRowWidth)
                throw std::exception("Board has inconsistent row widths");
        }

        Board * result = new Board(rowWidth, numRows);

        numRows = 0;
        currentRowWidth = 0;

        for (unsigned i = 0; i < characterCount; i++) {
            char ch = characters[i];

            if (
                (ch == '\n') || (ch == '\r') || (ch == '\0')
            ) {
                if (currentRowWidth)
                    numRows += 1;

                currentRowWidth = 0;
            } else {
                result->at(currentRowWidth, numRows) = ch;

                currentRowWidth += 1;
            }
        }

        return result;
    }

    char& Board::at (unsigned col, unsigned row) {
        if ((col >= width) || (row >= height))
            throw std::exception("Index out of range");

        return characters[(row * width) + col];
    }

    char Board::at (unsigned col, unsigned row) const {
        if ((col >= width) || (row >= height))
            throw std::exception("Index out of range");

        return characters[(row * width) + col];
    }
}