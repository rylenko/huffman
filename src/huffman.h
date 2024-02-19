#ifndef _HUFFMAN_H
#define _HUFFMAN_H

#include <stdio.h>
#include <stdlib.h>

typedef struct Node Node;
struct Node {
	unsigned char c;
	size_t frequency;
	Node *left;
	Node *right;
};

void compress(FILE *, FILE *);
void decompress(FILE *, FILE *);

#endif /* _HUFFMAN_H */
