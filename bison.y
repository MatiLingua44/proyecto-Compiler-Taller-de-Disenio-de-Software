%{
#include <stdio.h>
#include <stdlib.h>

// Declaraciones externas necesarias
extern int yylex();
extern int yyparse();
extern FILE *yyin;

void yyerror(const char *s);
%}

%token INTEGER BOOLEAN FLOAT ID
%token TYPE

%token SUMA RESTA MULTIPLICACION DIVISION MODULO
%token AND OR
%token MENOR MAYOR IGUALDAD



%left AND OR
%nonassoc MENOR MAYOR IGUALDAD
%left SUMA RESTA MULTIPLICACION DIVISION MODULO
%nonassoc MENOS_UNARIO

%%

variable_declr:
    TYPE variable_list ';' { printf("DECLARACION DE VARIABLE\n"); }
    ;

variable_list:
    ID                     { printf("DECLARACION DE VARIABLE SIMPLE\n"); }
    | variable_list ',' ID { printf("DECLARACION DE VARIABLE MULTIPLE\n"); }
    ;

/*
expression:
    INTEGER                          { printf("INTEGER\n"); }
    | BOOLEAN                        { printf("BOOLEAN\n"); }
    | FLOAT                          { printf("FLOAT\n"); }
    | ID                             { printf("ID\n"); }

    | expression SUMA expression     { printf("suma\n"); }
    | expression RESTA expression    { printf("resta\n"); }
    | expression MULTIPLICACION expression { printf("multiplicacion\n"); }
    | expression DIVISION expression       { printf("division\n"); }
    | expression MODULO expression         { printf("modulo\n"); }
    | expression AND expression      { printf("AND\n"); }
    | expression OR expression       { printf("OR\n"); }
    | expression MENOR expression    { printf("MENOR\n"); }
    | expression MAYOR expression    { printf("MAYOR\n"); }
    | expression IGUALDAD expression { printf("IGUALDAD\n"); }

    | RESTA expression %prec MENOS_UNARIO { printf("menos expresion\n"); }
    | '!' expression                      { printf("expresion negada\n"); }
    | '(' expression ')'                  { printf("expresion entre parentesis\n"); }
    ;
*/
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