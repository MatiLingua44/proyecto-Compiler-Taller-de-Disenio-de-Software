%{
#include <stdio.h>
#include <stdlib.h>

extern int yylex();
extern int yyparse();
extern FILE *yyin;

void yyerror(const char *s);
%}

%token TOKEN_ERROR

%token RETURN IF ELSE WHILE VOID
%token INTEGER BOOLEAN FLOAT ID
%token TYPE

%token SUMA RESTA MULTIPLICACION DIVISION MODULO
%token AND OR
%token MENOR MAYOR IGUALDAD

/* --- PRECEDENCIAS (de menor a mayor) --- */
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%left OR
%left AND
%nonassoc MENOR MAYOR IGUALDAD
%left SUMA RESTA
%left MULTIPLICACION DIVISION MODULO
%nonassoc '!' MENOS_UNARIO

%%

program:
    /* vacío */
    | lines
    ;

lines:
    line
    | lines line
    ;

line:
    variable_declr
    | method_decl
    ;

method_decl:
    TYPE ID '(' parameters_list ')' block { printf("declaracion de metodo\n"); }
    | VOID ID '(' parameters_list ')' block { printf("declaracion de metodo con retorno void\n"); }
    ;

parameters_list:
    /* vacío */
    | variable_declr_list { printf("lista de parametros\n"); }
    ;

variable_declr_list:
    TYPE ID
    | variable_declr_list ',' TYPE ID
    ;

block:
    '{' statements_list '}' { printf("BLOQUE\n"); }
    ;

statements_list:
    /* vacío */
    | statements_list statement_or_decl
    ;

statement_or_decl:
    variable_declr
    | statement
    ;

statement:
    ID '=' expression ';' { printf("ASIGNACION\n"); }
    | method_call ';'
    | IF '(' expression ')' block %prec LOWER_THAN_ELSE
    | IF '(' expression ')' block ELSE block
    | WHILE '(' expression ')' block
    | RETURN expression ';'
    | RETURN ';'
    | ';'
    | block
    ;

variable_declr:
    TYPE variable_list ';' { printf("DECLARACION DE VARIABLE\n"); }
    ;

variable_list:
    ID                     { printf("DECLARACION DE VARIABLE SIMPLE\n"); }
    | variable_list ',' ID { printf("DECLARACION DE VARIABLE MULTIPLE\n"); }
    ;

method_call:
    ID '(' expression_list ')' { printf("LLAMADA A METODO\n"); }
    ;

expression_list:
    /* vacío */
    | expression               { printf("LISTA DE EXPRESIONES\n"); }
    | expression_list ',' expression { printf("LISTA DE EXPRESIONES MULTIPLES\n"); }
    ;

expression:
    ID                             { printf("ID\n"); }
    | method_call
    | INTEGER                      { printf("INTEGER\n"); }
    | BOOLEAN                      { printf("BOOLEAN\n"); }
    | FLOAT                        { printf("FLOAT\n"); }

    | expression SUMA expression           { printf("suma\n"); }
    | expression RESTA expression          { printf("resta\n"); }
    | expression MULTIPLICACION expression { printf("multiplicacion\n"); }
    | expression DIVISION expression       { printf("division\n"); }
    | expression MODULO expression         { printf("modulo\n"); }
    | expression AND expression            { printf("AND\n"); }
    | expression OR expression             { printf("OR\n"); }
    | expression MENOR expression          { printf("MENOR\n"); }
    | expression MAYOR expression          { printf("MAYOR\n"); }
    | expression IGUALDAD expression       { printf("IGUALDAD\n"); }

    | RESTA expression %prec MENOS_UNARIO { printf("menos expresion\n"); }
    | '!' expression                      { printf("expresion negada\n"); }
    | '(' expression ')'                  { printf("expresion entre parentesis\n"); }
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