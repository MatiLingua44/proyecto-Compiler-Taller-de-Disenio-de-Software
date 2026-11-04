#ifndef CODIGO_INTERMEDIO_H
#define CODIGO_INTERMEDIO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tabla_simbolos.h"
#include "ast.h"

/*
 * ==============================================================================
 * GENERACIÓN DE CÓDIGO INTERMEDIO: CÓDIGO DE TRES DIRECCIONES (3AC / TAC)
 * Materia: Taller de Diseño de Software (3306) - UNRC
 * Cátedra: Prof. Francisco Bavera
 *
 * Basado en las Diapositivas 8, 9 y 10:
 * - Reescritura del árbol: reemplazo de nodos/subárboles por segmentos de código intermedio.
 * - Secuencia lineal: recorrido en post-orden del árbol reescrito.
 * - Instrucciones de 3 direcciones:
 *     Load_Const  c, Rn      (Rn := c)
 *     Load_Mem    x, Rn      (Rn := x)
 *     Store_Mem   Rn, x      (x := Rn)
 *     Add_Reg     Rm, Rn, R  (R := Rm + Rn)
 *     Sub_Reg     Rm, Rn, R  (R := Rm - Rn)
 *     Mul_Reg     Rm, Rn, R  (R := Rm * Rn)
 *     Div_Reg     Rm, Rn, R  (R := Rm / Rn)
 *     Mod_Reg     Rm, Rn, R  (R := Rm % Rn)
 *     Neg_Reg     Rn, R      (R := -Rn)
 *     Not_Reg     Rn, R      (R := !Rn)
 *     Lt_Reg      Rm, Rn, R  (R := Rm < Rn)
 *     Gt_Reg      Rm, Rn, R  (R := Rm > Rn)
 *     Eq_Reg      Rm, Rn, R  (R := Rm == Rn)
 *     And_Reg     Rm, Rn, R  (R := Rm && Rn)
 *     Or_Reg      Rm, Rn, R  (R := Rm || Rn)
 *     Label       L:
 *     Jump        L
 *     Jump_False  Rn, L
 *     Jump_True   Rn, L
 *     Begin_Func  f
 *     End_Func    f
 *     Param       Rn
 *     Call        f, n, R / Call f, n
 *     Return_Reg  Rn
 *     Return
 * ==============================================================================
 */

typedef enum {
    INSTR_LOAD_CONST,   // Load_Const c, Rn
    INSTR_LOAD_MEM,     // Load_Mem x, Rn
    INSTR_STORE_MEM,    // Store_Mem Rn, x
    INSTR_ADD_REG,      // Add_Reg Rm, Rn, R
    INSTR_SUB_REG,      // Sub_Reg Rm, Rn, R
    INSTR_MUL_REG,      // Mul_Reg Rm, Rn, R
    INSTR_DIV_REG,      // Div_Reg Rm, Rn, R
    INSTR_MOD_REG,      // Mod_Reg Rm, Rn, R
    INSTR_NEG_REG,      // Neg_Reg Rn, R
    INSTR_NOT_REG,      // Not_Reg Rn, R
    INSTR_LT_REG,       // Lt_Reg Rm, Rn, R
    INSTR_GT_REG,       // Gt_Reg Rm, Rn, R
    INSTR_EQ_REG,       // Eq_Reg Rm, Rn, R
    INSTR_AND_REG,      // And_Reg Rm, Rn, R
    INSTR_OR_REG,       // Or_Reg Rm, Rn, R
    INSTR_LABEL,        // Label L:
    INSTR_JUMP,         // Jump L
    INSTR_JUMP_FALSE,   // Jump_False Rn, L
    INSTR_JUMP_TRUE,    // Jump_True Rn, L
    INSTR_BEGIN_FUNC,   // Begin_Func f
    INSTR_END_FUNC,     // End_Func f
    INSTR_PARAM,        // Param Rn
    INSTR_CALL,         // Call f, cant_args, R (o Call f, cant_args)
    INSTR_RETURN_REG,   // Return_Reg Rn
    INSTR_RETURN        // Return
} TipoInstruccion3D;

// Instrucción individual en la secuencia lineal de tres direcciones
typedef struct Instruccion3D {
    TipoInstruccion3D tipo;
    char *op1;          // Primer operando / constante / variable / registro fuente
    char *op2;          // Segundo operando / cantidad de args / registro
    char *resultado;    // Registro destino / variable destino / etiqueta
    struct Instruccion3D *ant;
    struct Instruccion3D *sig;
} Instruccion3D;

// Lista doblemente enlazada de instrucciones lineales
typedef struct Lista3D {
    Instruccion3D *primero;
    Instruccion3D *ultimo;
    int cantidad;
} Lista3D;

// Nodo del Árbol Reescrito (Diapositivas 8 y 10: "Reescritura del árbol")
typedef struct NodoReescrito {
    TipoInstruccion3D tipo;
    char *op1;
    char *op2;
    char *resultado;
    struct NodoReescrito *hijo_izq;
    struct NodoReescrito *hijo_der;
    struct NodoReescrito *hijo_centro;
    struct NodoReescrito *sig_sentencia;
} NodoReescrito;

// Representación de un método o función en Código Intermedio
typedef struct Metodo3D {
    char *nombre;
    TipoDato tipo_retorno;
    NodoReescrito *arbol_reescrito; // Raíz del árbol reescrito del método
    Lista3D *instrucciones;         // Secuencia lineal de instrucciones 3D
    struct Metodo3D *sig;
} Metodo3D;

// Estructura principal que agrupa todo el código intermedio del programa
typedef struct CodigoIntermedio {
    Metodo3D *metodos;
    int total_metodos;
} CodigoIntermedio;

// Funciones principales del módulo
CodigoIntermedio* generar_codigo_intermedio(const TablaSimbolos *ts);
void imprimir_codigo_intermedio(const CodigoIntermedio *ci, FILE *salida);
void imprimir_arbol_reescrito_todos(const CodigoIntermedio *ci, FILE *salida);
void generar_ci_dot(const CodigoIntermedio *ci, const char *nombre_archivo);
void liberar_codigo_intermedio(CodigoIntermedio *ci);

// Funciones auxiliares de formateo
const char* tipo_instr_a_string(TipoInstruccion3D tipo);
void formatear_instruccion_3d(const Instruccion3D *ins, char *buffer, size_t max_len);

#endif // CODIGO_INTERMEDIO_H
