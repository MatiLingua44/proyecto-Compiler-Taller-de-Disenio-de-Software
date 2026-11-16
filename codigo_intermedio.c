#include "codigo_intermedio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Contexto local para numeración de temporales y etiquetas
typedef struct {
    int contador_registros;
    int contador_etiquetas;
} Contexto3D;

static char* nuevo_registro(Contexto3D *ctx) {
    char buf[32];
    snprintf(buf, sizeof(buf), "R%d", ++(ctx->contador_registros));
    return strdup(buf);
}

static char* nueva_etiqueta(Contexto3D *ctx) {
    char buf[32];
    snprintf(buf, sizeof(buf), "L%d", ++(ctx->contador_etiquetas));
    return strdup(buf);
}

// ------------------------------------------------------------------------------
// Gestión de Instrucciones de 3 Direcciones y Listas Lineales
// ------------------------------------------------------------------------------

static Instruccion3D* crear_instruccion_3d(TipoInstruccion3D tipo, const char *op1, const char *op2, const char *res) {
    Instruccion3D *ins = (Instruccion3D *)malloc(sizeof(Instruccion3D));
    if (!ins) {
        fprintf(stderr, "Error fatal: no se pudo asignar memoria para Instruccion3D.\n");
        exit(EXIT_FAILURE);
    }
    ins->tipo = tipo;
    ins->op1 = op1 ? strdup(op1) : NULL;
    ins->op2 = op2 ? strdup(op2) : NULL;
    ins->resultado = res ? strdup(res) : NULL;
    ins->ant = NULL;
    ins->sig = NULL;
    return ins;
}

static Lista3D* crear_lista_3d(void) {
    Lista3D *lista = (Lista3D *)malloc(sizeof(Lista3D));
    if (!lista) {
        fprintf(stderr, "Error fatal: no se pudo asignar memoria para Lista3D.\n");
        exit(EXIT_FAILURE);
    }
    lista->primero = NULL;
    lista->ultimo = NULL;
    lista->cantidad = 0;
    return lista;
}

static void agregar_instruccion(Lista3D *lista, Instruccion3D *ins) {
    if (!lista || !ins) return;
    if (lista->primero == NULL) {
        lista->primero = ins;
        lista->ultimo = ins;
    } else {
        lista->ultimo->sig = ins;
        ins->ant = lista->ultimo;
        lista->ultimo = ins;
    }
    lista->cantidad++;
}

static void emitir_3d(Lista3D *lista, TipoInstruccion3D tipo, const char *op1, const char *op2, const char *res) {
    agregar_instruccion(lista, crear_instruccion_3d(tipo, op1, op2, res));
}

// ------------------------------------------------------------------------------
// Gestión del Árbol Reescrito (Diapositivas 8 y 10)
// ------------------------------------------------------------------------------

static NodoReescrito* crear_nodo_reescrito(TipoInstruccion3D tipo, const char *op1, const char *op2, const char *res) {
    NodoReescrito *nodo = (NodoReescrito *)malloc(sizeof(NodoReescrito));
    if (!nodo) {
        fprintf(stderr, "Error fatal: no se pudo asignar memoria para NodoReescrito.\n");
        exit(EXIT_FAILURE);
    }
    nodo->tipo = tipo;
    nodo->op1 = op1 ? strdup(op1) : NULL;
    nodo->op2 = op2 ? strdup(op2) : NULL;
    nodo->resultado = res ? strdup(res) : NULL;
    nodo->hijo_izq = NULL;
    nodo->hijo_der = NULL;
    nodo->hijo_centro = NULL;
    nodo->sig_sentencia = NULL;
    return nodo;
}

static void liberar_nodo_reescrito(NodoReescrito *nodo) {
    if (!nodo) return;
    if (nodo->op1) free(nodo->op1);
    if (nodo->op2) free(nodo->op2);
    if (nodo->resultado) free(nodo->resultado);
    liberar_nodo_reescrito(nodo->hijo_izq);
    liberar_nodo_reescrito(nodo->hijo_centro);
    liberar_nodo_reescrito(nodo->hijo_der);
    liberar_nodo_reescrito(nodo->sig_sentencia);
    free(nodo);
}

// ------------------------------------------------------------------------------
// Traducción y Reescritura del AST a Código Intermedio
// ------------------------------------------------------------------------------

static int contar_args_ast(const NodoAST *args) {
    int c = 0;
    const NodoAST *curr = args;
    while (curr != NULL) {
        c++;
        curr = curr->hijo_der;
    }
    return c;
}

