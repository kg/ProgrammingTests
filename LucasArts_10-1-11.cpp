//
// Reverse a null terminated string using a maximum of two pointers and a single local variable
//

char *begin = pStr;
char *end = pStr;

while (*end != 0)
  end++;

end--;

while (begin < end) {
  char temp = *begin;
  *begin = *end;
  *end = temp;
  begin++;
  end--;
}

//
// Convert a character string into a signed integer without using any libraries.
// String is valid and contains no white space.
//

char ch;
bool isNegative = false;
int result = 0;

while ((ch = *pStr++) != 0) {
  if (ch == '-') {
    isNegative = true;
    continue;
  }

  int placeValue = ch - '0';
  if ((placeValue < 0) || (placeValue > 9))
    continue;

  result *= 10;
  result += placeValue;

}

if (isNegative)
  return -result;
else
  return result;

//
// Multiply an integer by 500 without using the multiply/divide operators, or loops
//

// (N * 512) – (N * 8) – (N * 4) == (N * 500)
return (value << 9) – (value << 3) – (value << 2);

//
// Remove a node from a null-terminated doubly-linked list.
//

ASSERT(pNodeToRemove != 0);
ASSERT(g_pLinkedList != 0);

LinkNode_t *pPrevNode = pNodeToRemove->pPrev, *pNextNode =
pNodeToRemove->pNext;

if (pPrevNode) {
  ASSERT(pPrevNode->pNext == pNodeToRemove);
  pPrevNode->pNext = pNextNode;
}
if (pNextNode) {
  ASSERT(pNextNode->pPrev == pNodeToRemove);
  pNextNode->pPrev = pPrevNode;
}
if (pNodeToRemove == g_pLinkedList) {
  ASSERT(pPrevNode == 0);
  g_pLinkedList = pNextNode;
}

if (pData) {
  free(pData); // Or whatever’s actually appropriate
}