#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"
#include "ad.h"
#include "utils.h"

Token *iTk;		// the iterator in the tokens list
Token *consumedTk;		// the last consumed token
Symbol *owner=NULL;		// the function or struct in which symbols are defined

bool arrayDecl(Type *t);
bool typeBase(Type *t);
bool typeName(Type *t);
bool fnParam();
bool fnDef();
bool stm();
bool stmCompound(bool newDomain);
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

static void addVarSymbol(Token *nameTk,Type type){
	Symbol *var=findSymbolInDomain(symTable,nameTk->text);
	if(var){
		tkerrAt(nameTk,"symbol redefinition: %s",nameTk->text);
		}
	var=newSymbol(nameTk->text,SK_VAR);
	var->type=type;
	var->owner=owner;
	addSymbolToDomain(symTable,var);
	if(owner){
		switch(owner->kind){
			case SK_FN:
				var->varIdx=symbolsLen(owner->fn.locals);
				addSymbolToList(&owner->fn.locals,dupSymbol(var));
				break;
			case SK_STRUCT:
				var->varIdx=typeSize(&owner->type);
				addSymbolToList(&owner->structMembers,dupSymbol(var));
				break;
			default:
				break;
			}
		}
	else{
		var->varMem=safeAlloc(typeSize(&type));
		}
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
	//printf("consume(%s)",tkCodeName(code));
	if(iTk->code==code){
		consumedTk=iTk;
		iTk=iTk->next;
		//printf(" => consumed\n");
		return true;
		}
	//printf(" => found %s\n",tkCodeName(iTk->code));
	return false;
	}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase(Type *t){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	t->n=-1;
	if(consume(TYPE_INT)){
		t->tb=TB_INT;
		t->s=NULL;
		return true;
		}
	if(consume(TYPE_DOUBLE)){
		t->tb=TB_DOUBLE;
		t->s=NULL;
		return true;
		}
	if(consume(TYPE_CHAR)){
		t->tb=TB_CHAR;
		t->s=NULL;
		return true;
		}
	if(consume(STRUCT)){
		if(consume(ID)){
			Token *tkName=consumedTk;
			Symbol *s=findSymbol(tkName->text);
			if(!s || s->kind!=SK_STRUCT){
				tkerrAt(tkName,"structura nedefinita: %s",tkName->text);
				}
			t->tb=TB_STRUCT;
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
		if(consume(INT)){
			Token *tkSize=consumedTk;
			t->n=tkSize->i;
			}
		else if(iTk->code==RBRACKET){
			t->n=0;
			}
		else{
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
	Type t;
	if(typeBase(&t)){
		if(consume(ID)){
			Token *tkName=consumedTk;
			if(arrayDecl(&t)){
				t.n=0;
				}
			Symbol *param=findSymbolInDomain(symTable,tkName->text);
			if(param){
				tkerrAt(tkName,"symbol redefinition: %s",tkName->text);
				}
			param=newSymbol(tkName->text,SK_PARAM);
			param->type=t;
			param->owner=owner;
			param->paramIdx=symbolsLen(owner->fn.params);
			addSymbolToDomain(symTable,param);
			addSymbolToList(&owner->fn.params,dupSymbol(param));
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
	Type t;
	if(typeBase(&t)){
		if(consume(ID)){
			Token *tkName=consumedTk;
			if(arrayDecl(&t) && t.n==0){
				tkerrAt(tkName,"a vector variable must have a specified dimension");
				}
			if(consume(SEMICOLON)){
				addVarSymbol(tkName,t);
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
bool stmCompound(bool newDomain){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(consume(LACC)){
		if(newDomain){
			pushDomain();
			}
		for(;;){
			if(varDef()){}
			else if(stm()){}
			else break;
			}
		if(consume(RACC)){
			if(newDomain){
				dropDomain();
				}
			return true;
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
	Type t;
	if(typeBase(&t) || (isVoid=consume(VOID))){
		if(isVoid){
			t=makeType(TB_VOID);
			}
		if(consume(ID)){
			Token *tkName=consumedTk;
			if(consume(LPAR)){
				Symbol *fn=findSymbolInDomain(symTable,tkName->text);
				if(fn){
					tkerrAt(tkName,"symbol redefinition: %s",tkName->text);
					}
				fn=newSymbol(tkName->text,SK_FN);
				fn->type=t;
				addSymbolToDomain(symTable,fn);
				owner=fn;
				pushDomain();
				if(fnParam()){
					while(consume(COMMA)){
						if(!fnParam()){
							tkerr("parametru invalid dupa , in lista de parametri sau lipseste");
							}
						}
					}
				if(consume(RPAR)){
					if(stmCompound(false)){
						dropDomain();
						owner=NULL;
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
			Token *tkName=consumedTk;
			if(consume(LACC)){
				Symbol *s=findSymbolInDomain(symTable,tkName->text);
				if(s){
					tkerrAt(tkName,"symbol redefinition: %s",tkName->text);
					}
				s=addSymbolToDomain(symTable,newSymbol(tkName->text,SK_STRUCT));
				s->type.tb=TB_STRUCT;
				s->type.s=s;
				s->type.n=-1;
				pushDomain();
				owner=s;
				while(varDef()){}
				if(consume(RACC)){
					if(consume(SEMICOLON)){
						owner=NULL;
						dropDomain();
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
	if(stmCompound(true)){
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
		Type t;
		if(typeName(&t)){
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
	owner=NULL;
	unit();
	}
