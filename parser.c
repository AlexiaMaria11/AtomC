#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

Token *iTk;		// the iterator in the tokens list
Token *consumedTk;		// the last consumed token

bool arrayDecl();
bool typeName();
bool fnParam();
bool fnDef();
bool stm();
bool stmCompound();
bool expr();
bool exprAssign();
bool exprOr();
bool exprAnd();
bool exprEq();
bool exprRel();
bool exprAdd();
bool exprMul();
bool exprCast();
bool exprUnary();
bool exprPostfix();
bool exprPrimary();

void tkerr(const char *fmt,...){
	fprintf(stderr,"error in line %d: ",iTk->line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
	}

bool consume(int code){
	if(iTk->code==code){
		consumedTk=iTk;
		iTk=iTk->next;
		return true;
		}
	return false;
	}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase(){
	Token *start=iTk;
	if(consume(TYPE_INT)){
		return true;
		}
	if(consume(TYPE_DOUBLE)){
		return true;
		}
	if(consume(TYPE_CHAR)){
		return true;
		}
	if(consume(STRUCT)){
		if(consume(ID)){
			return true;
			}
		}
	iTk=start;
	return false;
	}

// arrayDecl: LBRACKET expr? RBRACKET
bool arrayDecl(){
	Token *start=iTk;
	if(consume(LBRACKET)){
		expr();
		if(consume(RBRACKET)){
			return true;
			}
		}
	iTk=start;
	return false;
	}

// typeName: typeBase arrayDecl?
bool typeName(){
	Token *start=iTk;
	if(typeBase()){
		arrayDecl();
		return true;
		}
	iTk=start;
	return false;
	}

// fnParam: typeBase ID arrayDecl?
bool fnParam(){
	Token *start=iTk;
	if(typeBase()){
		if(consume(ID)){
			arrayDecl();
			return true;
			}
		}
	iTk=start;
	return false;
	}

//varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef(){
	Token *start=iTk;
	if(typeBase()){
		if(consume(ID)){
			arrayDecl();
			if(consume(SEMICOLON)){
				return true;
				}
			}
		}
	iTk=start;
	return false;
	}

// stmCompound: LACC ( varDef | stm )* RACC
bool stmCompound(){
	Token *start=iTk;
	if(consume(LACC)){
		for(;;){
			if(varDef()){}
			else if(stm()){}
			else break;
			}
		if(consume(RACC)){
			return true;
			}
		}
	iTk=start;
	return false;
	}

// fnDef: ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR stmCompound
bool fnDef(){
	Token *start=iTk;
	if(typeBase() || consume(VOID)){
		if(consume(ID) && consume(LPAR)){
			if(fnParam()){
				while(consume(COMMA)){
					if(!fnParam()){
						iTk=start;
						return false;
						}
					}
				}
			if(consume(RPAR) && stmCompound()){
				return true;
				}
			}
		}
	iTk=start;
	return false;
	}

//structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef(){
	Token *start=iTk;
	if(consume(STRUCT) && consume(ID) && consume(LACC)){
		while(varDef()){}
		if(consume(RACC) && consume(SEMICOLON)){
			return true;
			}
		}
	iTk=start;
	return false;
	}

// stm: stmCompound | IF LPAR expr RPAR stm ( ELSE stm )? | WHILE LPAR expr RPAR stm | RETURN expr? SEMICOLON | expr? SEMICOLON
bool stm(){
	Token *start=iTk;
	if(stmCompound()){
		return true;
		}

	iTk=start;
	if(consume(IF)){
		if(consume(LPAR) && expr() && consume(RPAR) && stm()){
			if(consume(ELSE)){
				if(stm()){
					return true;
					}
				iTk=start;
				return false;
				}
			return true;
			}
		iTk=start;
		return false;
		}

	iTk=start;
	if(consume(WHILE)){
		if(consume(LPAR) && expr() && consume(RPAR) && stm()){
			return true;
			}
		iTk=start;
		return false;
		}

	iTk=start;
	if(consume(RETURN)){
		expr();
		if(consume(SEMICOLON)){
			return true;
			}
		iTk=start;
		return false;
		}

	iTk=start;
	expr();
	if(consume(SEMICOLON)){
		return true;
		}
	iTk=start;
	return false;
	}

