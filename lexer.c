#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h"
#include "utils.h"

Token *tokens;
Token *lastTk;
int line = 1;

Token *addTk(int code){
    Token *tk = safeAlloc(sizeof(Token));
    tk->code = code;
    tk->line = line;
    tk->next = NULL;
    if(lastTk){
        lastTk->next = tk;
    }else{
        tokens = tk;
    }
    lastTk = tk;
    return tk;
}

char *extract(const char *begin, const char *end){
    size_t len = (size_t)(end - begin);
    char *s = (char*)safeAlloc(len + 1);
    memcpy(s, begin, len);
    s[len] = '\0';
    return s;
}

static void advanceNewline(const char **ppch) {
    if (**ppch == '\r' && (*ppch)[1] == '\n') {
        (*ppch)++;
    }
    (*ppch)++;
    line++;
}

static int isEscapeChar(char c) {
    switch (c) {
        case 'a': case 'b': case 'f': case 'n': case 'r':
        case 't': case 'v': case '\\': case '\'': case '"':
        case '0':
            return 1;
        default:
            return 0;
    }
}


static char decodeEscape(const char **ppch, int literalLine, const char *literalType) {
    char c = **ppch;
    if (c == '\0') {
        err("line %d: %s literal ends immediately after backslash", literalLine, literalType);
    }
    if (!isEscapeChar(c)) {
        if (isprint((unsigned char)c)) {
            err("line %d: invalid escape sequence \\%c in %s literal", literalLine, c, literalType);
        }
        err("line %d: invalid escape sequence \\\\x%02X in %s literal", literalLine, (unsigned char)c, literalType);
    }

    (*ppch)++;
    switch (c) {
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';
        case '\'': return '\'';
        case '"': return '"';
        case '\\': return '\\';
        case '0': return '\0';
        default:
            err("line %d: unsupported escape sequence in %s literal", literalLine, literalType);
    }
}

static void appendChar(char **buf, size_t *len, size_t *cap, char c) {
    if (*len + 1 >= *cap) {
        size_t newCap = (*cap == 0) ? 16 : (*cap * 2);
        char *newBuf = safeAlloc(newCap);
        if (*buf) {
            memcpy(newBuf, *buf, *len);
            free(*buf);
        }
        *buf = newBuf;
        *cap = newCap;
    }
    (*buf)[(*len)++] = c;
    (*buf)[*len] = '\0';
}

static char *parseStringLiteral(const char **ppch) {
    int literalLine = line;
    char *buf = NULL;
    size_t len = 0, cap = 0;

    (*ppch)++;
    while (**ppch) {
        if (**ppch == '"') {
            (*ppch)++;
            if (!buf) {
                buf = safeAlloc(1);
                buf[0] = '\0';
            }
            return buf;
        }
        if (**ppch == '\n' || **ppch == '\r') {
            err("line %d: string literal is not closed before end of line", literalLine);
        }
        if (**ppch == '\\') {
            (*ppch)++;
            appendChar(&buf, &len, &cap, decodeEscape(ppch, literalLine, "string"));
        } else {
            appendChar(&buf, &len, &cap, **ppch);
            (*ppch)++;
        }
    }

    err("line %d: unterminated string literal", literalLine);
}

static char parseCharLiteral(const char **ppch) {
    int literalLine = line;
    char value;

    (*ppch)++;
    if (**ppch == '\0' || **ppch == '\n' || **ppch == '\r') {
        err("line %d: unterminated character literal", literalLine);
    }
    if (**ppch == '\'') {
        err("line %d: empty character literal is not allowed", literalLine);
    }

    if (**ppch == '\\') {
        (*ppch)++;
        value = decodeEscape(ppch, literalLine, "character");
    } else {
        value = **ppch;
        (*ppch)++;
    }

    if (**ppch != '\'') {
        if (**ppch == '\0' || **ppch == '\n' || **ppch == '\r') {
            err("line %d: unterminated character literal", literalLine);
        }
        err("line %d: character literal must contain exactly one character", literalLine);
    }

    (*ppch)++;
    return value;
}

static void validateNumberSuffix(const char *pch, int literalLine, const char *kind) {
    if (isalpha((unsigned char)*pch) || *pch == '_') {
        err("line %d: %s constant has invalid suffix starting with '%c'", literalLine, kind, *pch);
    }
}

