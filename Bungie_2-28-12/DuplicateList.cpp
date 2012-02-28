#include "common.h"
#include <map>

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
    if (!list)
        return 0;

    std::map<const struct s_node *, struct s_node *> duplicatedNodes;

    // First, perform a full pass over the list to duplicate each unique node.
    {
        ListIterator iter(list);
        do {
            struct s_node * const current = iter.ptr();
            struct s_node * const dupe = new struct s_node;
            duplicatedNodes[current] = dupe;
        } while (iter.next());
    }

    // Now do a second pass over the list to wire up all the pointers so that they point to the duplicates.
    {
        ListIterator iter(list);
        do {
            struct s_node * const current = iter.ptr();
            struct s_node * const dupe = duplicatedNodes[current];
            dupe->next = duplicatedNodes[current->next];
            dupe->reference = duplicatedNodes[current->reference];
        } while (iter.next());
    }

    // Return the head of the duplicated list.
    return duplicatedNodes[list];
}

size_t copy_list_to_array(struct s_node * list, struct s_node * destination_buffer[], size_t destination_buffer_size) {
    if (!list)
        return 0;

    ListIterator iter(list);
    size_t i = 0;

    while (i < destination_buffer_size) {
        destination_buffer[i++] = iter.ptr();

        if (!iter.next())
            break;
    }

    return i;
}

void free_list(struct s_node * list) {
    bool ok = (list != 0);

    ListIterator iter(list);
    while (ok) {
        struct s_node * const to_delete = iter.ptr();
        ok = iter.next();
        delete to_delete;
    };
}