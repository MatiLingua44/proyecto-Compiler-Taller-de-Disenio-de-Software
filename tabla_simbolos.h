#ifndef TABLA_SIMBOLOS_H
#define TABLA_SIMBOLOS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tipos de datos soportados en C-TDS
typedef enum {
    TIPO_INT,
    TIPO_FLOAT,
    TIPO_BOOLEAN,
    TIPO_VOID,
    TIPO_ERROR,
    TIPO_DESCONOCIDO
} TipoDato;

// Categoría o flag del símbolo (según especificación y diapositivas de la materia)
typedef enum {
    FLAG_VARIABLE,  // Variable local o global
    FLAG_FUNCION,   // Método o función
    FLAG_PARAMETRO  // Parámetro formal de método
} TipoSimbolo;

// Declaración adelantada del nodo del AST
struct nodoAST;

// Parámetro formal de una función
typedef struct Parametro {
    char *nombre;
    TipoDato tipo;
    struct Parametro *sig;
} Parametro;

// Estructura de un Símbolo (según Diapositiva 18: struct simbolo{flag, nombre, tipo, valor, etc})
typedef struct simbolo {
    char *nombre;
    TipoSimbolo flag;
    TipoDato tipo;
    int nivel;             // Nivel de ámbito (1 = global, 2+ = local/bloque)
    int linea;
    int columna;

    // Campos específicos para funciones
    Parametro *parametros; // Lista enlazada de parámetros formales
    int cant_parametros;
    struct nodoAST *ast;   // Puntero a la raíz del AST de este método ("Un AST por función")

    // Campos específicos para variables (almacenamiento de valores para interpretación o codegen)
    union {
        int val_int;
        float val_float;
        int val_bool;
    } valor;

    struct simbolo *sig;   // Siguiente símbolo en la lista del mismo nivel
} Simbolo;

// Nivel de la pila de ámbitos (Pila de niveles)
typedef struct Nivel {
    int numero;                        // 1 = global, 2 = función, etc.
    Simbolo *simbolos;                 // Lista de símbolos declarados en este nivel
    struct Nivel *anterior;            // Puntero al nivel anterior en la pila
    struct Nivel *siguiente_historial; // Lista completa de niveles creados para preservar memoria
} Nivel;

// TAD Tabla de Símbolos
typedef struct TablaSimbolos {
    Nivel *tope;                       // Nivel actual (cima de la pila de ámbitos)
    Nivel *global;                     // Nivel 1 (ámbito global)
    int nivel_actual;                  // Contador del nivel actual
    Nivel *historial_niveles;          // Lista con todos los niveles creados (para no perder punteros del AST)
    int total_errores;                 // Contador de errores semánticos detectados en la TS
} TablaSimbolos;

// Estructura auxiliar para listas de identificadores en declaraciones múltiples (ej: int a, b, c;)
typedef struct NodoID {
    char *nombre;
    int linea;
    int columna;
    struct NodoID *sig;
} NodoID;

typedef struct ListaIDs {
    NodoID *primero;
    NodoID *ultimo;
} ListaIDs;

// Funciones del TAD Tabla de Símbolos (según Diapositiva 18):
TablaSimbolos* InicializarTS(void);
void AbrirNivel(TablaSimbolos *ts);
void CerrarNivel(TablaSimbolos *ts);
Simbolo* InsertarSimbolo(TablaSimbolos *ts, const char *nombre, TipoSimbolo flag, TipoDato tipo, int linea, int columna);
Simbolo* BuscarSimbolo(TablaSimbolos *ts, const char *nombre);
Simbolo* BuscarSimboloNivelActual(TablaSimbolos *ts, const char *nombre);

// Funciones auxiliares para parámetros
void AgregarParametroAFuncion(Simbolo *funcion, const char *nombre, TipoDato tipo);

// Funciones auxiliares para declaraciones múltiples
ListaIDs* crear_lista_ids(char *nombre, int linea, int columna);
ListaIDs* agregar_id_a_lista(ListaIDs *lista, char *nombre, int linea, int columna);
void liberar_lista_ids(ListaIDs *lista);

// Conversión y utilidades
const char* TipoDatoAString(TipoDato t);
const char* TipoSimboloAString(TipoSimbolo f);
TipoDato StringATipoDato(const char *str);

// Impresión y depuración (muestra la TS tal como en las diapositivas)
void ImprimirTS(const TablaSimbolos *ts);
void ImprimirSimbolo(const Simbolo *s);

// Liberación de recursos
void LiberarTS(TablaSimbolos *ts);

#endif // TABLA_SIMBOLOS_H
