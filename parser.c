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

static int errLine(const Token *tk){
	if(tk){
		return tk->line;
		}
	if(iTk){
		return iTk->line;
		}
	return 0;
	}

const char *tkCodeName(int code){
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
	fprintf(stderr,"eroare la linia %d: ",errLine(iTk));
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
	}

void tkerrAt(const Token *tk,const char *fmt,...){
	fprintf(stderr,"eroare la linia %d: ",errLine(tk));
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
	}

bool consume(int code){
	printf("consume(%s)",tkCodeName(code));
	if(iTk->code==code){
		consumedTk=iTk;
		iTk=iTk->next;
		printf(" => consumed\n");
		return true;
		}
	printf(" => found %s\n",tkCodeName(iTk->code));
	return false;
	}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
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
		tkerr("lipseste numele structurii dupa struct");
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

// arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(consume(LBRACKET)){
		if(!consume(INT) && iTk->code!=RBRACKET){
			tkerr("dimensiunea tabloului trebuie sa fie constanta intreaga sau vida");
			}
		if(consume(RBRACKET)){
			return true;
			}
		tkerr("lipseste ] in declaratia de tablou");
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

// typeName: typeBase arrayDecl?
bool typeName(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(typeBase()){
		arrayDecl();
		return true;
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

// fnParam: typeBase ID arrayDecl?
bool fnParam(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(typeBase()){
		if(consume(ID)){
			arrayDecl();
			return true;
			}
		tkerr("lipseste numele parametrului dupa tip");
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

// varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(typeBase()){
		if(consume(ID)){
			arrayDecl();
			if(consume(SEMICOLON)){
				return true;
				}
			tkerrAt(consumedTk,"lipseste ; dupa declaratia variabilei");
			}
		tkerr("lipseste numele variabilei dupa tip");
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

// stmCompound: LACC ( varDef | stm )* RACC
bool stmCompound(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(consume(LACC)){
		for(;;){
			if(varDef()){}
			else if(stm()){}
			else break;
			}
		if(consume(RACC)){
			return true;
			}
		if(iTk->code==END){
			tkerrAt(consumedTk,"lipseste } la sfarsitul blocului");
			}
		tkerr("instructiune invalida in bloc sau lipseste }");
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

// fnDef: ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR stmCompound
bool fnDef(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	bool isVoid=false;
	if(typeBase() || (isVoid=consume(VOID))){
		if(consume(ID)){
			if(consume(LPAR)){
				if(fnParam()){
					while(consume(COMMA)){
						if(!fnParam()){
							tkerr("parametru invalid dupa , in lista de parametri");
							}
						}
					}
				if(consume(RPAR)){
					if(stmCompound()){
						return true;
						}
					tkerr("lipseste corpul functiei dupa antet");
					}
				tkerrAt(consumedTk,"lista de parametri invalida sau lipseste )");
				}
			if(isVoid){
				tkerrAt(consumedTk,"tipul void poate fi folosit doar la functii; lipseste ( dupa numele functiei");
				}
			iTk=start;
			consumedTk=startConsumed;
			return false;
			}
		if(isVoid){
			tkerr("lipseste numele functiei dupa void");
			}
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(consume(STRUCT)){
		if(consume(ID)){
			if(consume(LACC)){
				while(varDef()){}
				if(consume(RACC)){
					if(consume(SEMICOLON)){
						return true;
						}
					tkerrAt(consumedTk,"lipseste ; dupa definitia structurii");
					}
				if(iTk->code==END){
					tkerrAt(consumedTk,"lipseste } la sfarsitul definitiei structurii");
					}
				tkerr("declaratie invalida in structura sau lipseste }");
				}
			iTk=start;
			consumedTk=startConsumed;
			return false;
			}
		tkerr("lipseste numele structurii dupa struct");
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

// stm: stmCompound | IF LPAR expr RPAR stm ( ELSE stm )? | WHILE LPAR expr RPAR stm | RETURN expr? SEMICOLON | expr? SEMICOLON
bool stm(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(stmCompound()){
		return true;
		}

	iTk=start;
	consumedTk=startConsumed;
	if(consume(IF)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()){
						if(consume(ELSE)){
							if(stm()){
								return true;
								}
							tkerr("lipseste instructiunea dupa else");
							}
						return true;
						}
					tkerr("lipseste instructiunea dupa conditia if");
					}
				tkerr("conditie invalida pentru if sau lipseste )");
				}
			tkerr("lipseste conditia din if");
			}
		tkerr("lipseste ( dupa if");
		}

	iTk=start;
	consumedTk=startConsumed;
	if(consume(WHILE)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()){
						return true;
						}
					tkerr("lipseste instructiunea dupa conditia while");
					}
				tkerr("conditie invalida pentru while sau lipseste )");
				}
			tkerr("lipseste conditia din while");
			}
		tkerr("lipseste ( dupa while");
		}

	iTk=start;
	consumedTk=startConsumed;
	if(consume(RETURN)){
		expr();
		if(consume(SEMICOLON)){
			return true;
			}
		tkerrAt(consumedTk,"lipseste ; dupa return");
		}

	iTk=start;
	consumedTk=startConsumed;
	if(expr()){
		if(consume(SEMICOLON)){
			return true;
			}
		tkerrAt(consumedTk,"instructiune expresie invalida sau lipseste ;");
		}
	if(consume(SEMICOLON)){
		return true;
		}

	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

//expr: exprAssign
bool expr(){
	return exprAssign();
	}

//exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprUnary()){
		if(consume(ASSIGN)){
			if(exprAssign()){
				return true;
				}
			tkerr("lipseste expresia din dreapta operatorului =");
			}
		}
	iTk=start;
	consumedTk=startConsumed;
	return exprOr();
	}

