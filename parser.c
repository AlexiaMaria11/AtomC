#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"
#include "ad.h"
#include "at.h"
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
bool expr(Ret *r);
bool exprAssign(Ret *r);
bool exprOr(Ret *r);
bool exprOrPrim(Ret *r);
bool exprAnd(Ret *r);
bool exprAndPrim(Ret *r);
bool exprEq(Ret *r);
bool exprEqPrim(Ret *r);
bool exprRel(Ret *r);
bool exprRelPrim(Ret *r);
bool exprAdd(Ret *r);
bool exprAddPrim(Ret *r);
bool exprMul(Ret *r);
bool exprMulPrim(Ret *r);
bool exprCast(Ret *r);
bool exprUnary(Ret *r);
bool exprPostfix(Ret *r);
bool exprPostfixPrim(Ret *r);
bool exprPrimary(Ret *r);
void tkerrAt(const Token *tk,const char *fmt,...);

static Type makeType(TypeBase tb){
	Type t;
	t.tb=tb;
	t.s=NULL;
	t.n=-1;
	return t;
	}

static Ret makeRet(Type type,bool lval,bool ct){
	Ret r;
	r.type=type;
	r.lval=lval;
	r.ct=ct;
	return r;
	}

static Ret makeBaseRet(TypeBase tb,bool lval,bool ct){
	return makeRet(makeType(tb),lval,ct);
	}

static bool startsTypeName(Token *tk){
	if(!tk)return false;
	switch(tk->code){
		case TYPE_INT:
		case TYPE_DOUBLE:
		case TYPE_CHAR:
		case STRUCT:
			return true;
		default:
			return false;
		}
	}

static bool startsWithTypeCast(){
	return iTk->code==LPAR && startsTypeName(iTk->next);
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
	fprintf(stderr,"error at line %d: ",errLine(iTk));
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
	}

void tkerrAt(const Token *tk,const char *fmt,...){
	fprintf(stderr,"error at line %d: ",errLine(tk));
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
				tkerrAt(tkName,"undefined struct: %s",tkName->text);
				}
			t->tb=TB_STRUCT;
			t->s=s;
			return true;
			}
		tkerr("missing structure name after struct");
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
			tkerr("array size must be an integer constant or empty");
			}
		if(consume(RBRACKET)){
			return true;
			}
		tkerr("missing ] in array declaration");
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
		tkerr("missing parameter name after type");
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
			tkerrAt(consumedTk,"missing ; after variable declaration");
			}
		tkerr("missing variable name after type");
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
		tkerr("invalid statement in block or missing }");
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
							tkerr("invalid or missing parameter after , in parameter list");
							}
						}
					}
				if(consume(RPAR)){
					if(stmCompound(false)){
						dropDomain();
						owner=NULL;
						return true;
						}
					tkerr("missing function body after header");
					}
				tkerrAt(consumedTk,"invalid parameter list or missing )");
				}
			if(isVoid){
				tkerrAt(consumedTk,"void type can only be used for functions; missing ( after function name");
				}
			iTk=start;
			consumedTk=startConsumed;
			return false;
			}
		if(isVoid){
			tkerr("missing function name after void");
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
					tkerrAt(consumedTk,"missing ; after structure definition");
					}
				if(iTk->code==END){
					tkerrAt(consumedTk,"missing } at the end of the structure definition");
					}
				tkerr("invalid declaration in structure or missing }");
				}
			iTk=start;
			consumedTk=startConsumed;
			return false;
			}
		tkerr("missing structure name after struct");
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
			Ret rCond;
			if(expr(&rCond)){
				if(!canBeScalar(&rCond)){
					tkerr("the if condition must be a scalar value");
					}
				if(consume(RPAR)){
					if(stm()){
						if(consume(ELSE)){
							if(stm()){
								return true;
								}
							tkerr("missing statement after else");
							}
						return true;
						}
					tkerr("missing statement after if condition");
					}
				tkerr("invalid if condition or missing )");
				}
			tkerr("missing if condition");
			}
		tkerr("missing ( after if");
		}

	iTk=start;
	consumedTk=startConsumed;
	if(consume(WHILE)){
		if(consume(LPAR)){
			Ret rCond;
			if(expr(&rCond)){
				if(!canBeScalar(&rCond)){
					tkerr("the while condition must be a scalar value");
					}
				if(consume(RPAR)){
					if(stm()){
						return true;
						}
					tkerr("missing statement after while condition");
					}
				tkerr("invalid while condition or missing )");
				}
			tkerr("missing while condition");
			}
		tkerr("missing ( after while");
		}

	iTk=start;
	consumedTk=startConsumed;
	if(consume(RETURN)){
		Ret rExpr;
		if(expr(&rExpr)){
			if(owner->type.tb==TB_VOID){
				tkerr("a void function cannot return a value");
				}
			if(!canBeScalar(&rExpr)){
				tkerr("the return value must be a scalar value");
				}
			if(!convTo(&rExpr.type,&owner->type)){
				tkerr("cannot convert the return expression type to the function return type");
				}
			}
		else if(owner->type.tb!=TB_VOID){
			tkerr("a non-void function must return a value");
			}
		if(consume(SEMICOLON)){
			return true;
			}
		tkerrAt(consumedTk,"missing ; after return");
		}

	iTk=start;
	consumedTk=startConsumed;
	Ret rExpr;
	if(expr(&rExpr)){
		if(consume(SEMICOLON)){
			return true;
			}
		tkerrAt(consumedTk,"invalid expression statement or missing ;");
		}
	if(consume(SEMICOLON)){
		return true;
		}

	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

