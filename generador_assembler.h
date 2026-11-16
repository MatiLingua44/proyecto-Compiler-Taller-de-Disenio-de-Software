#ifndef GENERADOR_ASSEMBLER_H
#define GENERADOR_ASSEMBLER_H

#include <stdio.h>
#include "codigo_intermedio.h"
#include "tabla_simbolos.h"

/*
 * ==============================================================================
 * GENERADOR DE CÓDIGO OBJETO / ASSEMBLER x86-64 (GNU Assembler - AT&T para GCC)
 * Materia: Taller de Diseño de Software (3306) - UNRC
 *
 * Genera código ensamblador x86-64 compatible directamente con el compilador GCC:
 *   gcc programa.s -o programa
 *
 * Características principales:
 * - Convención de llamadas estándar System V AMD64 ABI (%rdi, %rsi, %rdx, %rcx, %r8, %r9).
 * - Marco de pila alineado a 16 bytes con preservación de %rbp.
 * - Soporte para variables globales (en sección .data con direccionamiento RIP-relative).
 * - Soporte para parámetros y variables locales (con resolución de ámbito y shadowing).
 * - Mapeo de registros temporales (R1, R2, ...) en el marco de pila del método.
 * - Operaciones aritméticas, relacionales y lógicas completas.
 * - Estructuras de control (if, while) con etiquetas acotadas por función.
 * - Emisión de directiva .section .note.GNU-stack para compatibilidad total con GCC moderno.
 * ==============================================================================
 */

// Genera el código assembler x86-64 y lo escribe en el flujo de archivo indicado
void generar_codigo_assembler(const CodigoIntermedio *ci, const TablaSimbolos *ts, FILE *salida, const char *nombre_archivo_fuente);

// Genera el código assembler x86-64 y lo guarda en el archivo especificado en disco
int exportar_archivo_assembler(const CodigoIntermedio *ci, const TablaSimbolos *ts, const char *ruta_salida, const char *nombre_archivo_fuente);

#endif // GENERADOR_ASSEMBLER_H
