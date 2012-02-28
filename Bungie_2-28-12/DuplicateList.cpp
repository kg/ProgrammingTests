#include "common.h"

class ListIterator {
private:
    s_node * current;

public:
    ListIterator (s_node * head) 
        : current(head) {
    }

    bool next () {
        if (current == 0)
            return false;

        current = current->next;
        return (current != 0);
    }

    s_node * ptr () const {
        return current;
    }

    s_node * reference () const {
        return current->reference;
    }
};

struct s_node * duplicate_list(struct s_node * list) {
    return 0;
}

void free_list(struct s_node * list) {
    bool ok = (list != 0);

    ListIterator iter(list);
    while (ok) {
        const struct s_node * to_delete = iter.ptr();
        ok = iter.next();
        delete to_delete;
    }
}