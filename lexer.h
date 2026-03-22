#pragma once

#include <stdio.h>

enum{
    ID, 
    TYPE_CHAR, TYPE_DOUBLE, ELSE, IF, TYPE_INT, RETURN, STRUCT, VOID, WHILE,
    COMMA, END, SEMICOLON, LPAR, RPAR, LBRACKET, RBRACKET, LACC, RACC,
    ASSIGN, EQUAL, ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, NOTEQ, LESS, LESSEQ, GREATER, GREATEREQ,
    INT, DOUBLE, CHAR, STRING
};

typedef struct Token{
	int code;		// ID, TYPE_CHAR, ...
	int line;		// the line from the input file
	union{
		char *text;		// the chars for ID, STRING 
		int i;		// the value for INT
		char c;		// the value for CHAR
		double d;		// the value for DOUBLE
		};
	struct Token *next;		// next token in a simple linked list
	}Token;

Token *tokenize(const char *pch);
void showTokens(const Token *tokens);
void showTokensToFile(const Token *tokens, FILE *out);
void showTokensDebugToFile(const Token *tokens, FILE *out);
