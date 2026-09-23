%{
#include <stdio.h>
#include "bison.tab.h"

int commentary_counter = 0; /* Contador para comentarios anidados */
%}
DIGIT [0-9]
ALPHA [a-zA-Z]
ALPHA_NUM {ALPHA}|{DIGIT}|"_"

INTEGER {DIGIT}+
BOOLEAN "true"|"false"
FLOAT {INTEGER}\.{INTEGER}
ID {ALPHA}{ALPHA_NUM}*

TYPE "int"|"boolean"|"float"

%x COMENTARIO

%option yylineno
%option noyywrap

/* Reglas básicas de ejemplo */
%%
"return"                return RETURN;
"if"                    return IF;
"else"                  return ELSE;
"while"                 return WHILE;
"void"                  return VOID;

{TYPE}                  return TYPE;
{INTEGER}               return INTEGER;
{BOOLEAN}               return BOOLEAN;
{FLOAT}                 return FLOAT;
{ID}                    return ID;


"+"                     return SUMA;
"-"                     return RESTA;
"*"                     return MULTIPLICACION;
"/"                     return DIVISION;
"%"                     return MODULO;

"&&"                    return AND;
"||"                    return OR;

"<"                     return MENOR;
">"                     return MAYOR;
"=="                    return IGUALDAD;

"="                     return '=';
"!"                     return '!';


","                     return ',';
";"                     return ';';
"("                     return '(';
")"                     return ')';
"{"                     return '{';
"}"                     return '}';
"\n"                    { /* Ignorar saltos de línea */ }
[ \t]                   { /* Ignorar espacios vacios y tabulaciones */ }
.                       { printf("Lexema inesperado: %s\n", yytext); return TOKEN_ERROR; }

"//"[^\n]*\n            { printf("Comentario: %s", yytext); }

"/*"  {
        commentary_counter = 1;
        BEGIN(COMENTARIO);
      }

<COMENTARIO>"/*"    {
    commentary_counter++;
}

<COMENTARIO>"*/"    {
    commentary_counter--;
    if( commentary_counter == 0 ) {
        BEGIN(INITIAL);
    }
}

<COMENTARIO>\n  ;
<COMENTARIO>. ;

<COMENTARIO><<EOF>> {
    fprintf(stderr,
            "Error lexico:comentario sin Cerrar en linea %d\n",
            yylineno);
            return TOKEN_ERROR;
}
%%