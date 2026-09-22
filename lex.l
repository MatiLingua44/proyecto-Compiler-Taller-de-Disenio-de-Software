%{
#include <stdio.h>
#include "bison.tab.h"
%}
DIGIT [0-9]
ALPHA [a-zA-Z]
ALPHA_NUM {ALPHA}|{DIGIT}|"_"

INTEGER {DIGIT}+
BOOLEAN "true"|"false"
FLOAT {INTEGER}\.{INTEGER}
ID {ALPHA}{ALPHA_NUM}*

TYPE "int"|"bool"|"float"
RESERVED "return"|"if"|"while"|"else"|"void"|"main"

%option noyywrap


/* Reglas básicas de ejemplo */
%%
{RESERVED}              { printf("Palabra reservada: %s\n", yytext); }
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
.                       { printf("Lexema inesperado: %s\n", yytext); }
%%