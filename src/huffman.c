#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "consts.h"
#include "errors.h"
#include "huffman.h"
#include "utils.h"

static Node* node_alloc(
	unsigned char c,
	size_t frequency,
	Node *left,
	Node *right
);
static int nodes_cmp(const Node *x, const Node *y);
static Node *tree_build(FILE *in, size_t *in_len);
static void tree_get_codes(const Node *tree, unsigned char *codes[256]);
static void tree_dump(
	const Node *node,
	unsigned char to[BUFSIZE],
	size_t *bit_position
);
static void tree_get_codes_inner(
	const Node *tree,
	unsigned char *codes[256],
	unsigned char path[256],
	size_t path_i
);
static void tree_free(Node *tree);
static void tree_free_codes(unsigned char *codes[256]);
static Node *tree_read(unsigned char in_buf[BUFSIZE], size_t *bit_position);
static void tree_write_compressed(
	const Node *tree,
	FILE *in,
	size_t in_len,
	FILE *out
);
static void tree_write_decompressed(
	const Node *tree,
	size_t bit_position,
	size_t decompressed_content_len,
	FILE *in,
	unsigned char in_buf[BUFSIZE],
	FILE *out
);

void
compress(FILE *in, FILE *out)
{
	size_t in_len;
	Node *tree;

	if (!(tree = tree_build(in, &in_len)))
		return;
	fseek(in, 0, SEEK_SET);
	tree_write_compressed(tree, in, in_len, out);
	tree_free(tree);
}

void
decompress(FILE *in, FILE *out)
{
	unsigned char in_buf[BUFSIZE] = {0};
	Node *tree;
	size_t bit_position = 0,
	decompressed_content_len;

	if (fread(in_buf, sizeof(unsigned char), BUFSIZE, in) == 0)
		return;
	else if ((decompressed_content_len = bytes_to_uint(in_buf)) == 0)
		return;
	bit_position += sizeof(unsigned int) * 8;

	tree = tree_read(in_buf, &bit_position);
	tree_write_decompressed(
		tree,
		bit_position,
		decompressed_content_len,
		in,
		in_buf,
		out
	);
	tree_free(tree);
}

static Node*
node_alloc(
	unsigned char c,
	size_t frequency,
	Node *left,
	Node *right
)
{
	Node *rv = malloc(sizeof(Node));
	if (!rv)
		exit(ALLOC_NODE);
	rv->c = c;
	rv->frequency = frequency;
	rv->left = left;
	rv->right = right;
	return rv;
}

static int
nodes_cmp(const Node *x, const Node *y)
{
	if (x->frequency == y->frequency)
		return 0;
	return x->frequency < y->frequency ? 1 : -1;
}

static Node*
tree_build(FILE *in, size_t *in_len)
{
	Node *nodes[256] = {NULL};
	size_t frequencies[256] = {0},
		i,
		nodes_count = 0;

	if ((*in_len = calculate_frequencies(in, frequencies)) == 0)
		return NULL;

	for (i = 0; i < 256; ++i)
		if (frequencies[i] != 0)
			nodes[nodes_count++] = node_alloc(i, frequencies[i], NULL, NULL);
	for (; nodes_count > 1; nodes_count--) {
		sort(
			(void **)&nodes[0],
			nodes_count,
			(int (*)(const void *, const void *))nodes_cmp
		);
		nodes[nodes_count - 2] = node_alloc(
			'\0',
			nodes[nodes_count - 2]->frequency + nodes[nodes_count - 1]->frequency,
			nodes[nodes_count - 2],
			nodes[nodes_count - 1]
		);
	}
	return nodes[0];
}

static void
tree_get_codes(const Node *tree, unsigned char *codes[256])
{
	unsigned char path[256];
	tree_get_codes_inner(tree, codes, path, 0);
}

static void
tree_get_codes_inner(
	const Node *tree,
	unsigned char *codes[256],
	unsigned char path[256],
	size_t path_i
)
{
	unsigned char *path_copy;
	if (tree->left || tree->right) {
		path[path_i] = '0';
		tree_get_codes_inner(tree->left, codes, path, path_i + 1);
		path[path_i] = '1';
		tree_get_codes_inner(tree->right, codes, path, path_i + 1);
	} else {
		path[path_i++] = '\0';
		if (!(path_copy = malloc(path_i)))
			exit(ALLOC_CODE_PATH);
		memcpy(path_copy, path, path_i);
		codes[tree->c] = path_copy;
	}
}

