%{
#include <stdio.h>
%}
DIGIT [0-9]
ALPHA [a-zA-Z]
ALPHA_NUM {ALPHA}|{DIGIT}|"_"

INTEGER {DIGIT}+
BOOLEAN "true"|"false"
FLOAT {INTEGER}\.{INTEGER}
ID {ALPHA}{ALPHA_NUM}*

ARIT_OP "+"|"-"|"*"|"/"|"%"
COND_OP "&&"|"||"
REL_OP "<"|">"|"=="
OP {ARIT_OP}|{COND_OP}|{REL_OP}

TYPE "int"|"bool"|"float"
RESERVED "return"|"if"|"while"|"else"|"void"|"main"
DELIMITER ";"|"("|")"|"{"|"}"


/* Reglas básicas de ejemplo */
%%
{RESERVED}              { printf("Palabra reservada: %s\n", yytext); }
{DELIMITER}             { printf("Delimitador: %s\n", yytext); }
{TYPE}                  { printf("Tipo de dato: %s\n", yytext); }
{INTEGER}               { printf("Numero: %s\n", yytext); }
{BOOLEAN}               { printf("Booleano: %s\n", yytext); }
{FLOAT}                 { printf("Flotante: %s\n", yytext); }
{ID}                    { printf("ID: %s\n", yytext); }
"="                     { printf("Asignacion: %s\n", yytext); }
"!"                     { printf("Negacion: %s\n", yytext); }
{OP}                    { printf("OP: %s\n", yytext); }
"\n"                    { /* Ignorar saltos de línea */ }
[ \t]                   { /* Ignorar espacios vacios y tabulaciones */ }
.                       { printf("Lexema inesperado: %s\n", yytext); }
%%

int yywrap() {
    return 1;
}

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