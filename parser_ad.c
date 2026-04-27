#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "parser.h"
#include "ad.h"

Token *iTk;		// the iterator in the tokens list
Token *consumedTk;		// the last consumed token
Symbol *currentStruct=NULL;		// !=NULL while parsing a struct body
Symbol *currentFn=NULL;		// !=NULL while parsing a function body

bool arrayDecl(Type *t);
bool typeBase(Type *t);
bool typeName(Type *t);
bool fnParam();
bool fnDef();
bool stm();
bool stmCompound();
bool expr();
bool exprAssign();
bool exprOr();
bool exprOrPrim();
bool exprAnd();
bool exprAndPrim();
bool exprEq();
bool exprEqPrim();
bool exprRel();
bool exprRelPrim();
bool exprAdd();
bool exprAddPrim();
bool exprMul();
bool exprMulPrim();
bool exprCast();
bool exprUnary();
bool exprPostfix();
bool exprPostfixPrim();
bool exprPrimary();
void tkerrAt(const Token *tk,const char *fmt,...);

static Type makeType(TypeBase tb){
	Type t;
	t.tb=tb;
	t.s=NULL;
	t.n=-1;
	return t;
	}

static void addExtFns(){
	Type t=makeType(TB_VOID);
	addExtFn("puti",NULL,t);
	addExtFn("putd",NULL,t);
	addExtFn("putc",NULL,t);
	addExtFn("puts",NULL,t);
	t=makeType(TB_INT);
	addExtFn("geti",NULL,t);
	t=makeType(TB_DOUBLE);
	addExtFn("getd",NULL,t);
	t=makeType(TB_CHAR);
	addExtFn("getc",NULL,t);
	}

static Symbol *findSymbolInList(Symbol *list,const char *name){
	for(Symbol *s=list;s;s=s->next){
		if(!strcmp(s->name,name))return s;
		}
	return NULL;
	}

