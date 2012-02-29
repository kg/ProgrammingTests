#include "common.h"
#include <map>

class ListIterator {
private:
    s_node * current;
    s_node * fast;

    // To detect cycles within the list, we have a 'fast' pointer that moves twice as fast through the list
    //  as the slow 'current' pointer. If the fast pointer is ever equal to the slow pointer, that indicates
    //  that the fast pointer has traversed a cycle in the list.
    // Note that it is essential for the fast pointer to be initialized to head->next, not head, and we must
    //  ensure that we don't treat fast==current as a cycle if both fast and current are 0 (end of list).
    inline void fast_next() {
        if (fast)
            fast = fast->next;
        if ((fast == current) && fast)
            throw std::exception("List contains a cycle");
    }

public:
    inline ListIterator (s_node * head) 
        : current(head)
        , fast(head->next) {
    }

    inline bool next () {
        if (current == 0)
            return false;

        current = current->next;
        fast_next();
        fast_next();

        return (current != 0);
    }

    inline s_node * ptr () const {
        return current;
    }

    inline s_node * reference () const {
        return current->reference;
    }
};

struct s_node * duplicate_list(struct s_node * list) {
    if (!list)
        return 0;

    std::map<const struct s_node *, struct s_node *> duplicatedNodes;

    try {
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

                std::map<const struct s_node *, struct s_node *>::iterator iter;

                iter = duplicatedNodes.find(current->next);
                if (iter != duplicatedNodes.end())
                    dupe->next = iter->second;
                else
                    dupe->next = 0;

                iter = duplicatedNodes.find(current->reference);
                if (iter != duplicatedNodes.end())
                    dupe->reference = iter->second;
                else
                    dupe->reference = 0;
            } while (iter.next());
        }
    } catch (...) {
        // A failure has occurred while duplicating the list (we probably hit a cycle).
        // We need to clean up any duplicate nodes we allocated since they will never escape to get freed
        //  by the caller.

        std::map<const struct s_node *, struct s_node *>::iterator iter = duplicatedNodes.begin();
        while (iter != duplicatedNodes.end()) {
            delete iter->second;
            ++iter;
        }

        throw;
    }

    // Return the head of the duplicated list. The caller is responsible for freeing the list.
    return duplicatedNodes[list];
}

// Copies the list's contents sequentially to the destination array. Returns the number of elements copied.
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