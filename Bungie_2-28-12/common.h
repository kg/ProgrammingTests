#pragma once

#include <vector>

void reverse_words (char *);
void reverse_characters_in_place (char *, size_t);

struct s_node {
	struct s_node * next;
	struct s_node * reference;
};

struct s_node * duplicate_list (struct s_node *);

size_t copy_list_to_array (struct s_node *, struct s_node *[], size_t);
void free_list (struct s_node * list);

char * readEntireFile (const char * filePath);

namespace Boggle {
    class Node;
    class Dictionary;
    class Board;

    typedef unsigned int NodeIndex;
    const NodeIndex NODE_NONE = 0;

    class Node {
    public:
        NodeIndex children[26];
        const bool isValidWord;

        Node (bool validWord);
    };

    class Dictionary {
    private:
        std::vector<Node> nodes;

        NodeIndex allocateNode (bool validWord);

    public:
        unsigned wordCount;

        Dictionary (const char * dictionaryPath);

        NodeIndex addWord (const char * word, size_t wordLength);
    };

    class Board {
    private:
        char * characters;

    public:
        const unsigned width, height;

        Board  (unsigned width, unsigned height);
        ~Board ();

        static Board * fromFile   (const char * filename);
        static Board * fromString (const char * characters, size_t characterCount);

        char& at (unsigned col, unsigned row);
        char  at (unsigned col, unsigned row) const;
    };
}