//expr: exprAssign
bool expr(Ret *r){
	return exprAssign(r);
	}

//exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign(Ret *r){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	Ret rDst;
	if(!startsWithTypeCast() && exprUnary(&rDst)){
		if(consume(ASSIGN)){
			if(exprAssign(r)){
				if(!rDst.lval){
					tkerr("the assign destination must be a left-value");
					}
				if(rDst.ct){
					tkerr("the assign destination cannot be constant");
					}
				if(!canBeScalar(&rDst)){
					tkerr("the assign destination must be scalar");
					}
				if(!canBeScalar(r)){
					tkerr("the assign source must be scalar");
					}
				if(!convTo(&r->type,&rDst.type)){
					tkerr("the assign source cannot be converted to destination");
					}
				r->lval=false;
				r->ct=true;
				return true;
				}
			tkerr("missing expression on the right side of =");
			}
		}
	iTk=start;
	consumedTk=startConsumed;
	return exprOr(r);
	}

// regula initiala:
//   exprOr: exprOr OR exprAnd | exprAnd
// transformare fara recursivitate la stanga:
//   exprOr: exprAnd exprOrPrim
//   exprOrPrim: OR exprAnd exprOrPrim | epsilon
bool exprOr(Ret *r){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprAnd(r)){
		return exprOrPrim(r);
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

bool exprOrPrim(Ret *r){
	if(consume(OR)){
		Ret right;
		if(exprAnd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)){
				tkerr("invalid operand type for ||");
				}
			*r=makeBaseRet(TB_INT,false,true);
			return exprOrPrim(r);
			}
		tkerr("missing valid expression after ||");
		}
	return true;
	}

// regula initiala:
//   exprAnd: exprAnd AND exprEq | exprEq
// transformare fara recursivitate la stanga:
//   exprAnd: exprEq exprAndPrim
//   exprAndPrim: AND exprEq exprAndPrim | epsilon
bool exprAnd(Ret *r){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprEq(r)){
		return exprAndPrim(r);
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

bool exprAndPrim(Ret *r){
	if(consume(AND)){
		Ret right;
		if(exprEq(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)){
				tkerr("invalid operand type for &&");
				}
			*r=makeBaseRet(TB_INT,false,true);
			return exprAndPrim(r);
			}
		tkerr("missing valid expression after &&");
		}
	return true;
	}

// regula initiala:
//   exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel
// transformare fara recursivitate la stanga:
//   exprEq: exprRel exprEqPrim
//   exprEqPrim: EQUAL exprRel exprEqPrim | NOTEQ exprRel exprEqPrim | epsilon
bool exprEq(Ret *r){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprRel(r)){
		return exprEqPrim(r);
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

bool exprEqPrim(Ret *r){
	if(consume(EQUAL)){
		Ret right;
		if(exprRel(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)){
				tkerr("invalid operand type for ==");
				}
			*r=makeBaseRet(TB_INT,false,true);
			return exprEqPrim(r);
			}
		tkerr("missing valid expression after ==");
		}
	if(consume(NOTEQ)){
		Ret right;
		if(exprRel(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)){
				tkerr("invalid operand type for !=");
				}
			*r=makeBaseRet(TB_INT,false,true);
			return exprEqPrim(r);
			}
		tkerr("missing valid expression after !=");
		}
	return true;
	}

