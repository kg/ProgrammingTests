//
// Given a binary tree where each node is a character, locate the deepest nested node and print out the full path to that node.
//

#include <vector>
#include <cstdio>
#include <iterator>

using std::vector;
using std::copy;
using std::back_inserter;

template <typename T> void PrintStack (T & stack) {
    for (auto iter = stack.begin(), last = stack.end(); iter != last; ++iter)
        putchar((*iter)->c);
}

void PrintStringToDeepestLeafNode (Node * pRoot) {
    Node * popped = 0;
    vector<Node *> current, deepest;

    current.push_back(pRoot);

    while (size_t s = current.size()) {
        /* Any time we reach a new depth we make a copy of the current       */
        /* traversal stack so that the end of the function we will have all  */
        /* the pointers we need to print the entire path.                    */
        if (s > deepest.size()) {
            deepest.clear();
            copy(current.begin(), current.end(), back_inserter(deepest));
        }

        Node * const top = current[s - 1];
        Node * const prior = s > 1 ? current[s - 2] : 0;

        Node * const left = top->pLeft;
        Node * const right = top->pRight;

        bool const poppedLeft = (left == popped);
        bool const poppedRight = (right == popped);

        /* If the current node has a right leaf and we just climbed up from  */
        /* the left leaf, then climb down into the right leaf                */
        if (
            right && poppedLeft
        ) {
            current.push_back(right);

        /* Otherwise, if the current node has a left leaf and we have not    */
        /* just climbed up from the right leaf of this node, climb down into */
        /* the left leaf. We must also handle cases where there is no left   */
        /* leaf and cases where there is no right leaf.                      */
        } else if (
            left && (
                !poppedLeft && (!poppedRight || !right)
            )
        ) {
            current.push_back(left);

        /* If we decided not to climb down into either leaf it's time to     */
        /* climb back up. First, we check whether we are currently inside    */
        /* the left leaf of a node. If so, we may want to pivot into the     */
        /* right leaf. If we can't pivot into the right leaf, we just climb  */
        /* up.                                                               */
        } else if (prior) {
            Node * const priorRight = prior->pRight;

            if (
                (prior->pLeft == top) && 
                priorRight && (priorRight != popped)
            ) {
                current[s - 1] = priorRight;

            } else {
                current.pop_back();
                popped = top;
                continue;
            }

        } else {
            break;
        }

        popped = 0;
    }

    printf("Result: ");
    PrintStack(deepest);
    printf("\n");
}

//
// Determine which, if any, of a pair of bounding boxes encloses the other.
//

/* Store bounding pairs instead of position+size so that there's no need */
/* to add the position+size pairs every time a comparison is performed   */
struct AABB {
    int x1, x2;
    int y1, y2;
};

/* Using a negative value for first enables the code generator to make   */
/* use of je/jl/jg etc instead of using cmp exclusively when checking    */
/* values (though the existence of SAME means it still needs cmp)        */
enum Enclosing {
    /* Neither volume fully encloses the other.                          */
    NEITHER = 0,
    /* The first volume encloses the second volume.                      */
    FIRST = -1,
    /* The second volume encloses the first volume.                      */
    SECOND = 1,
    /* The volumes are identical.                                        */
    SAME = 2
};

/* Compares a pair of intervals to determine whether one fully encloses  */
/* the other or whether they are identical                               */
Enclosing CheckEnclosingInterval (
    /* each interval is defined by [value1, value2]                      */
    const int first1, const int first2, 
    const int second1, const int second2
) {
    const int delta1 = second1 - first1, delta2 = second2 - first2;

    if (delta1 == 0) {
        if (delta2 == 0)
            return SAME;
        else if (delta2 > 0)
            return SECOND;
        else
            return FIRST;
    } else if (delta1 > 0) {
        if (delta2 <= 0)
            return FIRST;
    } else {
        if (delta2 >= 0)
            return SECOND;
    }

    return NEITHER;
}

/* Passing the bounding boxes by reference instead of by value incurs    */
/* a performance hit due to the indirection, but bounding boxes are big  */
/* enough that it would be difficult to fit them in registers. since     */
/* each data member of each bounding box is only accessed once the total */
/* overhead of the indirection ends up being minimal.                    */
Enclosing CheckEnclosing (const AABB & first, const AABB & second) {
    /* Compare the x and y intervals representing each bounding box.     */
    /* Since these operations do not depend on any shared state it is    */
    /* possible for them to be executed simultaneously on an out-of-     */
    /* order processor or be pipelined by a sufficiently brilliant       */
    /* compiler.                                                         */
    const Enclosing x = CheckEnclosingInterval(
        first.x1, first.x2, second.x1, second.x2
    );
    const Enclosing y = CheckEnclosingInterval(
        first.y1, first.y2, second.y1, second.y2
    );

    /* Now with knowledge of whether the x and y intervals enclose each  */
    /* other, are the same, or are distinct, we can determine whether    */
    /* the two bounding boxes enclose each other or are separate.        */
    switch (x) {
        case SAME:
            return y;
        case NEITHER:
            return NEITHER;

        case FIRST:
            switch (y) {
                case SECOND:
                case NEITHER:
                    return NEITHER;
                default:
                    return FIRST;
            }

        case SECOND:
            switch (y) {
                case FIRST:
                case NEITHER:
                    return NEITHER;
                default:
                    return SECOND;
            }
    }
}