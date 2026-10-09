[1mdiff --cc bison.y[m
[1mindex 75e7140,11dcd28..0000000[m
[1m--- a/bison.y[m
[1m+++ b/bison.y[m
[36m@@@ -1,294 -1,53 +1,349 @@@[m
[32m++<<<<<<< HEAD[m
[32m +%{[m
[32m +#include <stdio.h>[m
[32m +#include <stdlib.h>[m
[32m +#include <string.h>[m
[32m +#include "tabla_simbolos.h"[m
[32m +#include "ast.h"[m
[32m +[m
[32m +extern int yylex(void);[m
[32m +extern int yylineno;[m
[32m +void yyerror(const char *s);[m
[32m +[m
[32m +// Puntero global a la tabla de símbolos[m
[32m +extern TablaSimbolos *tabla_simbolos;[m
[32m +[m
[32m +// Puntero al método actual que se está analizando[m
[32m +static Simbolo *metodo_actual = NULL;[m
[32m +%}[m
[32m +[m
[32m +%code requires {[m
[32m +    #include "tabla_simbolos.h"[m
[32m +    #include "ast.h"[m
[32m +}[m
[32m +[m
[32m +%locations[m
[32m +[m
[32m +%union {[m
[32m +    int          entero;[m
[32m +    float        flotante;[m
[32m +    int          boolean;[m
[32m +    char        *texto;[m
[32m +    NodoAST     *nodo;[m
[32m +    Simbolo     *simbolo;[m
[32m +    ListaIDs    *lista_ids;[m
[32m +}[m
[32m +[m
[32m +%token TOKEN_ERROR[m
[32m +[m
[32m +%token RETURN IF ELSE WHILE VOID[m
[32m +%token <entero> INTEGER <boolean> BOOLEAN <flotante> FLOAT <texto> ID[m
[32m +%token <texto> TYPE[m
[32m +[m
[32m +%token SUMA RESTA MULTIPLICACION DIVISION MODULO[m
[32m +%token AND OR[m
[32m +%token MENOR MAYOR IGUALDAD[m
[32m +[m
[32m +/* --- PRECEDENCIAS (de menor a mayor) --- */[m
[32m +%nonassoc LOWER_THAN_ELSE[m
[32m +%nonassoc ELSE[m
[32m +[m
[32m +%left OR[m
[32m +%left AND[m
[32m +%nonassoc MENOR MAYOR IGUALDAD[m
[32m +%left SUMA RESTA[m
[32m +%left MULTIPLICACION DIVISION MODULO[m
[32m +%nonassoc '!' MENOS_UNARIO[m
[32m +[m
[32m +/* --- TIPOS DE NO-TERMINALES --- */[m
[32m +%type <simbolo> method_header method_decl[m
[32m +%type <nodo> function_block block statement_or_decl statements_list statement[m
[32m +%type <nodo> expression method_call expression_list expression_list_items[m
[32m +%type <lista_ids> variable_list[m
[32m +[m
[32m +%%[m
[32m +[m
[32m +program:[m
[32m +    /* vacío */[m
[32m +    | lines[m
[32m +    ;[m
[32m +[m
[32m +lines:[m
[32m +    line[m
[32m +    | lines line[m
[32m +    ;[m
[32m +[m
[32m +line:[m
[32m +    variable_declr[m
[32m +    | method_decl[m
[32m +    ;[m
[32m +[m
[32m +method_decl:[m
[32m +    method_header '(' parameters_list ')' function_block {[m
[32m +        if ($1 != NULL) {[m
[32m +            $1->ast = $5;[m
[32m +        }[m
[32m +        CerrarNivel(tabla_simbolos);[m
[32m +        metodo_actual = NULL;[m
[32m +        $$ = $1;[m
[32m +    }[m
[32m +    ;[m
[32m +[m
[32m +method_header:[m
[32m +    TYPE ID {[m
[32m +        TipoDato td = StringATipoDato($1);[m
[32m +        free($1);[m
[32m +        Simbolo *s = InsertarSimbolo(tabla_simbolos, $2, FLAG_FUNCION, td, @2.first_line, @2.first_column);[m
[32m +        AbrirNivel(tabla_simbolos); // Nivel 2: parámetros y variables del método[m
[32m +        metodo_actual = s;[m
[32m +        free($2);[m
[32m +        $$ = s;[m
[32m +    }[m
[32m +    | VOID ID {[m
[32m +        Simbolo *s = InsertarSimbolo(tabla_simbolos, $2, FLAG_FUNCION, TIPO_VOID, @2.first_line, @2.first_column);[m
[32m +        AbrirNivel(tabla_simbolos); // Nivel 2: parámetros y variables del método[m
[32m +        metodo_actual = s;[m
[32m +        free($2);[m
[32m +        $$ = s;[m
[32m +    }[m
[32m +    ;[m
[32m +[m
[32m +parameters_list:[m
[32m +    /* vacío */[m
[32m +    | variable_declr_list[m
[32m +    ;[m
[32m +[m
[32m +variable_declr_list:[m
[32m +    param_item[m
[32m +    | variable_declr_list ',' param_item[m
[32m +    ;[m
[32m +[m
[32m +param_item:[m
[32m +    TYPE ID {[m
[32m +        TipoDato td = StringATipoDato($1);[m
[32m +        free($1);[m
[32m +        if (metodo_actual != NULL) {[m
[32m +            InsertarSimbolo(tabla_simbolos, $2, FLAG_PARAMETRO, td, @2.first_line, @2.first_column);[m
[32m +            AgregarParametroAFuncion(metodo_actual, $2, td);[m
[32m +        }[m
[32m +        free($2);[m
[32m +    }[m
[32m +    ;[m
[32m +[m
[32m +// Bloque principal de la función (el nivel ya fue abierto por method_header)[m
[32m +function_block:[m
[32m +    '{' statements_list '}' {[m
[32m +        $$ = $2;[m
[32m +    }[m
[32m +    ;[m
[32m +[m
[32m +// Bloque interno o bloque de control (abre y cierra un nuevo nivel local)[m
[32m +block:[m
[32m +    '{' { AbrirNivel(tabla_simbolos); } statements_list '}' {[m
[32m +        CerrarNivel(tabla_simbolos);[m
[32m +        $$ = $3;[m
[32m +    }[m
[32m +    ;[m
[32m +[m
[32m +statements_list:[m
[32m +    /* vacío */ {[m
[32m +        $$ = NULL;[m
[32m +    }[m
[32m +    | statements_list statement_or_decl {[m
[32m +        if ($2 != NULL) {[m
[32m +            $$ = concatenar_sentencias($1, $2);[m
[32m +        } else {[m
[32m +            $$ = $1;[m
[32m +        }[m
[32m +    }[m
[32m +    ;[m
[32m +[m
[32m +statement_or_decl:[m
[32m +    variable_declr {[m
[32m +        $$ = NULL; // Las declaraciones se registran en la TS pero no generan nodos de sentencia en el AST[m
[32m +    }[m
[32m +    | statement {[m
[32m +        $$ = $1;[m
[32m +    }[m
[32m +    ;[m
[32m +[m
[32m +statement:[m
[32m +    ID '=' expression ';' {[m
[32m +        Simbolo *s = BuscarSimbolo(tabla_simbolos, $1);[m
[32m +        NodoAST *var_nodo = crear_nodo_var(s, @1.first_line, @1.first_column);[m
[32m +        $$ = crear_nodo_asignacion(var_nodo, $3, @2.first_line, @2.first_column);[m
[32m +        free($1);[m
[32m +    }[m
[32m +    | method_call ';' {[m
[32m +        $$ = $1;[m
[32m +    }[m
[32m +    | IF '(' expression ')' block %prec LOWER_THAN_ELSE {[m
[32m +        $$ = crear_nodo_if($3, $5, NULL, @1.first_line, @1.first_column);[m
[32m +    }[m
[32m +    | IF '(' expression ')' block ELSE block {[m
[32m +        $$ = crear_nodo_if($3, $5, $7, @1.first_line, @1.first_column);[m
[32m +    }[m
[32m +    | WHILE expression block {[m
[32m +        $$ = crear_nodo_while($2, $3, @1.first_line, @1.first_column);[m
[32m +    }[m
[32m +    | RETURN expression ';' {[m
[32m +        $$ = crear_nodo_return($2, @1.first_line, @1.first_column);[m
[32m +    }[m
[32m +    | RETURN ';' {[m
[32m +        $$ = crear_nodo_return(NULL, @1.first_line, @1.first_column);[m
[32m +    }[m
[32m +    | ';' {[m
[32m +        $$ = NULL;[m
[32m +    }[m
[32m +    | block {[m
[32m +        $$ = $1;[m
[32m +    }[m
[32m +    ;[m
[32m +[m
[32m +variable_declr:[m
[32m +    TYPE variable_list ';' {[m
[32m +        TipoDato td = StringATipoDato($1);[m
[32m +        free($1);[m
[32m +        NodoID *curr = $2->primero;[m
[32m +        while (curr != NULL) {[m
[32m +            InsertarSimbolo(tabla_simbolos, curr->nombre, FLAG_VARIABLE, td, curr->linea, curr->columna);[m
[32m +            curr = curr->sig;[m
[32m +        }[m
[32m +        liberar_lista_ids($2);[m
[32m +    }[m
[32m +    ;[m
[32m +[m
[32m +variable_list:[m
[32m +    ID {[m
[32m +        $$ = crear_lista_ids($1, @1.first_line, @1.first_column);[m
[32m +        free($1);[m
[32m +    }[m
[32m +    | variable_list ',' ID {[m
[32m +        $$ = agregar_id_a_lista($1, $3, @3.first_line, @3.first_column);[m
[32m +        free($3);[m
[32m +    }[m
[32m +    ;[m
[32m +[m
[32m +method_call:[m
[32m +    ID '(' expression_list ')' {[m
[32m +        Simbolo *s = BuscarSimbolo(tabla_simbolos, $1);[m
[32m +        $$ = crear_nodo_llamada_stmt(s, $3, @1.first_line, @1.first_column);[m
[32m +        free($1);[m
[32m +    }[m
[32m +    ;[m
[32m +[m
[32m +expression_list:[m
[32m +    /* vacío */ {[m
[32m +        $$ = NULL;[m
[32m +    }[m
[32m +    | expression_list_items {[m
[32m +        $$ = $1;[m
[32m +    }[m
[32m +    ;[m
[32m +[m
[32m +expression_list_items:[m
[32m +    expression {[m
[32m +        $$ = crear_nodo_lista_args($1, NULL);[m
[32m +    }[m
[32m +    | expression_list_items ',' expression {[m
[32m +        $$ = agregar_arg_a_lista($1, $3);[m
[32m +    }[m
[32m +    ;[m
[32m +[m
[32m +expression:[m
[32m +    ID {[m
[32m +        Simbolo *s = BuscarSimbolo(tabla_simbolos, $1);[m
[32m +        $$ = crear_nodo_var(s, @1.first_line, @1.first_column);[m
[32m +        free($1);[m
[32m +    }[m
[32m +    | ID '(' expression_list ')' {[m
[32m +        Simbolo *s = BuscarSimbolo(tabla_simbolos, $1);[m
[32m +        $$ = crear_nodo_expr_llamada(s, $3, @1.first_line, @1.first_column);[m
[32m +        free($1);[m
[32m +    }[m
[32m +    | INTEGER {[m
[32m +        $$ = crear_nodo_literal_int($1, @1.first_line, @1.first_column);[m
[32m +    }[m
[32m +    | BOOLEAN {[m
[32m +        $$ = crear_nodo_literal_bool($1, @1.first_line, @1.first_column);[m
[32m +    }[m
[32m +    | FLOAT {[m
[32m +        $$ = crear_nodo_literal_float($1, @1.first_line, @1.first_column);[m
[32m +    }[m
[32m +[m
[32m +    | expression SUMA expression           { $$ = crear_nodo_binario(OP_SUMA, $1, $3, @2.first_line, @2.first_column); }[m
[32m +    | expression RESTA expression          { $$ = crear_nodo_binario(OP_RESTA, $1, $3, @2.first_line, @2.first_column); }[m
[32m +    | expression MULTIPLICACION expression { $$ = crear_nodo_binario(OP_MULT, $1, $3, @2.first_line, @2.first_column); }[m
[32m +    | expression DIVISION expression       { $$ = crear_nodo_binario(OP_DIV, $1, $3, @2.first_line, @2.first_column); }[m
[32m +    | expression MODULO expression         { $$ = crear_nodo_binario(OP_MOD, $1, $3, @2.first_line, @2.first_column); }[m
[32m +    | expression AND expression            { $$ = crear_nodo_binario(OP_AND, $1, $3, @2.first_line, @2.first_column); }[m
[32m +    | expression OR expression             { $$ = crear_nodo_binario(OP_OR, $1, $3, @2.first_line, @2.first_column); }[m
[32m +    | expression MENOR expression          { $$ = crear_nodo_binario(OP_MENOR, $1, $3, @2.first_line, @2.first_column); }[m
[32m +    | expression MAYOR expression          { $$ = crear_nodo_binario(OP_MAYOR, $1, $3, @2.first_line, @2.first_column); }[m
[32m +    | expression IGUALDAD expression       { $$ = crear_nodo_binario(OP_IGUAL, $1, $3, @2.first_line, @2.first_column); }[m
[32m +[m
[32m +    | RESTA expression %prec MENOS_UNARIO { $$ = crear_nodo_unario(OP_MENOS_UNARIO, $2, @1.first_line, @1.first_column); }[m
[32m +    | '!' expression                      { $$ = crear_nodo_unario(OP_NEGACION, $2, @1.first_line, @1.first_column); }[m
[32m +    | '(' expression ')'                  { $$ = $2; }[m
[32m +    ;[m
[32m +[m
[32m +%%[m
[32m +[m
[32m +void yyerror(const char *s) {[m
[32m +    fprintf(stderr, "Error Sintactico en la linea %d, columna %d: %s\n", [m
[32m +            yylloc.first_line, yylloc.first_column, s);[m
[32m++=======[m
[32m+ %{[m
[32m+ #include <stdio.h>[m
[32m+ #include <stdlib.h>[m
[32m+ #include "bison.tab.h"[m
[32m+ // Declaraciones externas necesarias[m
[32m+ extern int yylex();[m
[32m+ extern int yyparse();[m
[32m+ extern FILE *yyin;[m
[32m+ [m
[32m+ void yyerror(const char *s);[m
[32m+ %}[m
[32m+ [m
[32m+ %token INTEGER BOOLEAN FLOAT ID[m
[32m+ %token TOKEN_ERROR[m
[32m+ [m
[32m+ %%[m
[32m+ [m
[32m+ expression:[m
[32m+     INTEGER                          { printf("INTEGER\n"); }[m
[32m+     | BOOLEAN                          { printf("BOOLEAN\n"); }[m
[32m+     | FLOAT                            { printf("FLOAT\n"); }[m
[32m+     | ID                               { printf("ID\n"); }[m
[32m+     // | expression OP expression       { printf("suma\n"); }[m
[32m+     | '-' expression                 { printf("menos expresion\n"); }[m
[32m+     | '!' expression                  { printf("expresion negada\n"); }[m
[32m+     | '(' expression ')' { printf("expresion entre parentesis\n"); }[m
[32m+     ;[m
[32m+ %%[m
[32m+ [m
[32m+ void yyerror(const char *s) {[m
[32m+     fprintf(stderr, "Error sintactico: %s\n", s);[m
[32m+ }[m
[32m+ [m
[32m+ int main(int argc, char **argv) {[m
[32m+     // Verificamos si el usuario pasó el nombre del archivo como argumento[m
[32m+     if (argc < 2) {[m
[32m+         fprintf(stderr, "Uso: %s <archivo_de_entrada>\n", argv[0]);[m
[32m+         return 1;[m
[32m+     }[m
[32m+ [m
[32m+     // Abrimos el archivo en modo lectura[m
[32m+     yyin = fopen(argv[1], "r");[m
[32m+     if (!yyin) {[m
[32m+         perror("Error al abrir el archivo");[m
[32m+         return 1;[m
[32m+     }[m
[32m+ [m
[32m+     // Ejecutamos el analizador[m
[32m+     yyparse();[m
[32m+ [m
[32m+     fclose(yyin); // Cerramos el archivo[m
[32m+     return 0;[m
[32m++>>>>>>> origin/ricardo[m
  }[m
[1mdiff --cc lex.l[m
[1mindex 63011ea,d95f3c8..0000000[m
[1m--- a/lex.l[m
[1m+++ b/lex.l[m
[36m@@@ -24,87 -10,96 +24,127 @@@[m [mINTEGER {DIGIT}[m
  FLOAT {INTEGER}\.{INTEGER}[m
  ID {ALPHA}{ALPHA_NUM}*[m
  [m
[32m++<<<<<<< HEAD[m
[32m +TYPE "int"|"boolean"|"float"[m
[32m +[m
[32m +%x COMENTARIO[m
[32m +%x COMENTARIO_LINEA[m
[32m +[m
[32m +%option yylineno[m
[32m +%option noyywrap[m
[32m +%option nounput[m
[32m +%option noinput[m
[32m++=======[m
[32m+ TYPE "int"|"bool"|"float"[m
[32m+ RESERVED "return"|"if"|"while"|"else"|"void"|"main"[m
[32m+ %x COMENTARIO[m
[32m++>>>>>>> origin/ricardo[m
  [m
  /* Reglas básicas de ejemplo */[m
  %%[m
[31m -{RESERVED}              { printf("Palabra reservada: %s\n", yytext); }[m
[31m -{TYPE}                  { printf("Tipo de dato: %s\n", yytext); }[m
[31m -{INTEGER}               return INTEGER; // { printf("Numero: %s\n", yytext); }[m
[31m -{BOOLEAN}               return BOOLEAN; // { printf("Booleano: %s\n", yytext); }[m
[31m -{FLOAT}                 return FLOAT; // { printf("Flotante: %s\n", yytext); }[m
[31m -{ID}                    return ID; // { printf("ID: %s\n", yytext); }[m
[31m -[m
[32m +"return"                return RETURN;[m
[32m +"if"                    return IF;[m
[32m +"else"                  return ELSE;[m
[32m +"while"                 return WHILE;[m
[32m +"void"                  return VOID;[m
[32m +[m
[32m +{TYPE}                  { yylval.texto = strdup(yytext); return TYPE; }[m
[32m +{INTEGER}               { yylval.entero = atoi(yytext); return INTEGER; }[m
[32m +"true"                  { yylval.boolean = 1; return BOOLEAN; }[m
[32m +"false"                 { yylval.boolean = 0; return BOOLEAN; }[m
[32m +{FLOAT}                 { yylval.flotante = atof(yytext); return FLOAT; }[m
[32m +{ID}                    { yylval.texto = strdup(yytext); return ID; }[m
[32m +[m
[32m++<<<<<<< HEAD[m
[32m +[m
[32m +"+"                     return SUMA;[m
[32m +"-"                     return RESTA;[m
[32m +"*"                     return MULTIPLICACION;[m
[32m +"/"                     return DIVISION;[m
[32m +"%"                     return MODULO;[m
[32m++=======[m
[32m+             [m
[32m+ "+"                     return '+';[m
[32m+ "-"                     return '-';[m
[32m+ "*"                     return '*';[m
[32m+ "/"                     return '/';[m
[32m+ "%"                     return '%';[m
[32m++>>>>>>> origin/ricardo[m
  [m
[31m -"&&"                    return "&&";[m
[31m -"||"                    return "||";[m
[32m +"&&"                    return AND;[m
[32m +"||"                    return OR;[m
  [m
[31m -"<"                     return '<';[m
[31m -">"                     return '>';[m
[31m -"=="                    return "==";[m
[32m +"<"                     return MENOR;[m
[32m +">"                     return MAYOR;[m
[32m +"=="                    return IGUALDAD;[m
  [m
[31m -"="                     return '='; // { printf("Asignacion: %s\n", yytext); }[m
[31m -"!"                     return '!'; // { printf("Negacion: %s\n", yytext); }[m
[32m +"="                     return '=';[m
[32m +"!"                     return '!';[m
  [m
  [m
[32m +","                     return ',';[m
  ";"                     return ';';[m
[32m+ ","                     return ',';[m
  "("                     return '(';[m
  ")"                     return ')';[m
  "{"                     return '{';[m
  "}"                     return '}';[m
[31m -"\n"                    { /* Ignorar saltos de línea */ }[m
[32m +"\n"                    { current_column = 1; }[m
  [ \t]                   { /* Ignorar espacios vacios y tabulaciones */ }[m
[32m++<<<<<<< HEAD[m
[32m +.                       { printf("Error Lexico Lexema inesperado: (%s) en linea %d, columna %d\n", yytext, yylineno, yylloc.first_column); return TOKEN_ERROR; }[m
[32m++=======[m
[32m+ .                       { printf("Lexema inesperado: %s\n", yytext); }[m
[32m+              [m
[32m+ "//"[^\n]*\n[m
[32m+ [m
[32m+ "/*"  {BEGIN(COMENTARIO);}[m
[32m+ [m
[32m+ <COMENTARIO>"*/"    {[m
[32m+     BEGIN(INITIAL);[m
[32m+ }[m
[32m+ [m
[32m+ <COMENTARIO>\n  ;[m
[32m+ <COMENTARIO>. ;[m
[32m+ [m
[32m+ <COMENTARIO><<EOF>> {[m
[32m+     fprintf(stderr,[m
[32m+             "Error lexico:comentario sin Cerrar en linia %d\n",[m
[32m+             yylineno);[m
[32m+             return TOKEN_ERROR;[m
[32m+ }[m
[32m+ [m
[32m+ %%[m
[32m++>>>>>>> origin/ricardo[m
  [m
[32m +"//"                    { BEGIN(COMENTARIO_LINEA); }[m
[32m +<COMENTARIO_LINEA>\n    { current_column = 1; BEGIN(INITIAL); }[m
[32m +<COMENTARIO_LINEA>.     ;[m
[32m +<COMENTARIO_LINEA><<EOF>> { BEGIN(INITIAL); }[m
  [m
[31m -int yywrap() {[m
[31m -    return 1;[m
[31m -}[m
[32m +"/*"  {[m
[32m +        commentary_counter = 1;[m
[32m +        BEGIN(COMENTARIO);[m
[32m +      }[m
  [m
[31m -/*[m
[31m -int main(int argc, char *argv[]) {[m
[31m -    // Verificar si se pasó el nombre del archivo como argumento[m
[31m -    if (argc < 2) {[m
[31m -        printf("Uso: %s <archivo_de_entrada>\n", argv[0]);[m
[31m -        return 1;[m
[31m -    }[m
[32m +<COMENTARIO>"/*"    {[m
[32m +    commentary_counter++;[m
[32m +}[m
  [m
[31m -    // Abrir el archivo en modo lectura[m
[31m -    FILE *archivo = fopen(argv[1], "r");[m
[31m -    if (!archivo) {[m
[31m -        printf("Error: No se pudo abrir el archivo %s\n", argv[1]);[m
[31m -        return 1;[m
[32m +<COMENTARIO>"*/"    {[m
[32m +    commentary_counter--;[m
[32m +    if( commentary_counter == 0 ) {[m
[32m +        BEGIN(INITIAL);[m
      }[m
[32m +}[m
  [m
[31m -    // Redireccionar la entrada de Flex al archivo[m
[31m -    yyin = archivo;[m
[31m -[m
[31m -    // Iniciar el análisis léxico[m
[31m -    yylex();[m
[32m +<COMENTARIO>\n  { current_column = 1; };[m
[32m +<COMENTARIO>. ;[m
  [m
[31m -    // Cerrar el archivo al terminar[m
[31m -    fclose(archivo);[m
[31m -    return 0;[m
[32m +<COMENTARIO><<EOF>> {[m
[32m +    fprintf(stderr,[m
[32m +            "Error lexico:comentario sin Cerrar en linea %d\n",[m
[32m +            yylineno);[m
[32m +            return TOKEN_ERROR;[m
  }[m
[31m -*/[m
[32m +%%[m