// Reescritura de expresiones: reemplaza subárboles de expresiones por nodos de 3 direcciones
static NodoReescrito* reescribir_expresion(const NodoAST *nodo, Contexto3D *ctx) {
    if (!nodo) return NULL;

    switch (nodo->tipo_nodo) {
        case NODO_EXPR_LITERAL: {
            char val_buf[64];
            if (nodo->tipo_dato == TIPO_INT) {
                snprintf(val_buf, sizeof(val_buf), "%d", nodo->valor.val_int);
            } else if (nodo->tipo_dato == TIPO_FLOAT) {
                snprintf(val_buf, sizeof(val_buf), "%.2f", nodo->valor.val_float);
            } else if (nodo->tipo_dato == TIPO_BOOLEAN) {
                snprintf(val_buf, sizeof(val_buf), "%s", nodo->valor.val_bool ? "true" : "false");
            } else {
                snprintf(val_buf, sizeof(val_buf), "0");
            }
            char *reg = nuevo_registro(ctx);
            NodoReescrito *r = crear_nodo_reescrito(INSTR_LOAD_CONST, val_buf, NULL, reg);
            free(reg);
            return r;
        }

        case NODO_EXPR_VAR: {
            const char *var_nom = nodo->simbolo ? nodo->simbolo->nombre : "temp";
            char *reg = nuevo_registro(ctx);
            NodoReescrito *r = crear_nodo_reescrito(INSTR_LOAD_MEM, var_nom, NULL, reg);
            free(reg);
            return r;
        }

        case NODO_EXPR_BINARIA: {
            // Reescritura según Diapositiva 9:
            // Add_Reg Rm, Rn, R  (R := Rm + Rn)
            NodoReescrito *izq = reescribir_expresion(nodo->hijo_izq, ctx);
            NodoReescrito *der = reescribir_expresion(nodo->hijo_der, ctx);
            char *reg_dest = nuevo_registro(ctx);

            TipoInstruccion3D t_ins;
            switch (nodo->op) {
                case OP_SUMA:  t_ins = INSTR_ADD_REG; break;
                case OP_RESTA: t_ins = INSTR_SUB_REG; break;
                case OP_MULT:  t_ins = INSTR_MUL_REG; break;
                case OP_DIV:   t_ins = INSTR_DIV_REG; break;
                case OP_MOD:   t_ins = INSTR_MOD_REG; break;
                case OP_MENOR: t_ins = INSTR_LT_REG;  break;
                case OP_MAYOR: t_ins = INSTR_GT_REG;  break;
                case OP_IGUAL: t_ins = INSTR_EQ_REG;  break;
                case OP_AND:   t_ins = INSTR_AND_REG; break;
                case OP_OR:    t_ins = INSTR_OR_REG;  break;
                default:       t_ins = INSTR_ADD_REG; break;
            }

            NodoReescrito *r = crear_nodo_reescrito(t_ins, izq ? izq->resultado : "R0",
                                                   der ? der->resultado : "R0", reg_dest);
            r->hijo_izq = izq;
            r->hijo_der = der;
            free(reg_dest);
            return r;
        }

        case NODO_EXPR_UNARIA: {
            NodoReescrito *hijo = reescribir_expresion(nodo->hijo_izq, ctx);
            char *reg_dest = nuevo_registro(ctx);
            TipoInstruccion3D t_ins = (nodo->op == OP_NEGACION) ? INSTR_NOT_REG : INSTR_NEG_REG;
            NodoReescrito *r = crear_nodo_reescrito(t_ins, hijo ? hijo->resultado : "R0", NULL, reg_dest);
            r->hijo_izq = hijo;
            free(reg_dest);
            return r;
        }

        case NODO_EXPR_LLAMADA: {
            int num_args = contar_args_ast(nodo->hijo_izq);
            char cant_buf[16];
            snprintf(cant_buf, sizeof(cant_buf), "%d", num_args);
            char *reg_dest = nuevo_registro(ctx);
            const char *func_nom = nodo->simbolo ? nodo->simbolo->nombre : "func";

            NodoReescrito *r = crear_nodo_reescrito(INSTR_CALL, func_nom, cant_buf, reg_dest);

            // Encadenar los argumentos como hijos reescritos
            NodoReescrito *primer_arg = NULL;
            NodoReescrito *ultimo_arg = NULL;
            const NodoAST *curr = nodo->hijo_izq;
            while (curr != NULL) {
                NodoReescrito *arg_r = reescribir_expresion(curr->hijo_izq, ctx);
                if (!primer_arg) {
                    primer_arg = arg_r;
                    ultimo_arg = arg_r;
                } else {
                    ultimo_arg->sig_sentencia = arg_r;
                    ultimo_arg = arg_r;
                }
                curr = curr->hijo_der;
            }
            r->hijo_izq = primer_arg;
            free(reg_dest);
            return r;
        }

        default:
            return NULL;
    }
}

// Linearización en post-orden del árbol reescrito de una expresión
static void linearizar_arbol_expresion(const NodoReescrito *nodo, Lista3D *lista) {
    if (!nodo) return;

    if (nodo->tipo == INSTR_CALL) {
        // Linearizar cada argumento y emitir Param Rn
        const NodoReescrito *arg = nodo->hijo_izq;
        while (arg != NULL) {
            linearizar_arbol_expresion(arg, lista);
            emitir_3d(lista, INSTR_PARAM, arg->resultado, NULL, NULL);
            arg = arg->sig_sentencia;
        }
        // Emitir llamada con resultado en registro
        emitir_3d(lista, INSTR_CALL, nodo->op1, nodo->op2, nodo->resultado);
        return;
    }

    // Recorrido post-orden estándar (hijo_izq, hijo_der, raíz)
    linearizar_arbol_expresion(nodo->hijo_izq, lista);
    linearizar_arbol_expresion(nodo->hijo_der, lista);

    emitir_3d(lista, nodo->tipo, nodo->op1, nodo->op2, nodo->resultado);
}

