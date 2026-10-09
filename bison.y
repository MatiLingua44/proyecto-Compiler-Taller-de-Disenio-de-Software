%{
#include <stdio.h>
#include "ast.h"

extern int yylex();

void yyerror(const char *s);

extern ASTNode *root;
%}

%locations

%union {
    int    entero;
    float  flotante;
    int    boolean;
    char   *texto;
    struct ASTNode *node;
}


%token TOKEN_ERROR

%token RETURN IF ELSE WHILE VOID
%token <entero> INTEGER <boolean> BOOLEAN <flotante> FLOAT <texto> ID
%token <texto> TYPE

%token SUMA RESTA MULTIPLICACION DIVISION MODULO
%token AND OR
%token MENOR MAYOR IGUALDAD

%type <node> program lines line
%type <node> method_decl
%type <node> parameters_list
%type <node> variable_declr_list
%type <node> block
%type <node> statements_list
%type <node> statement_or_decl
%type <node> statement
%type <node> variable_declr
%type <node> variable_list
%type <node> method_call
%type <node> expression_list
%type <node> expression

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
    /* vacio */ { root = NULL; }
    | lines //  { root = $1; }
    ;

lines:
    line          // { $$ = $1; }
    | lines line  // { $$ = create_seq_node($1, $2); }
    ;

line:
    variable_declr // { $$ = $1; }
    | method_decl  // { $$ = $1; }
    ;

method_decl:
    TYPE ID '(' parameters_list ')' block { printf("declaracion de metodo\n"); } // { $$ = create_method_decl_node($1, $2, $4, $6); }
    | VOID ID '(' parameters_list ')' block { printf("declaracion de metodo con retorno void\n"); } // { $$ = create_method_decl_node("void", $2, $4, $6); }
    ;

parameters_list:
    /* vacío */           { $$ = NULL; }
    | variable_declr_list { printf("lista de parametros\n"); }
    ;

variable_declr_list:
    TYPE ID                           { printf("1. Param: %s %s\n", $1, $2); } // { $$ = create_declaracion_node($1, $2);}
    | variable_declr_list ',' TYPE ID { printf("2. Param: %s %s\n", $3, $4); }
    ;

block:
    '{' statements_list '}' { printf("BLOQUE\n"); } // $$ = create_block_node($2);
    ;

statements_list:
    /* vacio */ { $$ = NULL; }
    | statements_list statement_or_decl
    ;

statement_or_decl:
    variable_declr // { $$ = $1; }
    | statement    // { $$ = $1; }
    ;

statement:
    ID '=' expression ';' { printf("ASIGNACION\n"); } // { $$ = create_asig_node($1, $3); }
    | method_call ';'   // { $$ = $1; }
    | IF '(' expression ')' block %prec LOWER_THAN_ELSE { printf("IF\n"); } // { $$ = create_if_node($2, $3, NULL); }
    | IF '(' expression ')' block ELSE block            { printf("IF/ELSE\n"); } // { $$ = create_if_node($2, $3, $5); }
    | WHILE '(' expression ')' block // { $$ = create_while_node($2, $3); }
    | RETURN expression ';' // { $$ = create_return_node($2); }
    | RETURN ';'            // { $$ = create_return_node(NULL); }
    | ';'                   { printf("sentencia\n"); }
    | block // { $$ = $1; }
    ;

variable_declr:
    TYPE variable_list ';' { printf("DECLARACION DE VARIABLE\n"); } // { $$ = create_variable_declr_node($1, $2); }
    ;

variable_list:
    ID                     { printf("DECLARACION DE VARIABLE SIMPLE\n"); } // { $$ = $1; }
    | variable_list ',' ID { printf("DECLARACION DE VARIABLE MULTIPLE\n"); }
    ;

method_call:
    ID '(' expression_list ')' { printf("LLAMADA A METODO\n"); } // { $$ = create_method_call_node($1, $3); }
    ;

expression_list:
    /* vacío */                      { $$ = NULL; }
    | expression                     { printf("LISTA DE EXPRESIONES\n"); } // { $$ = $1; }
    | expression_list ',' expression { printf("LISTA DE EXPRESIONES MULTIPLES\n"); }
    ;

expression:
    ID                             { printf("ID (%s)\n", $1); } // { $$ = create_id_node($1); }
    | method_call                  // { $$ = $1; }
    | INTEGER                      { printf("INTEGER (%d)\n", $1); } // { $$ = create_int_node($1); }
    | BOOLEAN                      { printf("BOOLEAN (%d)\n", $1); } // { $$ = create_boolean_node($1); }
    | FLOAT                        { printf("FLOAT (%f)\n", $1); } // { $$ = create_float_node($1); }

    | expression SUMA expression           { printf("suma\n"); } // { $$ = create_op_node(NODE_ADD, $1, $3); }
    | expression RESTA expression          { printf("resta\n"); } // { $$ = create_op_node(NODE_SUB, $1, $3); }
    | expression MULTIPLICACION expression { printf("multiplicacion\n"); } // { $$ = create_op_node(NODE_MUL, $1, $3); }
    | expression DIVISION expression       { printf("division\n"); } // { $$ = create_op_node(NODE_DIV, $1, $3); }
    | expression MODULO expression         { printf("modulo\n"); } // { $$ = create_op_node(NODE_MOD, $1, $3); }
    | expression AND expression            { printf("AND\n"); } // { $$ = create_op_node(NODE_AND, $1, $3); }
    | expression OR expression             { printf("OR\n"); } // { $$ = create_op_node(NODE_OR, $1, $3); }
    | expression MENOR expression          { printf("MENOR\n"); } // { $$ = create_op_node(NODE_MENOR, $1, $3); }
    | expression MAYOR expression          { printf("MAYOR\n"); } // { $$ = create_op_node(NODE_MAYOR, $1, $3); }
    | expression IGUALDAD expression       { printf("IGUALDAD\n"); } // { $$ = create_op_node(NODE_IGUALDAD, $1, $3); }

    | RESTA expression %prec MENOS_UNARIO { printf("menos expresion\n"); } // { $$ = create_unary_node(NODE_NEGATIVE, $1, $3); }
    | '!' expression                      { printf("expresion negada\n"); } // { $$ = create_unary_node(NODE_NEGATION, $1, $3); }
    | '(' expression ')'                  { printf("expresion entre parentesis\n"); } // { $$ = $2; }
    ;
%%

// Implementación de yyerror usando la variable global yylloc de Bison
void yyerror(const char *s) {
    fprintf(stderr, "Error Sintactico en la linea %d, columna %d: %s\n", 
            yylloc.first_line, yylloc.first_column, s);
}