static Token *parseNumberToken(const char **ppch) {
    const char *pch = *ppch;
    const char *start = pch;
    int literalLine = line;
    int isReal = 0;
    char *numText;
    Token *tk;

    while (isdigit((unsigned char)*pch)) pch++;

    if (*pch == '.') {
        if (isdigit((unsigned char)pch[1])) {
            isReal = 1;
            pch++;
            while (isdigit((unsigned char)*pch)) pch++;
            if (*pch == 'e' || *pch == 'E') {
                const char *expStart;
                pch++;
                if (*pch == '+' || *pch == '-') pch++;
                expStart = pch;
                while (isdigit((unsigned char)*pch)) pch++;
                if (pch == expStart) {
                    err("line %d: real constant exponent must contain at least one digit", literalLine);
                }
            }
        } else if (pch[1] == 'e' || pch[1] == 'E') {
            err("line %d: real constant must have at least one digit after '.'", literalLine);
        }
    } else if (*pch == 'e' || *pch == 'E') {
        const char *expStart;
        isReal = 1;
        pch++;
        if (*pch == '+' || *pch == '-') pch++;
        expStart = pch;
        while (isdigit((unsigned char)*pch)) pch++;
        if (pch == expStart) {
            err("line %d: real constant exponent must contain at least one digit", literalLine);
        }
    }

    validateNumberSuffix(pch, literalLine, isReal ? "real" : "integer");
    numText = extract(start, pch);

    if (isReal) {
        tk = addTk(DOUBLE);
        tk->d = strtod(numText, NULL);
    } else {
        tk = addTk(INT);
        tk->i = (int)strtol(numText, NULL, 10);
    }

    free(numText);
    *ppch = pch;
    return tk;
}

Token *tokenize(const char *pch){
    const char *start;
    Token *tk;

    tokens = NULL;
    lastTk = NULL;
    line = 1;

    for(;;){
        switch(*pch){
            case ' ': case '\t': pch++; break;
            case '\r':
            case '\n':
                advanceNewline(&pch);
                break;
            case '\0':
                addTk(END);
                return tokens;
            case '/':
                if(pch[1] == '/'){
                    pch += 2;
                    while(*pch && *pch != '\n' && *pch != '\r') pch++;
                    break;
                }
                addTk(DIV); pch++; break;
            case ',': addTk(COMMA); pch++; break;
            case ';': addTk(SEMICOLON); pch++; break;
            case '(': addTk(LPAR); pch++; break;
            case ')': addTk(RPAR); pch++; break;
            case '[': addTk(LBRACKET); pch++; break;
            case ']': addTk(RBRACKET); pch++; break;
            case '{': addTk(LACC); pch++; break;
            case '}': addTk(RACC); pch++; break;
            case '+': addTk(ADD); pch++; break;
            case '-': addTk(SUB); pch++; break;
            case '*': addTk(MUL); pch++; break;
            case '.': addTk(DOT); pch++; break;
            case '&':
                if(pch[1] == '&'){
                    addTk(AND);
                    pch += 2;
                }else{
                    err("line %d: single '&' is not a valid token; did you mean && ?", line);
                }
                break;
            case '|':
                if(pch[1] == '|'){
                    addTk(OR);
                    pch += 2;
                }else{
                    err("line %d: single '|' is not a valid token; did you mean || ?", line);
                }
                break;
            case '!':
                if(pch[1] == '='){ addTk(NOTEQ); pch += 2; }
                else { addTk(NOT); pch++; }
                break;
            case '=':
                if(pch[1] == '='){ addTk(EQUAL); pch += 2; }
                else { addTk(ASSIGN); pch++; }
                break;
            case '<':
                if(pch[1] == '='){ addTk(LESSEQ); pch += 2; }
                else { addTk(LESS); pch++; }
                break;
            case '>':
                if(pch[1] == '='){ addTk(GREATEREQ); pch += 2; }
                else { addTk(GREATER); pch++; }
                break;
            case '"':
                tk = addTk(STRING);
                tk->text = parseStringLiteral(&pch);
                break;
            case '\'':
                tk = addTk(CHAR);
                tk->c = parseCharLiteral(&pch);
                break;
            default:
                if(isalpha((unsigned char)*pch) || *pch == '_'){
                    start = pch++;
                    while(isalnum((unsigned char)*pch) || *pch == '_') pch++;
                    char *text = extract(start, pch);
                    int type = ID;
                    if(strcmp(text, "char") == 0) type = TYPE_CHAR;
                    else if(strcmp(text, "double") == 0) type = TYPE_DOUBLE;
                    else if(strcmp(text, "else") == 0) type = ELSE;
                    else if(strcmp(text, "if") == 0) type = IF;
                    else if(strcmp(text, "int") == 0) type = TYPE_INT;
                    else if(strcmp(text, "return") == 0) type = RETURN;
                    else if(strcmp(text, "struct") == 0) type = STRUCT;
                    else if(strcmp(text, "void") == 0) type = VOID;
                    else if(strcmp(text, "while") == 0) type = WHILE;
                    tk = addTk(type);
                    if(type == ID) tk->text = text;
                    else free(text);
                } else if(isdigit((unsigned char)*pch)){
                    parseNumberToken(&pch);
                } else {
                    if (isprint((unsigned char)*pch)) {
                        err("line %d: invalid character '%c' (%d)", line, *pch, (unsigned char)*pch);
                    }
                    err("line %d: invalid character with code %d", line, (unsigned char)*pch);
                }
        }
    }
}

