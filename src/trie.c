#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHARS 26

struct Trie{
	int isLeaf;
	struct Trie *children[CHARS];
};

static struct Trie *createNewNode();
static int insert(struct Trie *root, char *str, unsigned len);
static int search(struct Trie *rutt, char *str, unsigned len, struct Trie **ress);
static void printTrie(struct Trie *ress, char *str, int len);

struct Trie * 
createNewNode()
{
	struct Trie *node = (struct Trie *)calloc(1, sizeof(struct Trie));
	node->isLeaf = 0;
	for(int i = 0; i < CHARS; ++i){
		node->children[i] = NULL;
	}
	return node;
}

int
insert(struct Trie *root, char *str, unsigned len)
{
	if(len == 0){
		root->isLeaf = 1;
		return 0;	
	}
	if(CHARS <= (*str - 'a'))
		exit(-1);

	if(root->children[*str - 'a'] == NULL)
		root->children[*str - 'a'] = createNewNode();

	return insert(root->children[*str - 'a'], str + 1, len - 1);
}

int
search(struct Trie *curr, char *str, unsigned len, struct Trie **ress)
{
	if(len == 0){
		*ress = curr;
		return 0;
	}

	if(CHARS <= (*str - 'a'))
		return -1;

	if(curr->children[*str - 'a'] == NULL)
		return -1;

	return search(curr->children[*str - 'a'], str + 1, len - 1, ress);
}

void
printTrie(struct Trie *ress, char *str, int len)
{
	if(ress->isLeaf == 1)
		printf("%.*s\n", len, str);

	for(int i = 0; i < CHARS; i++){

		if(NULL == ress->children[i])
			continue;

		str[len] = i + 'a';

		printTrie(ress->children[i], str, len + 1);
	}
}
