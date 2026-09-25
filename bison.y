%{
#include <stdio.h>
#include <stdlib.h>

extern int yylex();

void yyerror(const char *s);
%}

%locations

%union {
    int   entero;
    float flotante;
    int   boolean;
    char  *texto;
}


%token TOKEN_ERROR

%token RETURN IF ELSE WHILE VOID
%token <entero> INTEGER <boolean> BOOLEAN <flotante> FLOAT <texto> ID
%token <texto> TYPE

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
    | IF '(' expression ')' block %prec LOWER_THAN_ELSE { printf("IF\n"); }
    | IF '(' expression ')' block ELSE block            { printf("IF/ELSE\n"); }
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
    ID                             { printf("ID (%s)\n", $1); }
    | method_call
    | INTEGER                      { printf("INTEGER (%d)\n", $1); }
    | BOOLEAN                      { printf("BOOLEAN (%d)\n", $1); }
    | FLOAT                        { printf("FLOAT (%f)\n", $1); }

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

// Implementación de yyerror usando la variable global yylloc de Bison
void yyerror(const char *s) {
    fprintf(stderr, "Error Sintactico en la linea %d, columna %d: %s\n", 
            yylloc.first_line, yylloc.first_column, s);
}