const char *tokenNames[] = {
    "ID", "TYPE_CHAR", "TYPE_DOUBLE", "ELSE", "IF", "TYPE_INT", "RETURN", "STRUCT", "VOID", "WHILE",
    "COMMA", "END", "SEMICOLON", "LPAR", "RPAR", "LBRACKET", "RBRACKET", "LACC", "RACC",
    "ASSIGN", "EQUAL", "ADD", "SUB", "MUL", "DIV", "DOT", "AND", "OR", "NOT", "NOTEQ", "LESS", "LESSEQ", "GREATER", "GREATEREQ",
    "INT", "DOUBLE", "CHAR", "STRING"
};

static void fprintEscapedChar(FILE *out, char c) {
    switch ((unsigned char)c) {
        case '\a': fprintf(out, "\\a"); break;
        case '\b': fprintf(out, "\\b"); break;
        case '\f': fprintf(out, "\\f"); break;
        case '\n': fprintf(out, "\\n"); break;
        case '\r': fprintf(out, "\\r"); break;
        case '\t': fprintf(out, "\\t"); break;
        case '\v': fprintf(out, "\\v"); break;
        case '\\': fprintf(out, "\\\\"); break;
        case '\'': fprintf(out, "\\'"); break;
        case '\"': fprintf(out, "\\\""); break;
        case '\0': fprintf(out, "\\0"); break;
        default: fputc((unsigned char)c, out); break;
    }
}

static void fprintEscapedString(FILE *out, const char *text) {
    for (; *text; text++) {
        fprintEscapedChar(out, *text);
    }
}

void showTokensDebugToFile(const Token *tokens, FILE *out) {
    for (const Token *tk = tokens; tk; tk = tk->next) {
        fprintf(out, "%d\t%s", tk->line, tokenNames[tk->code]);
        switch (tk->code) {
            case ID: fprintf(out, ":%s", tk->text); break;
            case INT: fprintf(out, ":%d", tk->i); break;
            case DOUBLE: fprintf(out, ":%g", tk->d); break;
            case CHAR: fprintf(out, ":"); fprintEscapedChar(out, tk->c); break;
            case STRING: fprintf(out, ":"); fprintEscapedString(out, tk->text); break;
        }
        fprintf(out, "\n");
    }
}

void showTokensToFile(const Token *tokens, FILE *out) {
    for (const Token *tk = tokens; tk; tk = tk->next) {
        fprintf(out, "%d\t%s", tk->line, tokenNames[tk->code]);
        switch (tk->code) {
            case ID: fprintf(out, ":%s", tk->text); break;
            case INT: fprintf(out, ":%d", tk->i); break;
            case DOUBLE: fprintf(out, ":%g", tk->d); break;
            case CHAR: fprintf(out, ":%c", tk->c); break;
            case STRING: fprintf(out, ":%s", tk->text); break;
        }
        fprintf(out, "\n");
    }
}

void showTokens(const Token *tokens) {
    for (const Token *tk = tokens; tk; tk = tk->next) {
        printf("%d\t%s", tk->line, tokenNames[tk->code]);
        switch (tk->code) {
            case ID: printf(":%s", tk->text); break;
            case INT: printf(":%d", tk->i); break;
            case DOUBLE: printf(":%g", tk->d); break;
            case CHAR: printf(":%c", tk->c); break;
            case STRING: printf(":%s", tk->text); break;
        }
        printf("\n");
    }
}
