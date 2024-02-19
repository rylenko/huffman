#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>
#include <stdlib.h>

unsigned int bytes_to_uint(const unsigned char *);
size_t calculate_frequencies(FILE *, size_t *);
void sort(
	void **,
	size_t,
	int (*)(const void *, const void *)
);
void uint_to_bytes(unsigned int, unsigned char *);

#endif