// regula initiala:
//   exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd
// transformare fara recursivitate la stanga:
//   exprRel: exprAdd exprRelPrim
//   exprRelPrim: LESS exprAdd exprRelPrim | LESSEQ exprAdd exprRelPrim
//              | GREATER exprAdd exprRelPrim | GREATEREQ exprAdd exprRelPrim | epsilon
bool exprRel(Ret *r){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprAdd(r)){
		return exprRelPrim(r);
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

bool exprRelPrim(Ret *r){
	if(consume(LESS)){
		Ret right;
		if(exprAdd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)){
				tkerr("invalid operand type for <");
				}
			*r=makeBaseRet(TB_INT,false,true);
			return exprRelPrim(r);
			}
		tkerr("missing valid expression after <");
		}
	if(consume(LESSEQ)){
		Ret right;
		if(exprAdd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)){
				tkerr("invalid operand type for <=");
				}
			*r=makeBaseRet(TB_INT,false,true);
			return exprRelPrim(r);
			}
		tkerr("missing valid expression after <=");
		}
	if(consume(GREATER)){
		Ret right;
		if(exprAdd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)){
				tkerr("invalid operand type for >");
				}
			*r=makeBaseRet(TB_INT,false,true);
			return exprRelPrim(r);
			}
		tkerr("missing valid expression after >");
		}
	if(consume(GREATEREQ)){
		Ret right;
		if(exprAdd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)){
				tkerr("invalid operand type for >=");
				}
			*r=makeBaseRet(TB_INT,false,true);
			return exprRelPrim(r);
			}
		tkerr("missing valid expression after >=");
		}
	return true;
	}

// regula initiala:
//   exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul
// transformare fara recursivitate la stanga:
//   exprAdd: exprMul exprAddPrim
//   exprAddPrim: ADD exprMul exprAddPrim | SUB exprMul exprAddPrim | epsilon
bool exprAdd(Ret *r){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprMul(r)){
		return exprAddPrim(r);
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

bool exprAddPrim(Ret *r){
	if(consume(ADD)){
		Ret right;
		if(exprMul(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)){
				tkerr("invalid operand type for +");
				}
			*r=makeRet(tDst,false,true);
			return exprAddPrim(r);
			}
		tkerr("missing valid expression after +");
		}
	if(consume(SUB)){
		Ret right;
		if(exprMul(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)){
				tkerr("invalid operand type for -");
				}
			*r=makeRet(tDst,false,true);
			return exprAddPrim(r);
			}
		tkerr("missing valid expression after -");
		}
	return true;
	}

// regula initiala:
//   exprMul: exprMul ( MUL | DIV ) exprCast | exprCast
// transformare fara recursivitate la stanga:
//   exprMul: exprCast exprMulPrim
//   exprMulPrim: MUL exprCast exprMulPrim | DIV exprCast exprMulPrim | epsilon
bool exprMul(Ret *r){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprCast(r)){
		return exprMulPrim(r);
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

bool exprMulPrim(Ret *r){
	if(consume(MUL)){
		Ret right;
		if(exprCast(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)){
				tkerr("invalid operand type for *");
				}
			*r=makeRet(tDst,false,true);
			return exprMulPrim(r);
			}
		tkerr("missing valid expression after *");
		}
	if(consume(DIV)){
		Ret right;
		if(exprCast(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)){
				tkerr("invalid operand type for /");
				}
			*r=makeRet(tDst,false,true);
			return exprMulPrim(r);
			}
		tkerr("missing valid expression after /");
		}
	return true;
	}

//exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast(Ret *r){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(consume(LPAR)){
		Type t;
		if(typeName(&t)){
			if(consume(RPAR)){
				Ret op;
				if(exprCast(&op)){
					if(t.tb==TB_STRUCT){
						tkerr("cannot convert to a struct type");
						}
					if(op.type.tb==TB_STRUCT){
						tkerr("cannot convert a struct");
						}
					if(op.type.n>=0 && t.n<0){
						tkerr("an array can be converted only to another array");
						}
					if(op.type.n<0 && t.n>=0){
						tkerr("a scalar can be converted only to another scalar");
						}
					*r=makeRet(t,false,true);
					return true;
					}
				tkerr("missing expression after type cast");
				}
			tkerr("invalid type cast or missing )");
			}
		}
	iTk=start;
	consumedTk=startConsumed;
	return exprUnary(r);
	}

//exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary(Ret *r){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(consume(SUB)){
		if(exprUnary(r)){
			if(!canBeScalar(r)){
				tkerr("unary - must have a scalar operand");
				}
			r->lval=false;
			r->ct=true;
			return true;
			}
		tkerr("missing expression after -");
		}
	if(consume(NOT)){
		if(exprUnary(r)){
			if(!canBeScalar(r)){
				tkerr("unary ! must have a scalar operand");
				}
			r->type=makeType(TB_INT);
			r->lval=false;
			r->ct=true;
			return true;
			}
		tkerr("missing expression after !");
		}
	iTk=start;
	consumedTk=startConsumed;
	return exprPostfix(r);
	}

