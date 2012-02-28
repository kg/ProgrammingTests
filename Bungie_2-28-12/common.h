#pragma once

void reverse_words (char *);
void reverse_characters_in_place (char *, size_t);

struct s_node {
	struct s_node * next;
	struct s_node * reference;
};

struct s_node * duplicate_list (struct s_node *);

size_t copy_list_to_array(struct s_node *, struct s_node *[], size_t);
void free_list(struct s_node * list);