// Reescritura y linearización de sentencias y bloques del AST
static NodoReescrito* procesar_sentencias(const NodoAST *nodo, Lista3D *lista, Contexto3D *ctx) {
    if (!nodo) return NULL;

    switch (nodo->tipo_nodo) {
        case NODO_SECUENCIA: {
            NodoReescrito *s1 = procesar_sentencias(nodo->hijo_izq, lista, ctx);
            NodoReescrito *s2 = procesar_sentencias(nodo->hijo_der, lista, ctx);
            if (!s1) return s2;
            NodoReescrito *curr = s1;
            while (curr->sig_sentencia != NULL) {
                curr = curr->sig_sentencia;
            }
            curr->sig_sentencia = s2;
            return s1;
        }

        case NODO_BLOQUE: {
            return procesar_sentencias(nodo->hijo_izq, lista, ctx);
        }

        case NODO_ASIGNACION: {
            // Reescritura según Diapositiva 8 y 10:
            // p := p + 5
            // Store_Mem R, p
            //   └── Add_Reg R2, R1, R
            //         ├── Load_Mem p, R2
            //         └── Load_Const 5, R1
            const char *var_nom = (nodo->simbolo != NULL) ? nodo->simbolo->nombre
                                  : (nodo->hijo_izq && nodo->hijo_izq->simbolo) ? nodo->hijo_izq->simbolo->nombre
                                  : "var";

            NodoReescrito *expr_r = reescribir_expresion(nodo->hijo_der, ctx);
            const char *reg_res = expr_r ? expr_r->resultado : "R0";

            NodoReescrito *store_r = crear_nodo_reescrito(INSTR_STORE_MEM, reg_res, NULL, var_nom);
            store_r->hijo_izq = expr_r;

            // Secuencia lineal: linearizar expresión primero, luego Store_Mem
            linearizar_arbol_expresion(expr_r, lista);
            emitir_3d(lista, INSTR_STORE_MEM, reg_res, NULL, var_nom);

            return store_r;
        }

        case NODO_IF: {
            char *lbl_else = (nodo->hijo_der != NULL) ? nueva_etiqueta(ctx) : NULL;
            char *lbl_fin = nueva_etiqueta(ctx);

            NodoReescrito *cond_r = reescribir_expresion(nodo->hijo_izq, ctx);
            const char *reg_cond = cond_r ? cond_r->resultado : "R0";

            // Nodo de control en el árbol reescrito
            const char *lbl_salto = lbl_else ? lbl_else : lbl_fin;
            NodoReescrito *if_r = crear_nodo_reescrito(INSTR_JUMP_FALSE, reg_cond, NULL, lbl_salto);
            if_r->hijo_izq = cond_r;

            // Linearización de la condición
            linearizar_arbol_expresion(cond_r, lista);
            emitir_3d(lista, INSTR_JUMP_FALSE, reg_cond, NULL, lbl_salto);

            // Rama then
            NodoReescrito *then_r = procesar_sentencias(nodo->hijo_centro, lista, ctx);
            if_r->hijo_centro = then_r;

            if (lbl_else != NULL) {
                emitir_3d(lista, INSTR_JUMP, NULL, NULL, lbl_fin);
                emitir_3d(lista, INSTR_LABEL, NULL, NULL, lbl_else);

                // Rama else
                NodoReescrito *else_r = procesar_sentencias(nodo->hijo_der, lista, ctx);
                if_r->hijo_der = else_r;

                emitir_3d(lista, INSTR_LABEL, NULL, NULL, lbl_fin);
                free(lbl_else);
            } else {
                emitir_3d(lista, INSTR_LABEL, NULL, NULL, lbl_fin);
            }

            free(lbl_fin);
            return if_r;
        }

        case NODO_WHILE: {
            char *lbl_inicio = nueva_etiqueta(ctx);
            char *lbl_fin = nueva_etiqueta(ctx);

            emitir_3d(lista, INSTR_LABEL, NULL, NULL, lbl_inicio);

            NodoReescrito *cond_r = reescribir_expresion(nodo->hijo_izq, ctx);
            const char *reg_cond = cond_r ? cond_r->resultado : "R0";

            linearizar_arbol_expresion(cond_r, lista);
            emitir_3d(lista, INSTR_JUMP_FALSE, reg_cond, NULL, lbl_fin);

            NodoReescrito *while_r = crear_nodo_reescrito(INSTR_JUMP_FALSE, reg_cond, NULL, lbl_fin);
            while_r->hijo_izq = cond_r;

            // Cuerpo del bucle
            NodoReescrito *cuerpo_r = procesar_sentencias(nodo->hijo_centro, lista, ctx);
            while_r->hijo_centro = cuerpo_r;

            emitir_3d(lista, INSTR_JUMP, NULL, NULL, lbl_inicio);
            emitir_3d(lista, INSTR_LABEL, NULL, NULL, lbl_fin);

            free(lbl_inicio);
            free(lbl_fin);
            return while_r;
        }

        case NODO_RETURN: {
            if (nodo->hijo_izq != NULL) {
                NodoReescrito *expr_r = reescribir_expresion(nodo->hijo_izq, ctx);
                linearizar_arbol_expresion(expr_r, lista);
                emitir_3d(lista, INSTR_RETURN_REG, expr_r ? expr_r->resultado : "R0", NULL, NULL);

                NodoReescrito *ret_r = crear_nodo_reescrito(INSTR_RETURN_REG, expr_r ? expr_r->resultado : "R0", NULL, NULL);
                ret_r->hijo_izq = expr_r;
                return ret_r;
            } else {
                emitir_3d(lista, INSTR_RETURN, NULL, NULL, NULL);
                return crear_nodo_reescrito(INSTR_RETURN, NULL, NULL, NULL);
            }
        }

        case NODO_LLAMADA_STMT: {
            int num_args = contar_args_ast(nodo->hijo_izq);
            char cant_buf[16];
            snprintf(cant_buf, sizeof(cant_buf), "%d", num_args);
            const char *func_nom = nodo->simbolo ? nodo->simbolo->nombre : "func";

            // Linearizar argumentos y emitir Param
            const NodoAST *curr = nodo->hijo_izq;
            NodoReescrito *primer_arg = NULL;
            NodoReescrito *ultimo_arg = NULL;
            while (curr != NULL) {
                NodoReescrito *arg_r = reescribir_expresion(curr->hijo_izq, ctx);
                linearizar_arbol_expresion(arg_r, lista);
                emitir_3d(lista, INSTR_PARAM, arg_r ? arg_r->resultado : "R0", NULL, NULL);

                if (!primer_arg) {
                    primer_arg = arg_r;
                    ultimo_arg = arg_r;
                } else {
                    ultimo_arg->sig_sentencia = arg_r;
                    ultimo_arg = arg_r;
                }
                curr = curr->hijo_der;
            }

            emitir_3d(lista, INSTR_CALL, func_nom, cant_buf, NULL);

            NodoReescrito *call_r = crear_nodo_reescrito(INSTR_CALL, func_nom, cant_buf, NULL);
            call_r->hijo_izq = primer_arg;
            return call_r;
        }

        default:
            return NULL;
    }
}