bool expr(){
	return exprAssign();
	}

bool exprAssign(){
	Token *start=iTk;
	if(exprUnary()){
		if(consume(ASSIGN)){
			if(exprAssign()){
				return true;
				}
			iTk=start;
			return false;
			}
		}
	iTk=start;
	return exprOr();
	}

bool exprOr(){
	Token *start=iTk;
	if(exprAnd()){
		while(consume(OR)){
			if(!exprAnd()){
				iTk=start;
				return false;
				}
			}
		return true;
		}
	iTk=start;
	return false;
	}

bool exprAnd(){
	Token *start=iTk;
	if(exprEq()){
		while(consume(AND)){
			if(!exprEq()){
				iTk=start;
				return false;
				}
			}
		return true;
		}
	iTk=start;
	return false;
	}

bool exprEq(){
	Token *start=iTk;
	if(exprRel()){
		for(;;){
			if(consume(EQUAL) || consume(NOTEQ)){
				if(!exprRel()){
					iTk=start;
					return false;
					}
				}
			else break;
			}
		return true;
		}
	iTk=start;
	return false;
	}

bool exprRel(){
	Token *start=iTk;
	if(exprAdd()){
		for(;;){
			if(consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)){
				if(!exprAdd()){
					iTk=start;
					return false;
					}
				}
			else break;
			}
		return true;
		}
	iTk=start;
	return false;
	}

bool exprAdd(){
	Token *start=iTk;
	if(exprMul()){
		for(;;){
			if(consume(ADD) || consume(SUB)){
				if(!exprMul()){
					iTk=start;
					return false;
					}
				}
			else break;
			}
		return true;
		}
	iTk=start;
	return false;
	}

bool exprMul(){
	Token *start=iTk;
	if(exprCast()){
		for(;;){
			if(consume(MUL) || consume(DIV)){
				if(!exprCast()){
					iTk=start;
					return false;
					}
				}
			else break;
			}
		return true;
		}
	iTk=start;
	return false;
	}

bool exprCast(){
	Token *start=iTk;
	if(consume(LPAR)){
		if(typeName() && consume(RPAR) && exprCast()){
			return true;
			}
		}
	iTk=start;
	return exprUnary();
	}

bool exprUnary(){
	Token *start=iTk;
	if(consume(SUB) || consume(NOT)){
		if(exprUnary()){
			return true;
			}
		iTk=start;
		return false;
		}
	iTk=start;
	return exprPostfix();
	}

bool exprPostfix(){
	Token *start=iTk;
	if(exprPrimary()){
		for(;;){
			Token *opStart=iTk;
			if(consume(LBRACKET)){
				if(expr() && consume(RBRACKET)){
					continue;
					}
				iTk=start;
				return false;
				}
			iTk=opStart;
			if(consume(DOT)){
				if(consume(ID)){
					continue;
					}
				iTk=start;
				return false;
				}
			iTk=opStart;
			break;
			}
		return true;
		}
	iTk=start;
	return false;
	}

bool exprPrimary(){
	Token *start=iTk;
	if(consume(ID)){
		Token *afterId=iTk;
		if(consume(LPAR)){
			if(expr()){
				while(consume(COMMA)){
					if(!expr()){
						iTk=start;
						return false;
						}
					}
				}
			if(consume(RPAR)){
				return true;
				}
			iTk=start;
			return false;
			}
		iTk=afterId;
		return true;
		}

	iTk=start;
	if(consume(INT) || consume(DOUBLE) || consume(CHAR) || consume(STRING)){
		return true;
		}

	iTk=start;
	if(consume(LPAR)){
		if(expr() && consume(RPAR)){
			return true;
			}
		}
	iTk=start;
	return false;
}

// unit: ( structDef | fnDef | varDef )* END
bool unit(){
	for(;;){
		if(structDef()){}
		else if(fnDef()){}
		else if(varDef()){}
		else break;
		}
	if(consume(END)){
		return true;
		}
	return false;
	}

void parse(Token *tokens){
	iTk=tokens;
	consumedTk=NULL;
	unit();
	}
