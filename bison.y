%{
#include <stdio.h>
#include <stdlib.h>
#include "bison.tab.h"
// Declaraciones externas necesarias
extern int yylex();
extern int yyparse();
extern FILE *yyin;

void yyerror(const char *s);
%}

%token INTEGER BOOLEAN FLOAT ID
%token TOKEN_ERROR

%%

expression:
    INTEGER                          { printf("INTEGER\n"); }
    | BOOLEAN                          { printf("BOOLEAN\n"); }
    | FLOAT                            { printf("FLOAT\n"); }
    | ID                               { printf("ID\n"); }
    // | expression OP expression       { printf("suma\n"); }
    | '-' expression                 { printf("menos expresion\n"); }
    | '!' expression                  { printf("expresion negada\n"); }
    | '(' expression ')' { printf("expresion entre parentesis\n"); }
    ;
%%

void yyerror(const char *s) {
    fprintf(stderr, "Error sintactico: %s\n", s);
}

int main(int argc, char **argv) {
    // Verificamos si el usuario pasó el nombre del archivo como argumento
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo_de_entrada>\n", argv[0]);
        return 1;
    }

    // Abrimos el archivo en modo lectura
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror("Error al abrir el archivo");
        return 1;
    }

    // Ejecutamos el analizador
    yyparse();

    fclose(yyin); // Cerramos el archivo
    return 0;
}