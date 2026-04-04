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

static const char *tokenName(int code){
	switch(code){
		case ID:return "ID";
		case TYPE_CHAR:return "TYPE_CHAR";
		case TYPE_DOUBLE:return "TYPE_DOUBLE";
		case ELSE:return "ELSE";
		case IF:return "IF";
		case TYPE_INT:return "TYPE_INT";
		case RETURN:return "RETURN";
		case STRUCT:return "STRUCT";
		case VOID:return "VOID";
		case WHILE:return "WHILE";
		case COMMA:return "COMMA";
		case END:return "END";
		case SEMICOLON:return "SEMICOLON";
		case LPAR:return "LPAR";
		case RPAR:return "RPAR";
		case LBRACKET:return "LBRACKET";
		case RBRACKET:return "RBRACKET";
		case LACC:return "LACC";
		case RACC:return "RACC";
		case ASSIGN:return "ASSIGN";
		case EQUAL:return "EQUAL";
		case ADD:return "ADD";
		case SUB:return "SUB";
		case MUL:return "MUL";
		case DIV:return "DIV";
		case DOT:return "DOT";
		case AND:return "AND";
		case OR:return "OR";
		case NOT:return "NOT";
		case NOTEQ:return "NOTEQ";
		case LESS:return "LESS";
		case LESSEQ:return "LESSEQ";
		case GREATER:return "GREATER";
		case GREATEREQ:return "GREATEREQ";
		case INT:return "INT";
		case DOUBLE:return "DOUBLE";
		case CHAR:return "CHAR";
		case STRING:return "STRING";
		default:return "UNKNOWN";
		}
	}

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
			}else{
			tkerr("dupa struct trebuie sa urmeze numele structurii");
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
			}else{
			tkerr("lipseste ] la finalul declararii de tablou");
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
			}else{
			tkerr("lipseste numele parametrului dupa tip");
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
				}else{
				tkerr("lipseste ; dupa declararea variabilei");
				}
			}else{
			tkerr("lipseste numele variabilei dupa tip");
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
			}else{
			tkerr("lipseste } la finalul blocului");
			}
		}
	iTk=start;
	return false;
	}

// fnDef: ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR stmCompound
bool fnDef(){
	Token *start=iTk;
	if(typeBase() || consume(VOID)){
		if(consume(ID)){
			if(consume(LPAR)){
				if(fnParam()){
					while(consume(COMMA)){
						if(!fnParam()){
							tkerr("dupa , trebuie sa urmeze un parametru valid");
							}
						}
					}
				if(consume(RPAR)){
					if(stmCompound()){
						return true;
						}else{
						tkerr("lipseste corpul functiei");
						}
					}else{
					tkerr("lipseste ) dupa lista de parametri");
					}
				}else{
				iTk=start;
				return false;
				}
			}else{
			iTk=start;
			return false;
			}
		}
	iTk=start;
	return false;
	}

//structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef(){
	Token *start=iTk;
	if(consume(STRUCT)){
		if(consume(ID)){
			if(consume(LACC)){
				while(varDef()){}
				if(consume(RACC)){
					if(consume(SEMICOLON)){
						return true;
						}else{
						tkerr("lipseste ; dupa definitia structurii");
						}
					}else{
					tkerr("lipseste } la finalul structurii");
					}
				}else{
				iTk=start;
				return false;
				}
			}else{
			tkerr("lipseste numele structurii dupa struct");
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
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()){
						if(consume(ELSE)){
							if(stm()){
								return true;
								}else{
								tkerr("lipseste instructiunea dupa else");
								}
							}
						return true;
						}else{
						tkerr("lipseste instructiunea dupa if");
						}
					}else{
					tkerr("lipseste ) dupa conditia din if");
					}
				}else{
				tkerr("lipseste expresia de conditie din if");
				}
			}else{
			tkerr("lipseste ( dupa if");
			}
		}

	iTk=start;
	if(consume(WHILE)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()){
						return true;
						}else{
						tkerr("lipseste instructiunea dupa while");
						}
					}else{
					tkerr("lipseste ) dupa conditia din while");
					}
				}else{
				tkerr("lipseste expresia de conditie din while");
				}
			}else{
			tkerr("lipseste ( dupa while");
			}
		}

	iTk=start;
	if(consume(RETURN)){
		expr();
		if(consume(SEMICOLON)){
			return true;
			}else{
			tkerr("lipseste ; dupa instructiunea return");
			}
		}

	iTk=start;
	if(expr()){
		if(consume(SEMICOLON)){
			return true;
			}else{
			tkerr("lipseste ; dupa expresie");
			}
		}
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
				}else{
				tkerr("lipseste expresia din partea dreapta a atribuirii");
				}
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
				tkerr("dupa || trebuie sa urmeze o expresie valida");
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
				tkerr("dupa && trebuie sa urmeze o expresie valida");
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
					tkerr("dupa operatorul de egalitate trebuie sa urmeze o expresie valida");
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
					tkerr("dupa operatorul relational trebuie sa urmeze o expresie valida");
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
					tkerr("dupa + sau - trebuie sa urmeze o expresie valida");
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
					tkerr("dupa * sau / trebuie sa urmeze o expresie valida");
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
		if(typeName()){
			if(consume(RPAR)){
				if(exprCast()){
					return true;
					}else{
					tkerr("lipseste expresia dupa cast");
					}
				}else{
				tkerr("lipseste ) dupa tipul folosit la cast");
				}
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
			}else{
			tkerr("lipseste expresia dupa operatorul unar");
			}
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
				if(expr()){
					if(consume(RBRACKET)){
						continue;
						}else{
						tkerr("lipseste ] dupa indexarea in tablou");
						}
					}else{
					tkerr("lipseste expresia dintre [ si ]");
					}
				}
			iTk=opStart;
			if(consume(DOT)){
				if(consume(ID)){
					continue;
					}else{
					tkerr("lipseste numele campului dupa .");
					}
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
						tkerr("dupa , trebuie sa urmeze un argument valid");
						}
					}
				if(consume(RPAR)){
					return true;
					}else{
					tkerr("lipseste ) dupa argumentele functiei");
					}
				}else if(consume(RPAR)){
				return true;
				}else{
				tkerr("apel de functie invalid: lipseste argument sau )");
				}
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
		if(expr()){
			if(consume(RPAR)){
				return true;
				}else{
				tkerr("lipseste ) dupa expresia dintre paranteze");
				}
			}else{
			tkerr("lipseste expresia dupa (");
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
	tkerr("token invalid la nivel global: %s",tokenName(iTk->code));
	return false;
	}

void parse(Token *tokens){
	iTk=tokens;
	consumedTk=NULL;
	unit();
	}
