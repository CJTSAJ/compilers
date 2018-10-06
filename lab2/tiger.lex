%{
/* Lab2 Attention: You are only allowed to add code in this file and start at Line 26.*/
#include <string.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

int charPos=1;

int yywrap(void)
{
 charPos=1;
 return 1;
}

void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}

/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/

int commentCount = 0;

/* @function: getstr
 * @input: a string literal
 * @output: the string value for the input which has all the escape sequences
 * translated into their meaning.
 */
char *getstr(const char *str)
{
  char *resultStr = (char*)malloc(yyleng);

  int index = 1;
  int index_ = 0;
  while(str[index] != '"'){
    if(str[index] == '\\'){
      index++;
      switch(str[index]){
        case 'n':{
          resultStr[index_] = '\n';
          break;
        }
        case 't':{
          resultStr[index_] = '\t';
          break;
        }
        default: break;
      }
    }else{
      resultStr[index_] = str[index];
    }
    index++;
    index_++;
  }

  if(index_ == 0) return NULL;
  resultStr[index_] = '\0';
	return resultStr;
}

%}
  /* You can add lex definitions here. */
space   " "|"\t"
letter  [A-Za-z]
digit   [0-9]
id      {letter}({digit}|{letter}|_)*

%start COMMENT

%%
  /*
  * Below is an example, which you can wipe out
  * and write reguler expressions and actions of your own.
  */

<INITIAL>"array" { adjust(); return ARRAY; }
<INITIAL>"if" { adjust(); return IF; }
<INITIAL>"then" { adjust(); return THEN; }
<INITIAL>"else" { adjust(); return ELSE; }
<INITIAL>"while" { adjust(); return WHILE; }
<INITIAL>"for" { adjust(); return FOR; }
<INITIAL>"to" { adjust(); return TO; }
<INITIAL>"do" { adjust(); return DO; }
<INITIAL>"let" { adjust(); return LET; }
<INITIAL>"in" { adjust(); return IN; }
<INITIAL>"end" { adjust(); return END; }
<INITIAL>"of" { adjust(); return OF; }
<INITIAL>"break" { adjust(); return BREAK; }
<INITIAL>"nil" { adjust(); return NIL; }
<INITIAL>"function" { adjust(); return FUNCTION; }
<INITIAL>"var" { adjust(); return VAR; }
<INITIAL>"type" { adjust(); return TYPE; }

<INITIAL>{space} { adjust(); continue;}

<INITIAL>"\n" {adjust(); EM_newline(); continue;}
<INITIAL>{id} { adjust(); yylval.sval=String(yytext); return ID; }
<INITIAL>{digit}+ { adjust(); yylval.ival=atoi(yytext); return INT; }
<INITIAL>\"[^\"]*\" { adjust(); yylval.sval=getstr(yytext); return STRING; }
<INITIAL>"," { adjust(); return COMMA; }
<INITIAL>":" { adjust(); return COLON; }
<INITIAL>";" { adjust(); return SEMICOLON; }
<INITIAL>"(" { adjust(); return LPAREN; }
<INITIAL>")" { adjust(); return RPAREN; }
<INITIAL>"[" { adjust(); return LBRACK; }
<INITIAL>"]" { adjust(); return RBRACK; }
<INITIAL>"{" { adjust(); return LBRACE; }
<INITIAL>"}" { adjust(); return RBRACE; }
<INITIAL>"." { adjust(); return DOT; }
<INITIAL>"+" { adjust(); return PLUS; }
<INITIAL>"-" { adjust(); return MINUS; }
<INITIAL>"*" { adjust(); return TIMES; }
<INITIAL>"/" { adjust(); return DIVIDE; }
<INITIAL>"=" { adjust(); return EQ; }
<INITIAL>"<>" { adjust(); return NEQ; }
<INITIAL>"<" { adjust(); return LT; }
<INITIAL>"<=" { adjust(); return LE; }
<INITIAL>">" { adjust(); return GT; }
<INITIAL>">=" { adjust(); return GE; }
<INITIAL>"&" { adjust(); return AND; }
<INITIAL>"|" { adjust(); return OR; }
<INITIAL>":=" { adjust(); return ASSIGN; }

<INITIAL>"/*" { adjust(); commentCount++; BEGIN COMMENT; }

<COMMENT>"/*" { adjust(); commentCount++; }
<COMMENT>"*/" { adjust(); commentCount--; if(commentCount == 0){BEGIN INITIAL;} }
<COMMENT>.|\n { adjust(); }
