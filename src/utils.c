#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

static void swap(void **a, size_t i, size_t j);

unsigned int
bytes_to_uint(const unsigned char *value)
{
	size_t rv = 0;
	unsigned char i,
		  size = sizeof(unsigned int);
	for (i = 0; i < size; ++i)
		rv |= value[i] << (size - i - 1) * 8;
	return rv;
}

size_t
calculate_frequencies(FILE *in, size_t *to)
{
	int c;
	size_t len = 0;
	for (; (c = fgetc(in)) != EOF; ++len)
		to[c]++;
	return len;
}

void
sort(void **a, size_t len, int (*cmp)(const void *, const void *))
{
	size_t i,
		j;
	for (i = 0; i < len - 1; ++i)
		for (j = 0; j < len - i - 1; ++j)
			if (cmp(a[j], a[j + 1]) == 1)
				swap(a, j, j + 1);
}

void
uint_to_bytes(unsigned int value, unsigned char *to)
{
	unsigned char i,
		size = sizeof(unsigned int);
	for (i = 0; i < size; ++i)
		to[i] = value >> (size - i - 1) * 8;
}

static void
swap(void **a, size_t i, size_t j)
{
	void *tmp = a[i];
	a[i] = a[j];
	a[j] = tmp;
}
