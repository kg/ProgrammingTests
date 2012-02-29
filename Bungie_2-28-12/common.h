#pragma once

#include <exception>
#include <vector>
#include <string>
#include <set>

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
    struct CellId;

    typedef unsigned int NodeIndex;

    class Node {
    public:
        const char character;
        const bool isValidWord;
        NodeIndex children[26];

        Node (char character, bool validWord);

        inline bool contains (char ch) const {
            int index = ch - 'a';
            if ((index < 0) || (index >= 26))
                return false;

            return children[index] != 0;
        }
    };

    class Dictionary {
    private:
        std::vector<Node> nodes;

        NodeIndex allocateNode (char character, bool isValidWord);

    public:
        unsigned wordCount;

        Dictionary (const char * dictionaryPath);

        NodeIndex addWord (const char * word, size_t wordLength);
        inline const Node& node (NodeIndex index) const {
            return nodes[index];
        }
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

        inline bool isInBounds (const CellId & id) const;

        char& at (unsigned col, unsigned row);
        char  at (unsigned col, unsigned row) const;

        std::set<std::string> findWords (const Dictionary * dictionary) const;
    };

    struct CellId {
    public:
        unsigned x, y;

        inline CellId (unsigned _x, unsigned _y)
            : x(_x)
            , y(_y) {
        }
    };

    inline static bool operator < (const CellId & lhs, const CellId & rhs) {
        if (lhs.y < rhs.y)
            return true;
        else if (lhs.x < rhs.x)
            return true;

        return false;
    }

    inline static bool operator == (const CellId & lhs, const CellId & rhs) {
        return (lhs.x == rhs.x) && (lhs.y == rhs.y);
    }

    inline static CellId operator + (const CellId & lhs, const CellId & rhs) {
        return CellId(lhs.x + rhs.x, lhs.y + rhs.y);
    }
}