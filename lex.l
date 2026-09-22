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


/* Reglas básicas de ejemplo */
%%
{RESERVED}              { printf("Palabra reservada: %s\n", yytext); }
{TYPE}                  { printf("Tipo de dato: %s\n", yytext); }
{INTEGER}               return INTEGER; // { printf("Numero: %s\n", yytext); }
{BOOLEAN}               return BOOLEAN; // { printf("Booleano: %s\n", yytext); }
{FLOAT}                 return FLOAT; // { printf("Flotante: %s\n", yytext); }
{ID}                    return ID; // { printf("ID: %s\n", yytext); }


"+"                     return '+';
"-"                     return '-';
"*"                     return '*';
"/"                     return '/';
"%"                     return '%';

"&&"                    return "&&";
"||"                    return "||";

"<"                     return '<';
">"                     return '>';
"=="                    return "==";

"="                     return '='; // { printf("Asignacion: %s\n", yytext); }
"!"                     return '!'; // { printf("Negacion: %s\n", yytext); }


";"                     return ';';
"("                     return '(';
")"                     return ')';
"{"                     return '{';
"}"                     return '}';
"\n"                    { /* Ignorar saltos de línea */ }
[ \t]                   { /* Ignorar espacios vacios y tabulaciones */ }
.                       { printf("Lexema inesperado: %s\n", yytext); }
%%


int yywrap() {
    return 1;
}

/*
int main(int argc, char *argv[]) {
    // Verificar si se pasó el nombre del archivo como argumento
    if (argc < 2) {
        printf("Uso: %s <archivo_de_entrada>\n", argv[0]);
        return 1;
    }

    // Abrir el archivo en modo lectura
    FILE *archivo = fopen(argv[1], "r");
    if (!archivo) {
        printf("Error: No se pudo abrir el archivo %s\n", argv[1]);
        return 1;
    }

    // Redireccionar la entrada de Flex al archivo
    yyin = archivo;

    // Iniciar el análisis léxico
    yylex();

    // Cerrar el archivo al terminar
    fclose(archivo);
    return 0;
}
*/