// regula initiala:
//   exprPostfix: exprPostfix LBRACKET expr RBRACKET | exprPostfix DOT ID | exprPrimary
// transformare fara recursivitate la stanga:
//   exprPostfix: exprPrimary exprPostfixPrim
//   exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim
//                  | DOT ID exprPostfixPrim | epsilon
bool exprPostfix(Ret *r){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(exprPrimary(r)){
		return exprPostfixPrim(r);
		}
	iTk=start;
	consumedTk=startConsumed;
	return false;
	}

bool exprPostfixPrim(Ret *r){
	if(consume(LBRACKET)){
		Ret idx;
		if(expr(&idx)){
			if(consume(RBRACKET)){
				if(r->type.n<0){
					tkerr("only an array can be indexed");
					}
				Type tInt=makeType(TB_INT);
				if(!convTo(&idx.type,&tInt)){
					tkerr("the index is not convertible to int");
					}
				r->type.n=-1;
				r->lval=true;
				r->ct=false;
				return exprPostfixPrim(r);
				}
			tkerr("invalid array index or missing ]");
			}
		tkerr("missing expression between [ and ]");
		}
	if(consume(DOT)){
		if(consume(ID)){
			Token *tkName=consumedTk;
			if(r->type.tb!=TB_STRUCT || r->type.n>=0){
				tkerr("a field can only be selected from a struct");
				}
			Symbol *s=findSymbolInList(r->type.s->structMembers,tkName->text);
			if(!s){
				tkerr("the structure %s does not have a field %s",r->type.s->name,tkName->text);
				}
			*r=makeRet(s->type,true,s->type.n>=0);
			return exprPostfixPrim(r);
			}
		tkerr("missing field name after .");
		}
	return true;
	}

//exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
//| INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary(Ret *r){
	Token *start=iTk;
	Token *startConsumed=consumedTk;
	if(consume(ID)){
		Token *tkName=consumedTk;
		Symbol *s=findSymbol(tkName->text);
		if(!s){
			tkerr("undefined id: %s",tkName->text);
			}
		if(consume(LPAR)){
			if(s->kind!=SK_FN){
				tkerr("only a function can be called");
				}
			Ret rArg;
			Symbol *param=s->fn.params;
			if(expr(&rArg)){
				if(!param){
					tkerr("too many arguments in function call");
					}
				if(!convTo(&rArg.type,&param->type)){
					tkerr("in call, cannot convert the argument type to the parameter type");
					}
				param=param->next;
				while(consume(COMMA)){
					if(!expr(&rArg)){
						tkerr("invalid argument after ,");
						}
					if(!param){
						tkerr("too many arguments in function call");
						}
					if(!convTo(&rArg.type,&param->type)){
						tkerr("in call, cannot convert the argument type to the parameter type");
						}
					param=param->next;
					}
				if(consume(RPAR)){
					if(param){
						tkerr("too few arguments in function call");
						}
					*r=makeRet(s->type,false,true);
					return true;
					}
				tkerrAt(consumedTk,"invalid argument list or missing )");
				}
			else if(consume(RPAR)){
				if(param){
					tkerr("too few arguments in function call");
					}
				*r=makeRet(s->type,false,true);
				return true;
				}
			else{
				tkerrAt(consumedTk,"invalid function call or missing )");
				}
			}
		if(s->kind==SK_FN){
			tkerr("a function can only be called");
			}
		*r=makeRet(s->type,true,s->type.n>=0);
		return true;
		}

	iTk=start;
	consumedTk=startConsumed;
	if(consume(INT)){
		*r=makeBaseRet(TB_INT,false,true);
		return true;
		}
	if(consume(DOUBLE)){
		*r=makeBaseRet(TB_DOUBLE,false,true);
		return true;
		}
	if(consume(CHAR)){
		*r=makeBaseRet(TB_CHAR,false,true);
		return true;
		}
	if(consume(STRING)){
		Type t=makeType(TB_CHAR);
		t.n=0;
		*r=makeRet(t,false,true);
		return true;
		}

	iTk=start;
	consumedTk=startConsumed;
	if(consume(LPAR)){
		if(expr(r)){
			if(consume(RPAR)){
				return true;
				}
			tkerrAt(consumedTk,"invalid parenthesized expression or missing )");
			}
		tkerr("missing expression after (");
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
	tkerr("syntax error");
	return false;
	}

void parse(Token *tokens){
	iTk=tokens;
	consumedTk=NULL;
	owner=NULL;
	unit();
	}
