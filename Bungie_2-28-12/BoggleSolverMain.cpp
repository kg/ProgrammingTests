#include "common.h"

using namespace Boggle;

int main (int argc, const char* argv[]) {
    if (argc != 3) {
        printf("Usage: BoggleSolver [dictionary.txt] [board.txt]\n");
        return 1;
    }

    try {
        fprintf(stderr, "// Loading dictionary from '%s' ... ", argv[1]);
        Dictionary * dictionary = new Dictionary(argv[1]);
        fprintf(stderr, "done.\n");

        fprintf(stderr, "// Loading board from '%s' ... ", argv[2]);
        Board * board = Board::fromFile(argv[2]);
        fprintf(stderr, "done.\n");

        fprintf(stderr, "// Finding words ... ");
        std::set<std::string> words = board->findWords(dictionary);

        fprintf(stderr, "%d word(s) found.\n", words.size());
        std::set<std::string>::iterator iter = words.begin();
        while (iter != words.end()) {
            printf("%s\n", iter->c_str());
            ++iter;
        }
    } catch (std::exception exc) {
        printf("An error occurred: %s\n", exc.what());
        return 1;
    }
}