// ------------------------------------------------------------------------------
// Generación completa del Código Intermedio para todo el programa
// ------------------------------------------------------------------------------

CodigoIntermedio* generar_codigo_intermedio(const TablaSimbolos *ts) {
    if (!ts || !ts->global) return NULL;

    CodigoIntermedio *ci = (CodigoIntermedio *)malloc(sizeof(CodigoIntermedio));
    if (!ci) {
        fprintf(stderr, "Error fatal: no se pudo asignar memoria para CodigoIntermedio.\n");
        exit(EXIT_FAILURE);
    }
    ci->metodos = NULL;
    ci->total_metodos = 0;

    Contexto3D ctx;
    ctx.contador_etiquetas = 0;

    Metodo3D *ultimo_metodo = NULL;

    // Recorrer los símbolos globales buscando todas las funciones
    Simbolo *curr = ts->global->simbolos;
    while (curr != NULL) {
        if (curr->flag == FLAG_FUNCION && curr->ast != NULL) {
            // Reiniciar contador de registros por cada función para máxima claridad
            ctx.contador_registros = 0;

            Metodo3D *m = (Metodo3D *)malloc(sizeof(Metodo3D));
            m->nombre = strdup(curr->nombre);
            m->tipo_retorno = curr->tipo;
            m->instrucciones = crear_lista_3d();
            m->sig = NULL;

            // Emitir Begin_Func
            emitir_3d(m->instrucciones, INSTR_BEGIN_FUNC, NULL, NULL, m->nombre);

            // Reescritura del cuerpo de la función y linearización
            m->arbol_reescrito = procesar_sentencias(curr->ast, m->instrucciones, &ctx);

            // Si es void y no termina en return explícito, emitir Return
            if (m->tipo_retorno == TIPO_VOID) {
                if (m->instrucciones->ultimo == NULL ||
                    m->instrucciones->ultimo->tipo != INSTR_RETURN) {
                    emitir_3d(m->instrucciones, INSTR_RETURN, NULL, NULL, NULL);
                }
            }

            // Emitir End_Func
            emitir_3d(m->instrucciones, INSTR_END_FUNC, NULL, NULL, m->nombre);

            // Enlazar método en la lista
            if (!ci->metodos) {
                ci->metodos = m;
                ultimo_metodo = m;
            } else {
                ultimo_metodo->sig = m;
                ultimo_metodo = m;
            }
            ci->total_metodos++;
        }
        curr = curr->sig;
    }

    return ci;
}

// ------------------------------------------------------------------------------
// Formateo y Visualización de Código de Tres Direcciones
// ------------------------------------------------------------------------------

