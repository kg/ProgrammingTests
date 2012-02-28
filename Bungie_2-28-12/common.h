#pragma once

void reverse_words (char *);
void reverseCharactersInPlace (char *, size_t);

struct s_node {
	struct s_node * next;
	struct s_node * reference;
};

struct s_node * duplicate_list (struct s_node *);