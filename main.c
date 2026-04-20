#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "lexer.h"
#include "parser.h"

int main()
{
	char *text = loadFile("test.c");
	Token *tokens = tokenize(text);
	FILE *fout = fopen("tokens-output.txt", "w");
	if(!fout) {
		freeTokens(tokens);
		free(text);
		err("unable to open tokens-output.txt for writing");
	}

	showTokens(tokens);
	showTokensToFile(tokens, fout);
	parse(tokens);
	printf("Codul a fost scris corect.\n");
	fclose(fout);
	freeTokens(tokens);
	free(text);
	return 0;
}
