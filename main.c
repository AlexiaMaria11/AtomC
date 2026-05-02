#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "lexer.h"
#include "parser.h"
#include "ad.h"

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

	//showTokens(tokens);
	showTokensToFile(tokens, fout);
	pushDomain();
	parse(tokens);
	showDomain(symTable, "global");
	dropDomain();
	printf("Codul a fost scris corect.\n");
	fclose(fout);
	freeTokens(tokens);
	free(text);
	return 0;
}
