#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "lexer.h"

int main()
{
	char *text = loadFile("test.c");
	Token *tokens = tokenize(text);
	FILE *fout = fopen("tokens-output.txt", "w");
	FILE *fdebug = fopen("tokens-debug.txt", "w");
	if(!fout) err("unable to open tokens-output.txt for writing");
	if(!fdebug) err("unable to open tokens-debug.txt for writing");

	showTokens(tokens);
	showTokensToFile(tokens, fout);
	showTokensDebugToFile(tokens, fdebug);
	fclose(fout);
	fclose(fdebug);
	return 0;
}
