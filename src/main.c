#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "consts.h"
#include "errors.h"
#include "huffman.h"

void
usage(void)
{
	printf(USAGE);
	exit(0);
}

int
main(int argc, char **argv)
{
	FILE *in;

	if (argc != 3)
		usage();
	else if ((in = fopen(argv[2], "rb")) == NULL)
		exit(OPEN_IN);

	if (strcmp(argv[1], "compress") == 0) {
		compress(in, stdout);
	} else if (strcmp(argv[1], "decompress") == 0) {
		decompress(in, stdout);
	} else {
		usage();
	}

	fclose(in);
	return EXIT_SUCCESS;
}