const char* tipo_instr_a_string(TipoInstruccion3D tipo) {
    switch (tipo) {
        case INSTR_LOAD_CONST: return "Load_Const";
        case INSTR_LOAD_MEM:   return "Load_Mem";
        case INSTR_STORE_MEM:  return "Store_Mem";
        case INSTR_ADD_REG:    return "Add_Reg";
        case INSTR_SUB_REG:    return "Sub_Reg";
        case INSTR_MUL_REG:    return "Mul_Reg";
        case INSTR_DIV_REG:    return "Div_Reg";
        case INSTR_MOD_REG:    return "Mod_Reg";
        case INSTR_NEG_REG:    return "Neg_Reg";
        case INSTR_NOT_REG:    return "Not_Reg";
        case INSTR_LT_REG:     return "Lt_Reg";
        case INSTR_GT_REG:     return "Gt_Reg";
        case INSTR_EQ_REG:     return "Eq_Reg";
        case INSTR_AND_REG:    return "And_Reg";
        case INSTR_OR_REG:     return "Or_Reg";
        case INSTR_LABEL:      return "Label";
        case INSTR_JUMP:       return "Jump";
        case INSTR_JUMP_FALSE: return "Jump_False";
        case INSTR_JUMP_TRUE:  return "Jump_True";
        case INSTR_BEGIN_FUNC: return "Begin_Func";
        case INSTR_END_FUNC:   return "End_Func";
        case INSTR_PARAM:      return "Param";
        case INSTR_CALL:       return "Call";
        case INSTR_RETURN_REG: return "Return_Reg";
        case INSTR_RETURN:     return "Return";
        default:               return "NOP";
    }
}

void formatear_instruccion_3d(const Instruccion3D *ins, char *buffer, size_t max_len) {
    if (!ins || !buffer) return;

    switch (ins->tipo) {
        case INSTR_LOAD_CONST:
            snprintf(buffer, max_len, "Load_Const %s, %s", ins->op1 ? ins->op1 : "0", ins->resultado ? ins->resultado : "R");
            break;
        case INSTR_LOAD_MEM:
            snprintf(buffer, max_len, "Load_Mem %s, %s", ins->op1 ? ins->op1 : "x", ins->resultado ? ins->resultado : "R");
            break;
        case INSTR_STORE_MEM:
            snprintf(buffer, max_len, "Store_Mem %s, %s", ins->op1 ? ins->op1 : "R", ins->resultado ? ins->resultado : "x");
            break;
        case INSTR_ADD_REG:
            snprintf(buffer, max_len, "Add_Reg %s, %s, %s", ins->op1 ? ins->op1 : "R1", ins->op2 ? ins->op2 : "R2", ins->resultado ? ins->resultado : "R3");
            break;
        case INSTR_SUB_REG:
            snprintf(buffer, max_len, "Sub_Reg %s, %s, %s", ins->op1 ? ins->op1 : "R1", ins->op2 ? ins->op2 : "R2", ins->resultado ? ins->resultado : "R3");
            break;
        case INSTR_MUL_REG:
            snprintf(buffer, max_len, "Mul_Reg %s, %s, %s", ins->op1 ? ins->op1 : "R1", ins->op2 ? ins->op2 : "R2", ins->resultado ? ins->resultado : "R3");
            break;
        case INSTR_DIV_REG:
            snprintf(buffer, max_len, "Div_Reg %s, %s, %s", ins->op1 ? ins->op1 : "R1", ins->op2 ? ins->op2 : "R2", ins->resultado ? ins->resultado : "R3");
            break;
        case INSTR_MOD_REG:
            snprintf(buffer, max_len, "Mod_Reg %s, %s, %s", ins->op1 ? ins->op1 : "R1", ins->op2 ? ins->op2 : "R2", ins->resultado ? ins->resultado : "R3");
            break;
        case INSTR_NEG_REG:
            snprintf(buffer, max_len, "Neg_Reg %s, %s", ins->op1 ? ins->op1 : "R1", ins->resultado ? ins->resultado : "R2");
            break;
        case INSTR_NOT_REG:
            snprintf(buffer, max_len, "Not_Reg %s, %s", ins->op1 ? ins->op1 : "R1", ins->resultado ? ins->resultado : "R2");
            break;
        case INSTR_LT_REG:
            snprintf(buffer, max_len, "Lt_Reg %s, %s, %s", ins->op1 ? ins->op1 : "R1", ins->op2 ? ins->op2 : "R2", ins->resultado ? ins->resultado : "R3");
            break;
        case INSTR_GT_REG:
            snprintf(buffer, max_len, "Gt_Reg %s, %s, %s", ins->op1 ? ins->op1 : "R1", ins->op2 ? ins->op2 : "R2", ins->resultado ? ins->resultado : "R3");
            break;
        case INSTR_EQ_REG:
            snprintf(buffer, max_len, "Eq_Reg %s, %s, %s", ins->op1 ? ins->op1 : "R1", ins->op2 ? ins->op2 : "R2", ins->resultado ? ins->resultado : "R3");
            break;
        case INSTR_AND_REG:
            snprintf(buffer, max_len, "And_Reg %s, %s, %s", ins->op1 ? ins->op1 : "R1", ins->op2 ? ins->op2 : "R2", ins->resultado ? ins->resultado : "R3");
            break;
        case INSTR_OR_REG:
            snprintf(buffer, max_len, "Or_Reg %s, %s, %s", ins->op1 ? ins->op1 : "R1", ins->op2 ? ins->op2 : "R2", ins->resultado ? ins->resultado : "R3");
            break;
        case INSTR_LABEL:
            snprintf(buffer, max_len, "Label %s:", ins->resultado ? ins->resultado : "L");
            break;
        case INSTR_JUMP:
            snprintf(buffer, max_len, "Jump %s", ins->resultado ? ins->resultado : "L");
            break;
        case INSTR_JUMP_FALSE:
            snprintf(buffer, max_len, "Jump_False %s, %s", ins->op1 ? ins->op1 : "R", ins->resultado ? ins->resultado : "L");
            break;
        case INSTR_JUMP_TRUE:
            snprintf(buffer, max_len, "Jump_True %s, %s", ins->op1 ? ins->op1 : "R", ins->resultado ? ins->resultado : "L");
            break;
        case INSTR_BEGIN_FUNC:
            snprintf(buffer, max_len, "Begin_Func %s", ins->resultado ? ins->resultado : "func");
            break;
        case INSTR_END_FUNC:
            snprintf(buffer, max_len, "End_Func %s", ins->resultado ? ins->resultado : "func");
            break;
        case INSTR_PARAM:
            snprintf(buffer, max_len, "Param %s", ins->op1 ? ins->op1 : "R");
            break;
        case INSTR_CALL:
            if (ins->resultado != NULL) {
                snprintf(buffer, max_len, "Call %s, %s, %s", ins->op1 ? ins->op1 : "func", ins->op2 ? ins->op2 : "0", ins->resultado);
            } else {
                snprintf(buffer, max_len, "Call %s, %s", ins->op1 ? ins->op1 : "func", ins->op2 ? ins->op2 : "0");
            }
            break;
        case INSTR_RETURN_REG:
            snprintf(buffer, max_len, "Return_Reg %s", ins->op1 ? ins->op1 : "R");
            break;
        case INSTR_RETURN:
            snprintf(buffer, max_len, "Return");
            break;
        default:
            snprintf(buffer, max_len, "NOP");
            break;
    }
}

