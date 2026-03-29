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
	FILE *fdebug;
	if(!fout) {
		freeTokens(tokens);
		free(text);
		err("unable to open tokens-output.txt for writing");
	}
	fdebug = fopen("tokens-debug.txt", "w");
	if(!fdebug) {
		fclose(fout);
		freeTokens(tokens);
		free(text);
		err("unable to open tokens-debug.txt for writing");
	}

	showTokens(tokens);
	showTokensToFile(tokens, fout);
	showTokensDebugToFile(tokens, fdebug);
	parse(tokens);
	fclose(fout);
	fclose(fdebug);
	freeTokens(tokens);
	free(text);
	return 0;
}