static void
tree_dump(
	const Node *node,
	unsigned char to[BUFSIZE],
	size_t *bit_position
)
{
	if (node->left) {
		to[*bit_position / 8] |= 128u >> *bit_position % 8;
		*bit_position += 1;
		tree_dump(node->left, to, bit_position);
		tree_dump(node->right, to, bit_position);
	} else {
		*bit_position += 1;
		to[*bit_position / 8] |= node->c >> *bit_position % 8;
		to[*bit_position / 8 + 1] |= node->c << (8 - *bit_position % 8);
		*bit_position += 8;
	}
}

static void
tree_free(Node *tree)
{
	if (tree->left)
		tree_free(tree->left);
	if (tree->right)
		tree_free(tree->right);
	free(tree);
}

static void
tree_free_codes(unsigned char *codes[256])
{
	unsigned char i;
	for (i = 0;; ++i) {
		if (codes[i])
			free(codes[i]);
		if (i == 255)
			break;
	}
}

static Node*
tree_read(unsigned char in_buf[BUFSIZE], size_t *bit_position)
{
	unsigned char bit = in_buf[*bit_position / 8] & (128u >> (*bit_position % 8));
	Node *node = malloc(BUFSIZE);
	if (!node)
		exit(ALLOC_NODE);

	(*bit_position)++;
	if (bit) {
		node->left = tree_read(in_buf, bit_position);
		node->right = tree_read(in_buf, bit_position);
	} else {
		node->left = node->right = NULL;
		node->c = in_buf[*bit_position / 8] << *bit_position % 8;
		node->c |= in_buf[*bit_position / 8 + 1] >> (8 - *bit_position % 8);
		*bit_position += 8;
	}

	return node;
}

static void
tree_write_compressed(
	const Node *tree,
	FILE *in,
	size_t in_len,
	FILE *out
)
{
	unsigned char *code,
		in_buf[BUFSIZE] = {0},
		dump[BUFSIZE] = {0},
		*codes[256] = {NULL};
	size_t bit_position = 0,
		i,
		j,
		n;

	uint_to_bytes(in_len, dump);
	bit_position += sizeof(unsigned int) * 8;

	tree_dump(tree, dump, &bit_position);
	tree_get_codes(tree, codes);

	while ((n = fread(in_buf, sizeof(unsigned char), BUFSIZE, in)) > 0) {
		for (i = 0; i < n; ++i) {
			code = codes[in_buf[i]];
			for (j = 0; code[j] != '\0'; ++j) {
				if (code[j] == '1')
					dump[bit_position / 8] |= 128u >> bit_position % 8;
				++bit_position;
				if (bit_position / 8 == BUFSIZE) {
					fwrite(dump, sizeof(unsigned char), BUFSIZE, out);
					memset(dump, 0, BUFSIZE);
					bit_position = 0;
				}
			}
		}
	}
	if (bit_position != 0)
		fwrite(
			dump,
			sizeof(unsigned char),
			bit_position / 8 + (bit_position % 8 > 0 ? 1 : 0),
			out
		);

	tree_free_codes(codes);
	fflush(out);
}

static void
tree_write_decompressed(
	const Node *tree,
	size_t bit_position,
	size_t decompressed_content_len,
	FILE *in,
	unsigned char in_buf[BUFSIZE],
	FILE *out
)
{
	unsigned char bit,
		out_buf[BUFSIZE];
	size_t i,
		out_bufi = 0;
	const Node *nodei;

	for (i = 0; i < decompressed_content_len; ++i) {
		nodei = tree;

		while (nodei->left) {
			bit = in_buf[bit_position / 8] & (128u >> bit_position % 8);
			nodei = bit ? nodei->right : nodei->left;
			++bit_position;
			if (bit_position / 8 == BUFSIZE) {
				memset(in_buf, 0, BUFSIZE);
				if (fread(in_buf, sizeof(unsigned char), BUFSIZE, in) == 0)
					exit(INVALID_COMPRESSED_CONTENT);
				bit_position = 0;
			}
		}
		out_buf[out_bufi++] = nodei->c;
		if (out_bufi == BUFSIZE) {
			fwrite(out_buf, sizeof(unsigned char), BUFSIZE, out) ;
			memset(out_buf, 0, BUFSIZE);
			out_bufi = 0;
		}
	}
	if (out_bufi > 0)
		fwrite(out_buf, sizeof(unsigned char), out_bufi, out);
	fflush(out);
}