// Imprime la secuencia lineal de instrucciones de tres direcciones
void imprimir_codigo_intermedio(const CodigoIntermedio *ci, FILE *salida) {
    if (!ci) return;
    FILE *out = salida ? salida : stdout;

    Metodo3D *m = ci->metodos;
    while (m != NULL) {
        Instruccion3D *ins = m->instrucciones ? m->instrucciones->primero : NULL;
        char buf[128];
        while (ins != NULL) {
            formatear_instruccion_3d(ins, buf, sizeof(buf));
            if (ins->tipo == INSTR_BEGIN_FUNC || ins->tipo == INSTR_END_FUNC) {
                fprintf(out, "%s\n", buf);
            } else if (ins->tipo == INSTR_LABEL) {
                fprintf(out, "%s\n", buf);
            } else {
                fprintf(out, "    %s\n", buf);
            }
            ins = ins->sig;
        }
        fprintf(out, "\n");
        m = m->sig;
    }
}

// ------------------------------------------------------------------------------
// Impresión jerárquica del Árbol Reescrito (Diapositivas 8 y 10)
// ------------------------------------------------------------------------------

static void imprimir_arbol_reescrito_rec(const NodoReescrito *nodo, FILE *out, const char *prefijo, int es_ultimo) {
    if (!nodo) return;

    fprintf(out, "%s%s [ %s", prefijo, es_ultimo ? "└──" : "├──", tipo_instr_a_string(nodo->tipo));
    if (nodo->tipo == INSTR_STORE_MEM) {
        fprintf(out, " %s, %s ]\n", nodo->op1 ? nodo->op1 : "", nodo->resultado ? nodo->resultado : "");
    } else if (nodo->tipo == INSTR_LOAD_MEM || nodo->tipo == INSTR_LOAD_CONST) {
        fprintf(out, " %s, %s ]\n", nodo->op1 ? nodo->op1 : "", nodo->resultado ? nodo->resultado : "");
    } else if (nodo->tipo == INSTR_CALL) {
        if (nodo->resultado) {
            fprintf(out, " %s, %s, %s ]\n", nodo->op1 ? nodo->op1 : "", nodo->op2 ? nodo->op2 : "0", nodo->resultado);
        } else {
            fprintf(out, " %s, %s ]\n", nodo->op1 ? nodo->op1 : "", nodo->op2 ? nodo->op2 : "0");
        }
    } else if (nodo->tipo == INSTR_RETURN_REG) {
        fprintf(out, " %s ]\n", nodo->op1 ? nodo->op1 : "");
    } else if (nodo->tipo == INSTR_JUMP_FALSE) {
        fprintf(out, " %s, %s ]\n", nodo->op1 ? nodo->op1 : "", nodo->resultado ? nodo->resultado : "");
    } else if (nodo->tipo == INSTR_RETURN) {
        fprintf(out, " ]\n");
    } else {
        if (nodo->op1 && nodo->op2 && nodo->resultado) {
            fprintf(out, " %s, %s, %s ]\n", nodo->op1, nodo->op2, nodo->resultado);
        } else if (nodo->resultado) {
            fprintf(out, " %s ]\n", nodo->resultado);
        } else {
            fprintf(out, " ]\n");
        }
    }

    char nuevo_prefijo[512];
    snprintf(nuevo_prefijo, sizeof(nuevo_prefijo), "%s%s", prefijo, es_ultimo ? "    " : "│   ");

    // Contar hijos directos
    int cant_hijos = 0;
    if (nodo->hijo_izq) cant_hijos++;
    if (nodo->hijo_centro) cant_hijos++;
    if (nodo->hijo_der) cant_hijos++;

    int h = 0;
    if (nodo->hijo_izq) {
        h++;
        imprimir_arbol_reescrito_rec(nodo->hijo_izq, out, nuevo_prefijo, h == cant_hijos);
    }
    if (nodo->hijo_centro) {
        h++;
        imprimir_arbol_reescrito_rec(nodo->hijo_centro, out, nuevo_prefijo, h == cant_hijos);
    }
    if (nodo->hijo_der) {
        h++;
        imprimir_arbol_reescrito_rec(nodo->hijo_der, out, nuevo_prefijo, h == cant_hijos);
    }

    if (nodo->sig_sentencia) {
        imprimir_arbol_reescrito_rec(nodo->sig_sentencia, out, prefijo, es_ultimo);
    }
}