static void addVarSymbol(Token *nameTk,Type type){
	if(currentStruct){
		if(findSymbolInList(currentStruct->structMembers,nameTk->text)){
			tkerrAt(nameTk,"simbol redefinit: %s",nameTk->text);
			}
		Symbol *var=newSymbol(nameTk->text,SK_VAR);
		var->type=type;
		var->owner=currentStruct;
		var->varIdx=symbolsLen(currentStruct->structMembers);
		addSymbolToList(&currentStruct->structMembers,var);
		return;
		}
	if(findSymbolInDomain(symTable,nameTk->text)){
		tkerrAt(nameTk,"simbol redefinit: %s",nameTk->text);
		}
	Symbol *var=newSymbol(nameTk->text,SK_VAR);
	var->type=type;
	var->owner=currentFn;
	if(currentFn){
		var->varIdx=symbolsLen(currentFn->fn.locals);
		addSymbolToList(&currentFn->fn.locals,dupSymbol(var));
		}
	else{
		var->varMem=calloc(1,typeSize(&type));
		}
	addSymbolToDomain(symTable,var);
	}

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
bool typeBase(Type *t){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(consume(TYPE_INT)){
		*t=makeType(TB_INT);
		return true;
		}
	if(consume(TYPE_DOUBLE)){
		*t=makeType(TB_DOUBLE);
		return true;
		}
	if(consume(TYPE_CHAR)){
		*t=makeType(TB_CHAR);
		return true;
		}
	if(consume(STRUCT)){
		if(consume(ID)){
			Symbol *s=findSymbol(consumedTk->text);
			if(!s || s->kind!=SK_STRUCT){
				tkerrAt(consumedTk,"structura nedefinita: %s",consumedTk->text);
				}
			*t=makeType(TB_STRUCT);
			t->s=s;
			return true;
			}
		tkerr("lipseste numele structurii dupa struct");
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

// arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl(Type *t){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(consume(LBRACKET)){
		t->n=0;
		if(consume(INT)){
			t->n=consumedTk->i;
			}
		else if(iTk->code!=RBRACKET){
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
bool typeName(Type *t){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(typeBase(t)){
		arrayDecl(t);
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
	Type type;
	if(typeBase(&type)){
		if(consume(ID)){
			Token *nameTk=consumedTk;
			arrayDecl(&type);
			if(findSymbolInDomain(symTable,nameTk->text)){
				tkerrAt(nameTk,"parametru redefinit: %s",nameTk->text);
				}
			Symbol *param=newSymbol(nameTk->text,SK_PARAM);
			param->type=type;
			param->owner=currentFn;
			param->paramIdx=symbolsLen(currentFn->fn.params);
			addSymbolToList(&currentFn->fn.params,dupSymbol(param));
			addSymbolToDomain(symTable,param);
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
	Type type;
	if(typeBase(&type)){
		if(consume(ID)){
			Token *nameTk=consumedTk;
			arrayDecl(&type);
			if(consume(SEMICOLON)){
				addVarSymbol(nameTk,type);
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
		pushDomain();
		for(;;){
			if(varDef()){}
			else if(stm()){}
			else break;
			}
		if(consume(RACC)){
			dropDomain();
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
	Type retType;
	if(typeBase(&retType) || (isVoid=consume(VOID))){
		if(isVoid){
			retType=makeType(TB_VOID);
			}
		if(consume(ID)){
			Token *nameTk=consumedTk;
			if(consume(LPAR)){
				if(findSymbolInDomain(symTable,nameTk->text)){
					tkerrAt(nameTk,"simbol redefinit: %s",nameTk->text);
					}
				Symbol *oldFn=currentFn;
				Symbol *fn=newSymbol(nameTk->text,SK_FN);
				fn->type=retType;
				addSymbolToDomain(symTable,fn);
				currentFn=fn;
				pushDomain();
				if(fnParam()){
					while(consume(COMMA)){
						if(!fnParam()){
							tkerr("parametru invalid dupa , in lista de parametri");
							}
						}
					}
				if(consume(RPAR)){
					if(stmCompound()){
						dropDomain();
						currentFn=oldFn;
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
			Token *nameTk=consumedTk;
			if(consume(LACC)){
				if(findSymbolInDomain(symTable,nameTk->text)){
					tkerrAt(nameTk,"simbol redefinit: %s",nameTk->text);
					}
				Symbol *oldStruct=currentStruct;
				Symbol *s=newSymbol(nameTk->text,SK_STRUCT);
				s->type=makeType(TB_STRUCT);
				s->type.s=s;
				addSymbolToDomain(symTable,s);
				currentStruct=s;
				while(varDef()){}
				if(consume(RACC)){
					if(consume(SEMICOLON)){
						currentStruct=oldStruct;
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
// transformare fara recursivitate la stanga:
//   exprOr: exprAnd exprOrPrim
//   exprOrPrim: OR exprAnd exprOrPrim | epsilon
bool exprOr(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprAnd()){
		return exprOrPrim();
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

bool exprOrPrim(){
	if(consume(OR)){
		if(exprAnd()){
			return exprOrPrim();
			}
		tkerr("lipseste o expresie valida dupa ||");
		}
	return true;
	}

// regula initiala:
//   exprAnd: exprAnd AND exprEq | exprEq
// transformare fara recursivitate la stanga:
//   exprAnd: exprEq exprAndPrim
//   exprAndPrim: AND exprEq exprAndPrim | epsilon
bool exprAnd(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprEq()){
		return exprAndPrim();
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

bool exprAndPrim(){
	if(consume(AND)){
		if(exprEq()){
			return exprAndPrim();
			}
		tkerr("lipseste o expresie valida dupa &&");
		}
	return true;
	}

// regula initiala:
//   exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel
// transformare fara recursivitate la stanga:
//   exprEq: exprRel exprEqPrim
//   exprEqPrim: EQUAL exprRel exprEqPrim | NOTEQ exprRel exprEqPrim | epsilon
bool exprEq(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprRel()){
		return exprEqPrim();
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

bool exprEqPrim(){
	if(consume(EQUAL)){
		if(exprRel()){
			return exprEqPrim();
			}
		tkerr("lipseste o expresie valida dupa ==");
		}
	if(consume(NOTEQ)){
		if(exprRel()){
			return exprEqPrim();
			}
		tkerr("lipseste o expresie valida dupa !=");
		}
	return true;
	}

// regula initiala:
//   exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd
// transformare fara recursivitate la stanga:
//   exprRel: exprAdd exprRelPrim
//   exprRelPrim: LESS exprAdd exprRelPrim | LESSEQ exprAdd exprRelPrim
//              | GREATER exprAdd exprRelPrim | GREATEREQ exprAdd exprRelPrim | epsilon
bool exprRel(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprAdd()){
		return exprRelPrim();
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

bool exprRelPrim(){
	if(consume(LESS)){
		if(exprAdd()){
			return exprRelPrim();
			}
		tkerr("lipseste o expresie valida dupa <");
		}
	if(consume(LESSEQ)){
		if(exprAdd()){
			return exprRelPrim();
			}
		tkerr("lipseste o expresie valida dupa <=");
		}
	if(consume(GREATER)){
		if(exprAdd()){
			return exprRelPrim();
			}
		tkerr("lipseste o expresie valida dupa >");
		}
	if(consume(GREATEREQ)){
		if(exprAdd()){
			return exprRelPrim();
			}
		tkerr("lipseste o expresie valida dupa >=");
		}
	return true;
	}

// regula initiala:
//   exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul
// transformare fara recursivitate la stanga:
//   exprAdd: exprMul exprAddPrim
//   exprAddPrim: ADD exprMul exprAddPrim | SUB exprMul exprAddPrim | epsilon
bool exprAdd(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprMul()){
		return exprAddPrim();
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

bool exprAddPrim(){
	if(consume(ADD)){
		if(exprMul()){
			return exprAddPrim();
			}
		tkerr("lipseste o expresie valida dupa +");
		}
	if(consume(SUB)){
		if(exprMul()){
			return exprAddPrim();
			}
		tkerr("lipseste o expresie valida dupa -");
		}
	return true;
	}

// regula initiala:
//   exprMul: exprMul ( MUL | DIV ) exprCast | exprCast
// transformare fara recursivitate la stanga:
//   exprMul: exprCast exprMulPrim
//   exprMulPrim: MUL exprCast exprMulPrim | DIV exprCast exprMulPrim | epsilon
bool exprMul(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprCast()){
		return exprMulPrim();
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

bool exprMulPrim(){
	if(consume(MUL)){
		if(exprCast()){
			return exprMulPrim();
			}
		tkerr("lipseste o expresie valida dupa *");
		}
	if(consume(DIV)){
		if(exprCast()){
			return exprMulPrim();
			}
		tkerr("lipseste o expresie valida dupa /");
		}
	return true;
	}

//exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(consume(LPAR)){
		Type type;
		if(typeName(&type)){
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
// transformare fara recursivitate la stanga:
//   exprPostfix: exprPrimary exprPostfixPrim
//   exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim
//                  | DOT ID exprPostfixPrim | epsilon
bool exprPostfix(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprPrimary()){
		return exprPostfixPrim();
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

bool exprPostfixPrim(){
	if(consume(LBRACKET)){
		if(expr()){
			if(consume(RBRACKET)){
				return exprPostfixPrim();
				}
			tkerr("index de tablou invalid sau lipseste ]");
			}
		tkerr("lipseste expresia dintre [ si ]");
		}
	if(consume(DOT)){
		if(consume(ID)){
			return exprPostfixPrim();
			}
		tkerr("lipseste numele campului dupa .");
		}
	return true;
	}

//exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
//| INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary(){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(consume(ID)){
		Token *nameTk=consumedTk;
		Symbol *s=findSymbol(nameTk->text);
		if(!s){
			tkerrAt(nameTk,"simbol nedefinit: %s",nameTk->text);
			}
		Token *afterId=iTk;
		Token *afterIdConsumed=consumedTk;
		if(consume(LPAR)){
			if(s->kind!=SK_FN){
				tkerrAt(nameTk,"%s nu este functie",nameTk->text);
				}
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
	pushDomain();
	addExtFns();
	unit();
	showDomain(symTable,"global");
	dropDomain();
	}
