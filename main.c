#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "lexer.h"
#include "parser.h"
#include "ad.h"
#include "vm.h"

int main()
{
	char *text = loadFile("tests/testgc.c");
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
	vmInit();
	parse(tokens);
	//showDomain(symTable, "global");
	Symbol *symMain=findSymbolInDomain(symTable,"main");
	if(!symMain)err("missing main function");
	Instr *entryCode=NULL;
	addInstr(&entryCode,OP_CALL)->arg.instr=symMain->fn.instr;
	addInstr(&entryCode,OP_HALT);
	run(entryCode);
	dropDomain();
	printf("\nCodul a fost scris corect.\n");
	fclose(fout);
	freeTokens(tokens);
	free(text);
	return 0;
}
