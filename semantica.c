#include "semantica.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static int total_errores_semanticos = 0;

void inicializar_semantica(void) {
    total_errores_semanticos = 0;
}

void sem_error(int linea, int col, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (linea > 0) {
        fprintf(stderr, "Error semantico en linea %d, columna %d: ", linea, col);
    } else {
        fprintf(stderr, "Error semantico: ");
    }
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    total_errores_semanticos++;
}

int obtener_cant_errores_semanticos(void) {
    return total_errores_semanticos;
}

// Verifica la Regla 3: El programa debe contener la definición de un método llamado main sin parámetros
int verificar_programa(TablaSimbolos *ts) {
    if (!ts || !ts->global) return 0;

    Simbolo *main_sym = NULL;
    Simbolo *curr = ts->global->simbolos;
    while (curr != NULL) {
        if (strcmp(curr->nombre, "main") == 0) {
            main_sym = curr;
            break;
        }
        curr = curr->sig;
    }

    if (main_sym == NULL) {
        sem_error(0, 0, "Regla 3: No se encontro la definicion del metodo obligatorio 'main'.");
        return 0;
    }

    if (main_sym->flag != FLAG_FUNCION) {
        sem_error(main_sym->linea, main_sym->columna, "Regla 3: El identificador 'main' debe ser una funcion/metodo.");
        return 0;
    }

    if (main_sym->cant_parametros != 0) {
        sem_error(main_sym->linea, main_sym->columna, "Regla 3: El metodo 'main' no debe tener parametros (se encontraron %d).", main_sym->cant_parametros);
        return 0;
    }

    return 1;
}

// Verifica si dos tipos son numéricos (int o float)
static int es_tipo_numerico(TipoDato t) {
    return (t == TIPO_INT || t == TIPO_FLOAT);
}

// Verifica compatibilidad de asignación (Regla 13 y 14)
static int son_tipos_compatibles_asignacion(TipoDato destino, TipoDato origen) {
    if (destino == TIPO_ERROR || origen == TIPO_ERROR) return 1; // Evitar cascada de errores
    if (destino == origen) return 1;
    // Regla 14: Se permiten coerciones o truncamientos entre int y float
    if (es_tipo_numerico(destino) && es_tipo_numerico(origen)) return 1;
    return 0;
}

