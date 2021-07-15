#include <stdio.h>
#include "lib.h"

#define BUF_SIZE 2048

int main(int argc, char* argv[]) {
	char input[BUF_SIZE + 1] = { 0 };

	// read input up to BUF_SIZE bytes, this might truncate the input but
	// 2k sounds big enough for all reasonable list-id's..
	const char* ret = fgets(input, BUF_SIZE, stdin);

	if (ret == NULL) {
		return 0; // should this exit with 1 instead?
	}

	// just to be sure, sholdn't be requried as I already intialized
	// BUF_SIZE +1 bytes with '\0'
	input[BUF_SIZE] = '\0';

	// do the actual computation on the input
	// either of this might crash the process or do funky things as
	// I did write them :D
	const char* identifier = get_list_identifier(input);
	const char* tag = list_identifer_to_tag(identifier);

	fprintf(stdout, "tag: %s\n", tag);

	return 0;
}
