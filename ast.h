#ifndef AST_H
#define AST_H

#include "tabla_simbolos.h"

// Tipos de nodo del AST (según Especificación y Diapositivas de la materia)
typedef enum {
    // --- Sentencias ---
    NODO_SECUENCIA,     // Enlace de sentencias ';' (izq: sent1, der: sent2 o siguiente secuencia)
    NODO_ASIGNACION,    // Asignación '=' (izq: variable ID, der: expresión)
    NODO_IF,            // Condicional ternario (izq: condición, centro: rama then, der: rama else)
    NODO_WHILE,         // Bucle while (izq: condición, centro: cuerpo del bucle)
    NODO_RETURN,        // Sentencia return (izq: expresión o NULL si es void)
    NODO_LLAMADA_STMT,  // Llamada a función como sentencia (simbolo: función en TS, izq: argumentos)
    NODO_BLOQUE,        // Bloque de sentencias delimitado por { }

    // --- Expresiones ---
    NODO_EXPR_BINARIA,  // Operación binaria (izq: expr, der: expr)
    NODO_EXPR_UNARIA,   // Operación unaria (izq: expr)
    NODO_EXPR_VAR,      // Referencia a variable/parámetro (simbolo: puntero al Simbolo en la TS)
    NODO_EXPR_LITERAL,  // Literal constante (int, float, boolean)
    NODO_EXPR_LLAMADA,  // Invocación a función dentro de expresión (simbolo: función en TS, izq: argumentos)
    NODO_LISTA_ARGS     // Nodo para encadenar argumentos en llamadas (izq: argumento, der: siguiente)
} TipoNodoAST;

// Operadores en expresiones binarias y unarias
typedef enum {
    OP_SUMA,            // +
    OP_RESTA,           // -
    OP_MULT,            // *
    OP_DIV,             // /
    OP_MOD,             // %
    OP_MENOR,           // <
    OP_MAYOR,           // >
    OP_IGUAL,           // ==
    OP_AND,             // &&
    OP_OR,              // ||
    OP_NEGACION,        // !
    OP_MENOS_UNARIO,    // -
    OP_NINGUNO
} OperadorAST;

// Estructura del Nodo del AST
// Sigue la especificación de la Diapositiva 13:
// struct nodoAST { *simbolo, *nodoAST, *nodoAST, *nodoAST, otra info }
typedef struct nodoAST {
    TipoNodoAST tipo_nodo;
    Simbolo *simbolo;               // Referencia al elemento de la TS (puntero directo)
    struct nodoAST *hijo_izq;       // Primer subárbol (izq / condición / primer arg / var)
    struct nodoAST *hijo_centro;    // Segundo subárbol (centro / cuerpo then / cuerpo while)
    struct nodoAST *hijo_der;       // Tercer subárbol (der / cuerpo else / encadenamiento ;)
    
    // Otra información (para análisis semántico, chequeo de tipos y codegen):
    OperadorAST op;                 // Operador aritmético/lógico/relacional
    TipoDato tipo_dato;             // Tipo de dato inferido o del literal
    union {
        int val_int;
        float val_float;
        int val_bool;
    } valor;                        // Valor numérico/booleano para literales
    
    int linea;
    int columna;
} NodoAST;

// Constructores de nodos de sentencias
NodoAST* crear_nodo(TipoNodoAST tipo, int linea, int columna);
NodoAST* crear_nodo_secuencia(NodoAST *izq, NodoAST *der);
NodoAST* concatenar_sentencias(NodoAST *s1, NodoAST *s2);
NodoAST* crear_nodo_asignacion(NodoAST *var, NodoAST *expr, int linea, int columna);
NodoAST* crear_nodo_if(NodoAST *cond, NodoAST *then_b, NodoAST *else_b, int linea, int columna);
NodoAST* crear_nodo_while(NodoAST *cond, NodoAST *cuerpo, int linea, int columna);
NodoAST* crear_nodo_return(NodoAST *expr, int linea, int columna);
NodoAST* crear_nodo_llamada_stmt(Simbolo *metodo, NodoAST *args, int linea, int columna);
NodoAST* crear_nodo_bloque(NodoAST *sentencias, int linea, int columna);

// Constructores de nodos de expresiones
NodoAST* crear_nodo_binario(OperadorAST op, NodoAST *izq, NodoAST *der, int linea, int columna);
NodoAST* crear_nodo_unario(OperadorAST op, NodoAST *expr, int linea, int columna);
NodoAST* crear_nodo_var(Simbolo *s, int linea, int columna);
NodoAST* crear_nodo_literal_int(int val, int linea, int columna);
NodoAST* crear_nodo_literal_float(float val, int linea, int columna);
NodoAST* crear_nodo_literal_bool(int val, int linea, int columna);
NodoAST* crear_nodo_expr_llamada(Simbolo *metodo, NodoAST *args, int linea, int columna);
NodoAST* crear_nodo_lista_args(NodoAST *arg, NodoAST *sig);
NodoAST* agregar_arg_a_lista(NodoAST *lista, NodoAST *nuevo_arg);

// Funciones de visualización y liberación
const char* OperadorAString(OperadorAST op);
void imprimir_ast(const NodoAST *nodo, int indent, int es_ultimo, const char *prefijo);
void imprimir_ast_metodo(const Simbolo *metodo);
void imprimir_ast_todos_los_metodos(const TablaSimbolos *ts);
void generar_ast_dot(const TablaSimbolos *ts, const char *nombre_archivo);
void liberar_ast(NodoAST *nodo);

#endif // AST_H