void imprimir_arbol_reescrito_todos(const CodigoIntermedio *ci, FILE *salida) {
    if (!ci) return;
    FILE *out = salida ? salida : stdout;

    fprintf(out, "\n======================================================================\n");
    fprintf(out, "              ARBOLES REESCRITOS (CODIGO INTERMEDIO)\n");
    fprintf(out, "        (Reescritura del AST a segmentos de tres direcciones)\n");
    fprintf(out, "======================================================================\n");

    Metodo3D *m = ci->metodos;
    while (m != NULL) {
        fprintf(out, "\nArbol reescrito de la funcion '%s' (tipo retorno: %s):\n",
                m->nombre, TipoDatoAString(m->tipo_retorno));
        if (m->arbol_reescrito != NULL) {
            imprimir_arbol_reescrito_rec(m->arbol_reescrito, out, "", 1);
        } else {
            fprintf(out, "    (Cuerpo vacio)\n");
        }
        m = m->sig;
    }
    fprintf(out, "======================================================================\n\n");
}

// ------------------------------------------------------------------------------
// Exportación Graphviz DOT del Código Intermedio / Árbol Reescrito
// ------------------------------------------------------------------------------

static int generar_nodo_ci_dot_rec(const NodoReescrito *nodo, FILE *f, int *id_counter) {
    if (!nodo) return -1;

    int mi_id = (*id_counter)++;
    char label_buf[128];

    switch (nodo->tipo) {
        case INSTR_STORE_MEM:
            snprintf(label_buf, sizeof(label_buf), "Store_Mem\\n%s, %s",
                     nodo->op1 ? nodo->op1 : "", nodo->resultado ? nodo->resultado : "");
            fprintf(f, "    ci%d [label=\"%s\", shape=box, style=\"filled,rounded\", fillcolor=\"#FFF3BF\", color=\"#FAB005\"];\n", mi_id, label_buf);
            break;
        case INSTR_LOAD_MEM:
            snprintf(label_buf, sizeof(label_buf), "Load_Mem\\n%s, %s",
                     nodo->op1 ? nodo->op1 : "", nodo->resultado ? nodo->resultado : "");
            fprintf(f, "    ci%d [label=\"%s\", shape=ellipse, style=\"filled\", fillcolor=\"#D3F9D8\", color=\"#40C057\"];\n", mi_id, label_buf);
            break;
        case INSTR_LOAD_CONST:
            snprintf(label_buf, sizeof(label_buf), "Load_Const\\n%s, %s",
                     nodo->op1 ? nodo->op1 : "", nodo->resultado ? nodo->resultado : "");
            fprintf(f, "    ci%d [label=\"%s\", shape=ellipse, style=\"filled\", fillcolor=\"#FFF9DB\", color=\"#F59F00\"];\n", mi_id, label_buf);
            break;
        case INSTR_ADD_REG:
        case INSTR_SUB_REG:
        case INSTR_MUL_REG:
        case INSTR_DIV_REG:
        case INSTR_MOD_REG:
            snprintf(label_buf, sizeof(label_buf), "%s\\n%s, %s, %s",
                     tipo_instr_a_string(nodo->tipo), nodo->op1 ? nodo->op1 : "", nodo->op2 ? nodo->op2 : "", nodo->resultado ? nodo->resultado : "");
            fprintf(f, "    ci%d [label=\"%s\", shape=box, style=\"filled,rounded\", fillcolor=\"#E7F5FF\", color=\"#339AF0\"];\n", mi_id, label_buf);
            break;
        case INSTR_JUMP_FALSE:
            snprintf(label_buf, sizeof(label_buf), "Jump_False %s\\n-> %s",
                     nodo->op1 ? nodo->op1 : "", nodo->resultado ? nodo->resultado : "");
            fprintf(f, "    ci%d [label=\"%s\", shape=diamond, style=\"filled\", fillcolor=\"#FFE3E3\", color=\"#FA5252\"];\n", mi_id, label_buf);
            break;
        case INSTR_RETURN_REG:
        case INSTR_RETURN:
            snprintf(label_buf, sizeof(label_buf), "Return %s", nodo->op1 ? nodo->op1 : "");
            fprintf(f, "    ci%d [label=\"%s\", shape=box, style=\"filled,rounded\", fillcolor=\"#C5F6FA\", color=\"#15AABF\"];\n", mi_id, label_buf);
            break;
        case INSTR_CALL:
            snprintf(label_buf, sizeof(label_buf), "Call %s\\nargs: %s -> %s",
                     nodo->op1 ? nodo->op1 : "", nodo->op2 ? nodo->op2 : "0", nodo->resultado ? nodo->resultado : "void");
            fprintf(f, "    ci%d [label=\"%s\", shape=box, style=\"filled,rounded\", fillcolor=\"#EBE4FF\", color=\"#7950F2\"];\n", mi_id, label_buf);
            break;
        default:
            snprintf(label_buf, sizeof(label_buf), "%s", tipo_instr_a_string(nodo->tipo));
            fprintf(f, "    ci%d [label=\"%s\", shape=box, style=\"filled\"];\n", mi_id, label_buf);
            break;
    }

    if (nodo->hijo_izq) {
        int id_izq = generar_nodo_ci_dot_rec(nodo->hijo_izq, f, id_counter);
        if (id_izq >= 0) fprintf(f, "    ci%d -> ci%d;\n", mi_id, id_izq);
    }
    if (nodo->hijo_centro) {
        int id_c = generar_nodo_ci_dot_rec(nodo->hijo_centro, f, id_counter);
        if (id_c >= 0) fprintf(f, "    ci%d -> ci%d [label=\"then/body\"];\n", mi_id, id_c);
    }
    if (nodo->hijo_der) {
        int id_der = generar_nodo_ci_dot_rec(nodo->hijo_der, f, id_counter);
        if (id_der >= 0) fprintf(f, "    ci%d -> ci%d [label=\"else\"];\n", mi_id, id_der);
    }
    if (nodo->sig_sentencia) {
        int id_sig = generar_nodo_ci_dot_rec(nodo->sig_sentencia, f, id_counter);
        if (id_sig >= 0) fprintf(f, "    ci%d -> ci%d [style=dashed, label=\"sig\"];\n", mi_id, id_sig);
    }

    return mi_id;
}