// Chequeo recursivo de tipos sobre un nodo del AST
TipoDato chequear_tipos_nodo(NodoAST *nodo, Simbolo *metodo_actual) {
    if (!nodo) return TIPO_VOID;

    switch (nodo->tipo_nodo) {
        case NODO_EXPR_LITERAL:
            return nodo->tipo_dato;

        case NODO_EXPR_VAR:
            if (!nodo->simbolo) {
                return TIPO_ERROR;
            }
            nodo->tipo_dato = nodo->simbolo->tipo;
            return nodo->tipo_dato;

        case NODO_EXPR_BINARIA: {
            TipoDato t_izq = chequear_tipos_nodo(nodo->hijo_izq, metodo_actual);
            TipoDato t_der = chequear_tipos_nodo(nodo->hijo_der, metodo_actual);

            if (t_izq == TIPO_ERROR || t_der == TIPO_ERROR) {
                nodo->tipo_dato = TIPO_ERROR;
                return TIPO_ERROR;
            }

            switch (nodo->op) {
                case OP_SUMA:
                case OP_RESTA:
                case OP_MULT:
                case OP_DIV:
                    // Regla 10: operandos deben ser int o float
                    if (!es_tipo_numerico(t_izq) || !es_tipo_numerico(t_der)) {
                        sem_error(nodo->linea, nodo->columna,
                                  "Regla 10: Los operandos del operador '%s' deben ser de tipo int o float (se recibieron %s y %s).",
                                  OperadorAString(nodo->op), TipoDatoAString(t_izq), TipoDatoAString(t_der));
                        nodo->tipo_dato = TIPO_ERROR;
                        return TIPO_ERROR;
                    }
                    // Regla 14: si uno es float, el resultado es float, sino int
                    if (t_izq == TIPO_FLOAT || t_der == TIPO_FLOAT) {
                        nodo->tipo_dato = TIPO_FLOAT;
                    } else {
                        nodo->tipo_dato = TIPO_INT;
                    }
                    return nodo->tipo_dato;

                case OP_MOD:
                    // Módulo sólo entre enteros
                    if (t_izq != TIPO_INT || t_der != TIPO_INT) {
                        sem_error(nodo->linea, nodo->columna,
                                  "Los operandos del operador '%%' deben ser de tipo entero.");
                        nodo->tipo_dato = TIPO_ERROR;
                        return TIPO_ERROR;
                    }
                    nodo->tipo_dato = TIPO_INT;
                    return TIPO_INT;

                case OP_MENOR:
                case OP_MAYOR:
                    // Regla 10: operandos de relacionales deben ser int o float
                    if (!es_tipo_numerico(t_izq) || !es_tipo_numerico(t_der)) {
                        sem_error(nodo->linea, nodo->columna,
                                  "Regla 10: Los operandos de comparacion '%s' deben ser int o float.",
                                  OperadorAString(nodo->op));
                        nodo->tipo_dato = TIPO_ERROR;
                        return TIPO_ERROR;
                    }
                    nodo->tipo_dato = TIPO_BOOLEAN;
                    return TIPO_BOOLEAN;

                case OP_IGUAL:
                    // Regla 11: operandos de == deben tener el mismo tipo (o int/float compatibles)
                    if (!son_tipos_compatibles_asignacion(t_izq, t_der) && t_izq != t_der) {
                        sem_error(nodo->linea, nodo->columna,
                                  "Regla 11: Los operandos de '==' deben tener el mismo tipo (se recibieron %s y %s).",
                                  TipoDatoAString(t_izq), TipoDatoAString(t_der));
                        nodo->tipo_dato = TIPO_ERROR;
                        return TIPO_ERROR;
                    }
                    nodo->tipo_dato = TIPO_BOOLEAN;
                    return TIPO_BOOLEAN;

                case OP_AND:
                case OP_OR:
                    // Regla 12: operandos de && y || deben ser boolean
                    if (t_izq != TIPO_BOOLEAN || t_der != TIPO_BOOLEAN) {
                        sem_error(nodo->linea, nodo->columna,
                                  "Regla 12: Los operandos de '%s' deben ser de tipo boolean.",
                                  OperadorAString(nodo->op));
                        nodo->tipo_dato = TIPO_ERROR;
                        return TIPO_ERROR;
                    }
                    nodo->tipo_dato = TIPO_BOOLEAN;
                    return TIPO_BOOLEAN;

                default:
                    nodo->tipo_dato = TIPO_ERROR;
                    return TIPO_ERROR;
            }
        }

        case NODO_EXPR_UNARIA: {
            TipoDato t_op = chequear_tipos_nodo(nodo->hijo_izq, metodo_actual);
            if (t_op == TIPO_ERROR) {
                nodo->tipo_dato = TIPO_ERROR;
                return TIPO_ERROR;
            }

            if (nodo->op == OP_MENOS_UNARIO) {
                if (!es_tipo_numerico(t_op)) {
                    sem_error(nodo->linea, nodo->columna, "El operador menos unario requiere operando int o float.");
                    nodo->tipo_dato = TIPO_ERROR;
                    return TIPO_ERROR;
                }
                nodo->tipo_dato = t_op;
                return t_op;
            } else if (nodo->op == OP_NEGACION) {
                // Regla 12: el operando de ! debe ser boolean
                if (t_op != TIPO_BOOLEAN) {
                    sem_error(nodo->linea, nodo->columna, "Regla 12: El operando de la negacion '!' debe ser de tipo boolean.");
                    nodo->tipo_dato = TIPO_ERROR;
                    return TIPO_ERROR;
                }
                nodo->tipo_dato = TIPO_BOOLEAN;
                return TIPO_BOOLEAN;
            }
            return TIPO_ERROR;
        }

        case NODO_ASIGNACION: {
            TipoDato t_var = chequear_tipos_nodo(nodo->hijo_izq, metodo_actual);
            TipoDato t_expr = chequear_tipos_nodo(nodo->hijo_der, metodo_actual);

            // Regla 8: Un <id> usado como <location> debe ser variable o parámetro
            if (nodo->hijo_izq && nodo->hijo_izq->simbolo) {
                if (nodo->hijo_izq->simbolo->flag == FLAG_FUNCION) {
                    sem_error(nodo->linea, nodo->columna, "Regla 8: No se puede asignar a una funcion '%s'.", nodo->hijo_izq->simbolo->nombre);
                }
            }

            // Regla 13 y 14: La asignación debe tener el mismo tipo (o int/float)
            if (!son_tipos_compatibles_asignacion(t_var, t_expr)) {
                sem_error(nodo->linea, nodo->columna,
                          "Regla 13: Tipos incompatibles en la asignacion. Variable de tipo '%s', expresion de tipo '%s'.",
                          TipoDatoAString(t_var), TipoDatoAString(t_expr));
            }
            nodo->tipo_dato = t_var;
            return t_var;
        }

        case NODO_IF: {
            // Regla 9: La expresión de if debe ser boolean
            TipoDato t_cond = chequear_tipos_nodo(nodo->hijo_izq, metodo_actual);
            if (t_cond != TIPO_BOOLEAN && t_cond != TIPO_ERROR) {
                sem_error(nodo->linea, nodo->columna, "Regla 9: La condicion de la sentencia 'if' debe ser de tipo boolean (es %s).", TipoDatoAString(t_cond));
            }
            chequear_tipos_nodo(nodo->hijo_centro, metodo_actual);
            if (nodo->hijo_der) {
                chequear_tipos_nodo(nodo->hijo_der, metodo_actual);
            }
            return TIPO_VOID;
        }

        case NODO_WHILE: {
            // Regla 9: La expresión de while debe ser boolean
            TipoDato t_cond = chequear_tipos_nodo(nodo->hijo_izq, metodo_actual);
            if (t_cond != TIPO_BOOLEAN && t_cond != TIPO_ERROR) {
                sem_error(nodo->linea, nodo->columna, "Regla 9: La condicion de la sentencia 'while' debe ser de tipo boolean (es %s).", TipoDatoAString(t_cond));
            }
            chequear_tipos_nodo(nodo->hijo_centro, metodo_actual);
            return TIPO_VOID;
        }

        case NODO_RETURN: {
            if (metodo_actual != NULL) {
                if (metodo_actual->tipo == TIPO_VOID) {
                    // Regla 6: En método void no puede tener expresión asociada
                    if (nodo->hijo_izq != NULL) {
                        sem_error(nodo->linea, nodo->columna,
                                  "Regla 6: El metodo '%s' es de tipo void y no puede retornar ningun valor.",
                                  metodo_actual->nombre);
                    }
                } else {
                    // Regla 6 y 7: Método no void debe retornar expresión coincidente
                    if (nodo->hijo_izq == NULL) {
                        sem_error(nodo->linea, nodo->columna,
                                  "Regla 6: El metodo '%s' debe retornar un valor de tipo '%s'.",
                                  metodo_actual->nombre, TipoDatoAString(metodo_actual->tipo));
                    } else {
                        TipoDato t_ret = chequear_tipos_nodo(nodo->hijo_izq, metodo_actual);
                        if (!son_tipos_compatibles_asignacion(metodo_actual->tipo, t_ret)) {
                            sem_error(nodo->linea, nodo->columna,
                                      "Regla 7: El tipo retornado '%s' no coincide con el tipo de retorno declarado '%s' en el metodo '%s'.",
                                      TipoDatoAString(t_ret), TipoDatoAString(metodo_actual->tipo), metodo_actual->nombre);
                        }
                    }
                }
            }
            return TIPO_VOID;
        }

        case NODO_EXPR_LLAMADA:
        case NODO_LLAMADA_STMT: {
            Simbolo *m = nodo->simbolo;
            if (!m) return TIPO_ERROR;

            // Regla 5: Si la invocación a un método es usada como una expresión, debe retornar un resultado (no void)
            if (nodo->tipo_nodo == NODO_EXPR_LLAMADA && m->tipo == TIPO_VOID) {
                sem_error(nodo->linea, nodo->columna,
                          "Regla 5: El metodo '%s' es void y no puede ser usado dentro de una expresion.", m->nombre);
                nodo->tipo_dato = TIPO_ERROR;
                return TIPO_ERROR;
            }

            // Regla 4: Verificar cantidad y tipos de los argumentos
            Parametro *p = m->parametros;
            NodoAST *arg_nodo = nodo->hijo_izq;
            int arg_index = 1;

            while (p != NULL && arg_nodo != NULL) {
                TipoDato t_arg = chequear_tipos_nodo(arg_nodo->hijo_izq, metodo_actual);
                if (!son_tipos_compatibles_asignacion(p->tipo, t_arg)) {
                    sem_error(nodo->linea, nodo->columna,
                              "Regla 4: El argumento %d en la llamada a '%s' es de tipo '%s', pero se esperaba '%s'.",
                              arg_index, m->nombre, TipoDatoAString(t_arg), TipoDatoAString(p->tipo));
                }
                p = p->sig;
                arg_nodo = arg_nodo->hijo_der;
                arg_index++;
            }

            if (p != NULL || arg_nodo != NULL) {
                sem_error(nodo->linea, nodo->columna,
                          "Regla 4: Cantidad incorrecta de argumentos en la llamada a '%s'. Esperados: %d.",
                          m->nombre, m->cant_parametros);
            }

            nodo->tipo_dato = m->tipo;
            return m->tipo;
        }

        case NODO_SECUENCIA:
            chequear_tipos_nodo(nodo->hijo_izq, metodo_actual);
            chequear_tipos_nodo(nodo->hijo_der, metodo_actual);
            return TIPO_VOID;

        case NODO_BLOQUE:
            chequear_tipos_nodo(nodo->hijo_izq, metodo_actual);
            return TIPO_VOID;

        default:
            return TIPO_VOID;
    }
}

// Chequea los tipos de todo el programa recorriendo el AST de cada método
int chequear_tipos_programa(TablaSimbolos *ts) {
    if (!ts || !ts->global) return 0;

    Simbolo *s = ts->global->simbolos;
    while (s != NULL) {
        if (s->flag == FLAG_FUNCION && s->ast != NULL) {
            chequear_tipos_nodo(s->ast, s);
        }
        s = s->sig;
    }

    return (total_errores_semanticos == 0);
}