// regula initiala:
//   exprOr: exprOr OR exprAnd | exprAnd
// transformarea folosita in cod:
//   exprOr: exprAnd ( OR exprAnd )*
bool exprOr(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprAnd()){
		while(consume(OR)){
			if(!exprAnd()){
				tkerr("lipseste o expresie valida dupa ||");
				}
			}
		return true;
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

// regula initiala:
//   exprAnd: exprAnd AND exprEq | exprEq
// transformarea folosita in cod:
//   exprAnd: exprEq ( AND exprEq )*
bool exprAnd(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprEq()){
		while(consume(AND)){
			if(!exprEq()){
				tkerr("lipseste o expresie valida dupa &&");
				}
			}
		return true;
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

// regula initiala:
//   exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel
// transformarea folosita in cod:
//   exprEq: exprRel ( ( EQUAL | NOTEQ ) exprRel )*
bool exprEq(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprRel()){
		for(;;){
			if(consume(EQUAL)){
				if(!exprRel()){
					tkerr("lipseste o expresie valida dupa ==");
					}
				}
			else if(consume(NOTEQ)){
				if(!exprRel()){
					tkerr("lipseste o expresie valida dupa !=");
					}
				}
			else break;
			}
		return true;
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

// regula initiala:
//   exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd
// transformarea folosita in cod:
//   exprRel: exprAdd ( ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd )*
bool exprRel(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprAdd()){
		for(;;){
			if(consume(LESS)){
				if(!exprAdd()){
					tkerr("lipseste o expresie valida dupa <");
					}
				}
			else if(consume(LESSEQ)){
				if(!exprAdd()){
					tkerr("lipseste o expresie valida dupa <=");
					}
				}
			else if(consume(GREATER)){
				if(!exprAdd()){
					tkerr("lipseste o expresie valida dupa >");
					}
				}
			else if(consume(GREATEREQ)){
				if(!exprAdd()){
					tkerr("lipseste o expresie valida dupa >=");
					}
				}
			else break;
			}
		return true;
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

// regula initiala:
//   exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul
// transformarea folosita in cod:
//   exprAdd: exprMul ( ( ADD | SUB ) exprMul )*
bool exprAdd(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprMul()){
		for(;;){
			if(consume(ADD)){
				if(!exprMul()){
					tkerr("lipseste o expresie valida dupa +");
					}
				}
			else if(consume(SUB)){
				if(!exprMul()){
					tkerr("lipseste o expresie valida dupa -");
					}
				}
			else break;
			}
		return true;
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

// regula initiala:
//   exprMul: exprMul ( MUL | DIV ) exprCast | exprCast
// transformarea folosita in cod:
//   exprMul: exprCast ( ( MUL | DIV ) exprCast )*
bool exprMul(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprCast()){
		for(;;){
			if(consume(MUL)){
				if(!exprCast()){
					tkerr("lipseste o expresie valida dupa *");
					}
				}
			else if(consume(DIV)){
				if(!exprCast()){
					tkerr("lipseste o expresie valida dupa /");
					}
				}
			else break;
			}
		return true;
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

//exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(consume(LPAR)){
		if(typeName()){
			if(consume(RPAR)){
				if(exprCast()){
					return true;
					}
				tkerr("lipseste expresia dupa conversia de tip");
				}
			tkerr("conversie de tip invalida sau lipseste )");
			}
		}
	iTk=start;
	consumedTk=startConsumed;
	return exprUnary();
	}

//exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(consume(SUB)){
		if(exprUnary()){
			return true;
			}
		tkerr("lipseste expresia dupa -");
		}
	if(consume(NOT)){
		if(exprUnary()){
			return true;
			}
		tkerr("lipseste expresia dupa !");
		}
	iTk=start;
	consumedTk=startConsumed;
	return exprPostfix();
	}

// regula initiala:
//   exprPostfix: exprPostfix LBRACKET expr RBRACKET | exprPostfix DOT ID | exprPrimary
// transformarea folosita in cod:
//   exprPostfix: exprPrimary ( LBRACKET expr RBRACKET | DOT ID )*
bool exprPostfix(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprPrimary()){
		for(;;){
			Token *opStart=iTk;
			Token *opStartConsumed=consumedTk;
			if(consume(LBRACKET)){
				if(expr()){
					if(consume(RBRACKET)){
						continue;
						}
					tkerr("index de tablou invalid sau lipseste ]");
					}
				tkerr("lipseste expresia dintre [ si ]");
				}
			iTk=opStart;
			consumedTk=opStartConsumed;
			if(consume(DOT)){
				if(consume(ID)){
					continue;
					}
				tkerr("lipseste numele campului dupa .");
				}
			iTk=opStart;
			consumedTk=opStartConsumed;
			break;
			}
		return true;
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

//exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
//| INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(consume(ID)){
		Token *afterId=iTk;
		Token *afterIdConsumed=consumedTk;
		if(consume(LPAR)){
			if(expr()){
				while(consume(COMMA)){
					if(!expr()){
						tkerr("argument invalid dupa ,");
						}
					}
				if(consume(RPAR)){
					return true;
					}
				tkerrAt(consumedTk,"lista de argumente invalida sau lipseste )");
				}
			else if(consume(RPAR)){
				return true;
				}
			else{
				tkerrAt(consumedTk,"apel de functie invalid sau lipseste )");
				}
			}
		iTk=afterId;
		consumedTk=afterIdConsumed;
		return true;
		}

	iTk=start;
	consumedTk=startConsumed;
	if(consume(INT) || consume(DOUBLE) || consume(CHAR) || consume(STRING)){
		return true;
		}

	iTk=start;
	consumedTk=startConsumed;
	if(consume(LPAR)){
		if(expr()){
			if(consume(RPAR)){
				return true;
				}
			tkerrAt(consumedTk,"expresie invalida intre paranteze sau lipseste )");
			}
		tkerr("lipseste expresia dupa (");
		}
	iTk=start;
	consumedTk=startConsumed;
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
	tkerr("eroare de sintaxa");
	return false;
	}

void parse(Token *tokens){
	iTk=tokens;
	consumedTk=NULL;
	unit();
	}