void generar_ci_dot(const CodigoIntermedio *ci, const char *nombre_archivo) {
    if (!ci || !nombre_archivo) return;

    FILE *f = fopen(nombre_archivo, "w");
    if (!f) {
        fprintf(stderr, "Error al abrir '%s' para escribir DOT de Código Intermedio.\n", nombre_archivo);
        return;
    }

    fprintf(f, "digraph CodigoIntermedio {\n");
    fprintf(f, "    rankdir=TB;\n");
    fprintf(f, "    node [fontname=\"Helvetica\", fontsize=10];\n");
    fprintf(f, "    edge [fontname=\"Helvetica\", fontsize=9];\n");

    int id_counter = 0;
    Metodo3D *m = ci->metodos;
    int cluster_idx = 0;
    while (m != NULL) {
        fprintf(f, "\n    subgraph cluster_%d {\n", cluster_idx++);
        fprintf(f, "        label=\"Funcion: %s()\";\n", m->nombre);
        fprintf(f, "        style=\"filled,rounded\";\n");
        fprintf(f, "        fillcolor=\"#F8F9FA\";\n");
        fprintf(f, "        color=\"#ADB5BD\";\n");

        if (m->arbol_reescrito != NULL) {
            generar_nodo_ci_dot_rec(m->arbol_reescrito, f, &id_counter);
        } else {
            fprintf(f, "        vacio_%d [label=\"(Cuerpo vacio)\", shape=plaintext];\n", cluster_idx);
        }
        fprintf(f, "    }\n");
        m = m->sig;
    }

    fprintf(f, "}\n");
    fclose(f);
}

// ------------------------------------------------------------------------------
// Liberación de Memoria
// ------------------------------------------------------------------------------

void liberar_codigo_intermedio(CodigoIntermedio *ci) {
    if (!ci) return;

    Metodo3D *m = ci->metodos;
    while (m != NULL) {
        Metodo3D *sig_m = m->sig;

        if (m->nombre) free(m->nombre);
        if (m->arbol_reescrito) {
            liberar_nodo_reescrito(m->arbol_reescrito);
        }

        if (m->instrucciones) {
            Instruccion3D *ins = m->instrucciones->primero;
            while (ins != NULL) {
                Instruccion3D *sig_ins = ins->sig;
                if (ins->op1) free(ins->op1);
                if (ins->op2) free(ins->op2);
                if (ins->resultado) free(ins->resultado);
                free(ins);
                ins = sig_ins;
            }
            free(m->instrucciones);
        }

        free(m);
        m = sig_m;
    }

    free(ci);
}
