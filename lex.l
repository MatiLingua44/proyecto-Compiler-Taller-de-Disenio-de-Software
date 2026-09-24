%{
#include <stdio.h>
#include "bison.tab.h"

int commentary_counter = 0; /* Contador para comentarios anidados */
// Variable global para rastrear la columna actual
int current_column = 1;

// YY_USER_ACTION se ejecuta automáticamente antes de la acción de CUALQUIER token
#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = current_column; \
    yylloc.last_column = current_column + yyleng - 1; \
    current_column += yyleng;
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
%x COMENTARIO_LINEA

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
"\n"                    { current_column = 1; }
[ \t]                   { /* Ignorar espacios vacios y tabulaciones */ }
.                       { printf("Error Lexico Lexema inesperado: (%s) en linea %d, columna %d\n", yytext, yylineno, yylloc.first_column); return TOKEN_ERROR; }

"//"                    { BEGIN(COMENTARIO_LINEA); }
<COMENTARIO_LINEA>\n    { current_column = 1; BEGIN(INITIAL); }
<COMENTARIO_LINEA>.     ;
<COMENTARIO_LINEA><<EOF>> { BEGIN(INITIAL); }

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

<COMENTARIO>\n  { current_column = 1; };
<COMENTARIO>. ;

<COMENTARIO><<EOF>> {
    fprintf(stderr,
            "Error lexico:comentario sin Cerrar en linea %d\n",
            yylineno);
            return TOKEN_ERROR;
}
%%