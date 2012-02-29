#include "common.h"
#include <direct.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stack>
#include <sstream>
#include <algorithm>

// Reads the entire contents of a file into a buffer (that you must delete using delete[]). The buffer is null terminated.
// At completion fileSize will be updated to contain the size of the file's contents (not including the null terminator).
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
    // New dictionaries are initialized with at least this many nodes. 
    // Tuning this upward might improve dictionary creation performance.
    const unsigned DEFAULT_DICTIONARY_SIZE = 4096;
    // Boggle rules state that a valid word must be 3 letters.
    const unsigned MINIMUM_WORD_LENGTH = 3;

    Node::Node (char _character, bool _isValidWord)
        : isValidWord(_isValidWord)
        , character(_character)
    {
        // Within a node, 0 represents no child instead of representing the root,
        //  because no node can ever point back to the root node.
        memset(children, 0, sizeof(NodeIndex) * 26);
    }
    
    // Creates a new node for the given character and returns its NodeIndex.
    // Note that calling this may resize the nodes vector and invalidate any references
    //  to existing nodes.
    NodeIndex Dictionary::allocateNode (char character, bool isValidWord) {
        NodeIndex result = nodes.size();
        nodes.push_back(Node(character, isValidWord));
        return result;
    }

    Dictionary::Dictionary (const char * dictionaryPath)
        : wordCount(0)
    {
        // Allocate node 0 to be the root.
        nodes.reserve(DEFAULT_DICTIONARY_SIZE);
        // The root node does not actually contain character information, just children.
        nodes.push_back(Node('\0', false));

        const char * dictionaryBuffer;
        size_t dictionaryLength;

        dictionaryBuffer = readEntireFile(dictionaryPath, dictionaryLength);

        try {
          // Scan through the dictionary file for words and add them to the dictionary.
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
        } catch (...) {
          delete[] dictionaryBuffer;
          throw;
        }
    }

    NodeIndex Dictionary::addWord (const char * word, size_t wordLength) {
        // Start at the root
        NodeIndex currentIndex = 0;        

        // Walk through the word one character at a time, ensuring that the entire path
        //  through the trie that represents the word exists. Any time we find a missing
        //  node, we must create it.
        for (unsigned i = 0; i < wordLength; i++) {
            char ch = tolower(word[i]);
          
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

        // First, we make a pass through the entire board to determine its width/height.
        
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

        // Now that we know the size of the board, we can allocate space for it.
        Board * result = new Board(rowWidth, numRows);

        // And then finally, copy the characters from the string into the board.
        numRows = 0;
        currentRowWidth = 0;

        for (unsigned i = 0; i < characterCount; i++) {
            char ch = tolower(characters[i]);

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
        // First, grab the character value for the current cell, and fetch its
        //  associated node from the dictionary trie, if it exists.
        char ch = board->at(cellStack.back().x, cellStack.back().y);
        const Node & parentNode = dictionary->node(nodeStack.back());
        // The current node in the dictionary trie may not have any children for
        //  the current cell. If so, we can stop here without exploring neighbors.
        if (!parentNode.contains(ch))
            return;

        NodeIndex nodeIndex = parentNode.children[ch - 'a'];
        nodeStack.push_back(nodeIndex);
        
        // If the dictionary trie had a child for the current cell, and it is a
        //  valid word, add it to the results list.
        if ((nodeStack.size() > MINIMUM_WORD_LENGTH) && dictionary->node(nodeIndex).isValidWord) {
            // Convert the nodes on the stack into a string by walking them and
            //  concatenating their characters together.
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

        // We potentially explore all eight of a cell's neighbors
        static CellId potentialNeighbors[] = {
            CellId(-1, -1), CellId(0, -1), CellId(1, -1),
            CellId(-1,  0),                CellId(1,  0),
            CellId(-1,  1), CellId(0,  1), CellId(1,  1)
        };

        for (unsigned i = 0; i < 9; i++) {
            CellId neighborId = cellStack.back() + potentialNeighbors[i];
            // We don't want to walk off the edges of the board.
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

    // Sets up the recursive exploration of a given cell's neighbors for valid
    //  words. Ensures that given cells are not visited multiple times and also
    //  ensures that duplicate words are not added to the result set.
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

    // Scans the entire board for words using a provided dictionary.
    // Returns a set of the unique words found.
    std::set<std::string> Board::findWords (const Dictionary * dictionary) const {
        std::set<std::string> result;

        for (unsigned y = 0; y < height; y++) {
            for (unsigned x = 0; x < width; x++) {
                findWordsStartingInCell(this, dictionary, result, CellId(x, y));
            }
        }

        return result;
    }

    // Given x and y coordinates, returns true if the coordinates are within
    //  the bounds of the board.
    inline bool Board::isInBounds (const CellId & id) const {
        if (id.x >= width)
            return false;
        else if (id.y >= height)
            return false;
        else
            return true;
    }
}