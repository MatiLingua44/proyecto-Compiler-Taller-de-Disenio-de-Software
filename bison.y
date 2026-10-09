%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tabla_simbolos.h"
#include "ast.h"
#include "semantica.h"

extern int yylex(void);
extern int yylineno;
void yyerror(const char *s);

// Puntero global a la tabla de símbolos
extern TablaSimbolos *tabla_simbolos;

// Puntero al método actual que se está analizando
static Simbolo *metodo_actual = NULL;
%}

%code requires {
    #include "tabla_simbolos.h"
    #include "ast.h"
}

%locations

%union {
    int          entero;
    float        flotante;
    int          boolean;
    char        *texto;
    NodoAST     *nodo;
    Simbolo     *simbolo;
    ListaIDs    *lista_ids;
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

/* --- TIPOS DE NO-TERMINALES --- */
%type <simbolo> method_header method_decl
%type <nodo> function_block block statement_or_decl statements_list statement
%type <nodo> expression method_call expression_list expression_list_items
%type <lista_ids> variable_list

%%

program:
    /* vacío */
    | lines {
        // Regla 3: Verificar que el programa contiene la definición de main()
        verificar_programa(tabla_simbolos);
    }
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
    method_header '(' parameters_list ')' function_block {
        if ($1 != NULL) {
            $1->ast = $5;
        }
        CerrarNivel(tabla_simbolos);
        metodo_actual = NULL;
        $$ = $1;
    }
    ;

method_header:
    TYPE ID {
        TipoDato td = StringATipoDato($1);
        free($1);
        Simbolo *s = InsertarSimbolo(tabla_simbolos, $2, FLAG_FUNCION, td, @2.first_line, @2.first_column);
        AbrirNivel(tabla_simbolos); // Nivel 2: parámetros y variables del método
        metodo_actual = s;
        free($2);
        $$ = s;
    }
    | VOID ID {
        Simbolo *s = InsertarSimbolo(tabla_simbolos, $2, FLAG_FUNCION, TIPO_VOID, @2.first_line, @2.first_column);
        AbrirNivel(tabla_simbolos); // Nivel 2: parámetros y variables del método
        metodo_actual = s;
        free($2);
        $$ = s;
    }
    ;

parameters_list:
    /* vacío */
    | variable_declr_list
    ;

variable_declr_list:
    param_item
    | variable_declr_list ',' param_item
    ;

param_item:
    TYPE ID {
        TipoDato td = StringATipoDato($1);
        free($1);
        if (metodo_actual != NULL) {
            InsertarSimbolo(tabla_simbolos, $2, FLAG_PARAMETRO, td, @2.first_line, @2.first_column);
            AgregarParametroAFuncion(metodo_actual, $2, td);
        }
        free($2);
    }
    ;

// Bloque principal de la función (el nivel ya fue abierto por method_header)
function_block:
    '{' statements_list '}' {
        $$ = $2;
    }
    ;

// Bloque interno o bloque de control (abre y cierra un nuevo nivel local)
block:
    '{' { AbrirNivel(tabla_simbolos); } statements_list '}' {
        CerrarNivel(tabla_simbolos);
        $$ = $3;
    }
    ;

statements_list:
    /* vacío */ {
        $$ = NULL;
    }
    | statements_list statement_or_decl {
        if ($2 != NULL) {
            $$ = concatenar_sentencias($1, $2);
        } else {
            $$ = $1;
        }
    }
    ;

statement_or_decl:
    variable_declr {
        $$ = NULL; // Las declaraciones se registran en la TS pero no generan nodos de sentencia en el AST
    }
    | statement {
        $$ = $1;
    }
    ;

statement:
    ID '=' expression ';' {
        Simbolo *s = BuscarSimbolo(tabla_simbolos, $1);
        if (s == NULL) {
            sem_error(@1.first_line, @1.first_column, "Regla 2: Variable '%s' no declarada antes de su uso.", $1);
        } else if (s->flag == FLAG_FUNCION) {
            sem_error(@1.first_line, @1.first_column, "Regla 8: No se puede asignar al identificador '%s' porque es un metodo.", $1);
        }
        NodoAST *var_nodo = crear_nodo_var(s, @1.first_line, @1.first_column);
        $$ = crear_nodo_asignacion(var_nodo, $3, @2.first_line, @2.first_column);
        free($1);
    }
    | method_call ';' {
        $$ = $1;
    }
    | IF '(' expression ')' block %prec LOWER_THAN_ELSE {
        $$ = crear_nodo_if($3, $5, NULL, @1.first_line, @1.first_column);
    }
    | IF '(' expression ')' block ELSE block {
        $$ = crear_nodo_if($3, $5, $7, @1.first_line, @1.first_column);
    }
    | WHILE expression block {
        $$ = crear_nodo_while($2, $3, @1.first_line, @1.first_column);
    }
    | RETURN expression ';' {
        $$ = crear_nodo_return($2, @1.first_line, @1.first_column);
    }
    | RETURN ';' {
        $$ = crear_nodo_return(NULL, @1.first_line, @1.first_column);
    }
    | ';' {
        $$ = NULL;
    }
    | block {
        $$ = $1;
    }
    ;

variable_declr:
    TYPE variable_list ';' {
        TipoDato td = StringATipoDato($1);
        free($1);
        NodoID *curr = $2->primero;
        while (curr != NULL) {
            InsertarSimbolo(tabla_simbolos, curr->nombre, FLAG_VARIABLE, td, curr->linea, curr->columna);
            curr = curr->sig;
        }
        liberar_lista_ids($2);
    }
    ;

variable_list:
    ID {
        $$ = crear_lista_ids($1, @1.first_line, @1.first_column);
        free($1);
    }
    | variable_list ',' ID {
        $$ = agregar_id_a_lista($1, $3, @3.first_line, @3.first_column);
        free($3);
    }
    ;

method_call:
    ID '(' expression_list ')' {
        Simbolo *s = BuscarSimbolo(tabla_simbolos, $1);
        if (s == NULL) {
            sem_error(@1.first_line, @1.first_column, "Regla 2: Metodo '%s' no declarado antes de su uso.", $1);
        } else if (s->flag != FLAG_FUNCION) {
            sem_error(@1.first_line, @1.first_column, "El identificador '%s' no es un metodo.", $1);
        }
        $$ = crear_nodo_llamada_stmt(s, $3, @1.first_line, @1.first_column);
        free($1);
    }
    ;

expression_list:
    /* vacío */ {
        $$ = NULL;
    }
    | expression_list_items {
        $$ = $1;
    }
    ;

expression_list_items:
    expression {
        $$ = crear_nodo_lista_args($1, NULL);
    }
    | expression_list_items ',' expression {
        $$ = agregar_arg_a_lista($1, $3);
    }
    ;

expression:
    ID {
        Simbolo *s = BuscarSimbolo(tabla_simbolos, $1);
        if (s == NULL) {
            sem_error(@1.first_line, @1.first_column, "Regla 2: Variable '%s' no declarada antes de su uso.", $1);
        } else if (s->flag == FLAG_FUNCION) {
            sem_error(@1.first_line, @1.first_column, "El identificador '%s' es un metodo, no una variable.", $1);
        }
        $$ = crear_nodo_var(s, @1.first_line, @1.first_column);
        free($1);
    }
    | ID '(' expression_list ')' {
        Simbolo *s = BuscarSimbolo(tabla_simbolos, $1);
        if (s == NULL) {
            sem_error(@1.first_line, @1.first_column, "Regla 2: Metodo '%s' no declarado antes de su uso.", $1);
        } else if (s->flag != FLAG_FUNCION) {
            sem_error(@1.first_line, @1.first_column, "El identificador '%s' no es un metodo.", $1);
        }
        $$ = crear_nodo_expr_llamada(s, $3, @1.first_line, @1.first_column);
        free($1);
    }
    | INTEGER {
        $$ = crear_nodo_literal_int($1, @1.first_line, @1.first_column);
    }
    | BOOLEAN {
        $$ = crear_nodo_literal_bool($1, @1.first_line, @1.first_column);
    }
    | FLOAT {
        $$ = crear_nodo_literal_float($1, @1.first_line, @1.first_column);
    }

    | expression SUMA expression           { $$ = crear_nodo_binario(OP_SUMA, $1, $3, @2.first_line, @2.first_column); }
    | expression RESTA expression          { $$ = crear_nodo_binario(OP_RESTA, $1, $3, @2.first_line, @2.first_column); }
    | expression MULTIPLICACION expression { $$ = crear_nodo_binario(OP_MULT, $1, $3, @2.first_line, @2.first_column); }
    | expression DIVISION expression       { $$ = crear_nodo_binario(OP_DIV, $1, $3, @2.first_line, @2.first_column); }
    | expression MODULO expression         { $$ = crear_nodo_binario(OP_MOD, $1, $3, @2.first_line, @2.first_column); }
    | expression AND expression            { $$ = crear_nodo_binario(OP_AND, $1, $3, @2.first_line, @2.first_column); }
    | expression OR expression             { $$ = crear_nodo_binario(OP_OR, $1, $3, @2.first_line, @2.first_column); }
    | expression MENOR expression          { $$ = crear_nodo_binario(OP_MENOR, $1, $3, @2.first_line, @2.first_column); }
    | expression MAYOR expression          { $$ = crear_nodo_binario(OP_MAYOR, $1, $3, @2.first_line, @2.first_column); }
    | expression IGUALDAD expression       { $$ = crear_nodo_binario(OP_IGUAL, $1, $3, @2.first_line, @2.first_column); }

    | RESTA expression %prec MENOS_UNARIO { $$ = crear_nodo_unario(OP_MENOS_UNARIO, $2, @1.first_line, @1.first_column); }
    | '!' expression                      { $$ = crear_nodo_unario(OP_NEGACION, $2, @1.first_line, @1.first_column); }
    | '(' expression ')'                  { $$ = $2; }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Error Sintactico en la linea %d, columna %d: %s\n", 
            yylloc.first_line, yylloc.first_column, s);
}