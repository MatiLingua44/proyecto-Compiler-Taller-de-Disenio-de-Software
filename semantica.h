#ifndef SEMANTICA_H
#define SEMANTICA_H

#include "tabla_simbolos.h"
#include "ast.h"

// Inicializa el módulo de semántica
void inicializar_semantica(void);

// Reporta un error semántico con línea y columna
void sem_error(int linea, int col, const char *fmt, ...);

// Obtiene la cantidad total de errores semánticos registrados
int obtener_cant_errores_semanticos(void);

// Verificaciones semánticas del programa completo:
// Regla 3: Verificar existencia del método main sin parámetros
int verificar_programa(TablaSimbolos *ts);

// Chequeo de tipos sobre el AST (según Reglas 4 a 14 de C-TDS)
int chequear_tipos_programa(TablaSimbolos *ts);
TipoDato chequear_tipos_nodo(NodoAST *nodo, Simbolo *metodo_actual);

#endif // SEMANTICA_H
