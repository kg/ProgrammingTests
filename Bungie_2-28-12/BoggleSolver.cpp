#include "common.h"
#include <direct.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stack>
#include <sstream>
#include <algorithm>

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
    const unsigned MINIMUM_WORD_LENGTH = 3;

    Node::Node (char _character, bool _isValidWord)
        : isValidWord(_isValidWord)
        , character(_character)
    {
        memset(children, 0, sizeof(NodeIndex) * 26);
    }
    
    NodeIndex Dictionary::allocateNode (char character, bool isValidWord) {
        NodeIndex result = nodes.size();
        nodes.push_back(Node(character, isValidWord));
        return result;
    }

    Dictionary::Dictionary (const char * dictionaryPath)
        : wordCount(0)
    {
        // Allocate node 0 to be the root
        nodes.reserve(DEFAULT_DICTIONARY_SIZE);
        nodes.push_back(Node('\0', false));

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
            if (nextIndex == 0)
                // We need to evaluate nodes[currentIndex] again here because allocateNode may resize nodes
                nextIndex = nodes[currentIndex].children[index] = allocateNode(ch, i == wordLength - 1);

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

    static void exploreCellNeighbors (
        const Board * board, const Dictionary * dictionary, std::set<std::string> & result, 
        std::vector<CellId> cellStack, std::vector<NodeIndex> nodeStack
    ) {
        char ch = board->at(cellStack.back().x, cellStack.back().y);
        const Node & parentNode = dictionary->node(nodeStack.back());
        if (!parentNode.contains(ch))
            return;

        NodeIndex nodeIndex = parentNode.children[ch - 'a'];
        nodeStack.push_back(nodeIndex);
        if ((nodeStack.size() > MINIMUM_WORD_LENGTH) && dictionary->node(nodeIndex).isValidWord) {
            std::stringstream w;
            std::vector<NodeIndex>::iterator iter = nodeStack.begin();

            while (iter != nodeStack.end()) {
                // Ignore the root node
                if (*iter != 0) {
                    char ch = dictionary->node(*iter).character;
                    w << ch;
                }

                ++iter;
            }
            result.insert(w.str());
        }

        static CellId potentialNeighbors[] = {
            CellId(-1, -1), CellId(0, -1), CellId(1, -1),
            CellId(-1,  0),                CellId(1,  0),
            CellId(-1,  1), CellId(0,  1), CellId(1,  1)
        };

        for (unsigned i = 0; i < 9; i++) {
            CellId neighborId = cellStack.back() + potentialNeighbors[i];
            if (!board->isInBounds(neighborId))
                continue;

            // Cell already visited during this traversal, so we can't reuse it.
            if (std::find(cellStack.begin(), cellStack.end(), neighborId) != cellStack.end())
                continue;

            cellStack.push_back(neighborId);
            exploreCellNeighbors(board, dictionary, result, cellStack, nodeStack);
            cellStack.pop_back();
        }

        nodeStack.pop_back();
    }

    static void findWordsStartingInCell (
        const Board * board, const Dictionary * dictionary, 
        std::set<std::string> & result, CellId startCell
    ) {
        std::vector<CellId> cellStack;
        std::vector<NodeIndex> nodeStack;
        cellStack.push_back(startCell);
        nodeStack.push_back(0);

        exploreCellNeighbors(board, dictionary, result, cellStack, nodeStack);
    }

    std::set<std::string> Board::findWords (const Dictionary * dictionary) const {
        std::set<std::string> result;

        for (unsigned y = 0; y < height; y++) {
            for (unsigned x = 0; x < width; x++) {
                findWordsStartingInCell(this, dictionary, result, CellId(x, y));
            }
        }

        return result;
    }

    inline bool Board::isInBounds (const CellId & id) const {
        if (id.x >= width)
            return false;
        else if (id.y >= height)
            return false;
        else
            return true;
    }
}