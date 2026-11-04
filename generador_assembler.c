#include "generador_assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * ==============================================================================
 * GENERADOR DE CÓDIGO ASSEMBLER x86-64
 * Basado estrictamente en:
 * 1. "Tutorial Code Assembly Generation" (C-TDS x86-64 subset)
 * 2. "CSCI-2500 X86 Machine-Level Programming" (AMD64 / GNU Assembler)
 *
 * Convención y Reglas de la Guía:
 * - Marco de Pila:
 *     enter $(8 * N), $0   (reserva N palabras cuádruples de 8 bytes para locales y temporales)
 *     leave                (restaura el marco anterior)
 *     ret                  (retorno al llamador)
 * - Registros temporales para evaluación de expresiones:
 *     %r10 y %r11 exclusivamente (sin asignación compleja de registros).
 * - Convención de llamadas (Linux AMD64 ABI):
 *     Argumentos 1 a 6: %rdi, %rsi, %rdx, %rcx, %r8, %r9 (64 bits).
 *     Valor de retorno: %rax (64 bits).
 *     Antes de llamar funciones (call), %rax = $0.
 * - Operaciones booleanas y condicionales:
 *     Uso de cmp con cmove, cmovne, cmovg, cmovl, etc.
 * - Soporte para llamadas a la librería estándar del curso: printInt(x) y get_int().
 * ==============================================================================
 */

static const char *REGISTROS_ARG_64[] = { "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9" };
#define MAX_ARGS_REGISTROS 6

typedef struct {
    char *nombre;
    int offset;    // Offset positivo relativo a %rbp (se usa -offset(%rbp))
    int es_param;  // 1 si es parámetro formal, 0 si es variable local
} SimboloLocal;

typedef struct {
    char *nombre_func;
    SimboloLocal locales[256];
    int cant_locales;

    // Offsets de registros virtuales R1, R2, ..., R<max_reg>
    int reg_offsets[512];
    int max_reg;

    int total_quadwords; // N para enter $(8 * N), $0
    TipoDato tipo_retorno;
} ContextoFuncionASM;

// ------------------------------------------------------------------------------
// Funciones auxiliares de resolución de operandos y offsets
// ------------------------------------------------------------------------------

static int obtener_indice_registro(const char *reg_str) {
    if (!reg_str) return -1;
    if (reg_str[0] == 'R' || reg_str[0] == 'r') {
        int idx = atoi(reg_str + 1);
        return idx;
    }
    return -1;
}

static int buscar_offset_local(const ContextoFuncionASM *ctx, const char *nombre) {
    if (!ctx || !nombre) return -1;
    for (int i = 0; i < ctx->cant_locales; i++) {
        if (strcmp(ctx->locales[i].nombre, nombre) == 0) {
            return ctx->locales[i].offset;
        }
    }
    return -1;
}

static int es_variable_global(const TablaSimbolos *ts, const char *nombre) {
    if (!ts || !ts->global || !nombre) return 0;
    Simbolo *curr = ts->global->simbolos;
    while (curr != NULL) {
        if (curr->flag == FLAG_VARIABLE && strcmp(curr->nombre, nombre) == 0) {
            return 1;
        }
        curr = curr->sig;
    }
    return 0;
}

static void obtener_operando_mem(const ContextoFuncionASM *ctx, const TablaSimbolos *ts,
                                 const char *nombre, char *buffer, size_t max_len) {
    if (!nombre) {
        snprintf(buffer, max_len, "$0");
        return;
    }

    // 1. ¿Es un registro virtual R1, R2, ...?
    int idx_reg = obtener_indice_registro(nombre);
    if (idx_reg > 0 && idx_reg < 512) {
        int off = ctx->reg_offsets[idx_reg];
        if (off > 0) {
            snprintf(buffer, max_len, "-%d(%%rbp)", off);
            return;
        }
    }

    // 2. ¿Es un parámetro formal o variable local de la función (shadowing)?
    int off_local = buscar_offset_local(ctx, nombre);
    if (off_local != -1) {
        snprintf(buffer, max_len, "-%d(%%rbp)", off_local);
        return;
    }

    // 3. ¿Es una variable global? (Direccionamiento relativo a RIP)
    if (es_variable_global(ts, nombre)) {
        snprintf(buffer, max_len, "%s(%%rip)", nombre);
        return;
    }

    // 4. Fallback si no fue encontrada
    snprintf(buffer, max_len, "-%d(%%rbp)", (ctx->total_quadwords > 0 ? ctx->total_quadwords : 1) * 8);
}

// Recorrido recursivo del AST para registrar todas las variables locales
static void recolectar_simbolos_ast(const NodoAST *nodo, ContextoFuncionASM *ctx, int *offset_actual) {
    if (!nodo) return;

    if (nodo->tipo_nodo == NODO_EXPR_VAR) {
        if (nodo->simbolo != NULL && nodo->simbolo->flag == FLAG_VARIABLE && nodo->simbolo->nivel > 1) {
            const char *nom = nodo->simbolo->nombre;
            if (buscar_offset_local(ctx, nom) == -1 && ctx->cant_locales < 256) {
                *offset_actual += 8;
                ctx->locales[ctx->cant_locales].nombre = strdup(nom);
                ctx->locales[ctx->cant_locales].offset = *offset_actual;
                ctx->locales[ctx->cant_locales].es_param = 0;
                ctx->cant_locales++;
            }
        }
    } else if (nodo->tipo_nodo == NODO_ASIGNACION) {
        Simbolo *s = nodo->simbolo;
        if (!s && nodo->hijo_izq && nodo->hijo_izq->simbolo) {
            s = nodo->hijo_izq->simbolo;
        }
        if (s != NULL && s->flag == FLAG_VARIABLE && s->nivel > 1) {
            const char *nom = s->nombre;
            if (buscar_offset_local(ctx, nom) == -1 && ctx->cant_locales < 256) {
                *offset_actual += 8;
                ctx->locales[ctx->cant_locales].nombre = strdup(nom);
                ctx->locales[ctx->cant_locales].offset = *offset_actual;
                ctx->locales[ctx->cant_locales].es_param = 0;
                ctx->cant_locales++;
            }
        }
    }

    recolectar_simbolos_ast(nodo->hijo_izq, ctx, offset_actual);
    recolectar_simbolos_ast(nodo->hijo_centro, ctx, offset_actual);
    recolectar_simbolos_ast(nodo->hijo_der, ctx, offset_actual);
}

// Limpia el nombre de una etiqueta quitando dos puntos finales si existen
static void limpiar_etiqueta(const char *origen, char *destino, size_t max_len) {
    if (!origen || !destino) return;
    strncpy(destino, origen, max_len - 1);
    destino[max_len - 1] = '\0';
    char *colon = strchr(destino, ':');
    if (colon) *colon = '\0';
}

// ------------------------------------------------------------------------------
// Generación del cuerpo de una función en Assembler x86-64
// ------------------------------------------------------------------------------

static void generar_funcion_asm(const Metodo3D *m, const TablaSimbolos *ts, FILE *salida) {
    if (!m) return;

    ContextoFuncionASM ctx;
    memset(&ctx, 0, sizeof(ContextoFuncionASM));
    ctx.nombre_func = m->nombre;
    ctx.tipo_retorno = m->tipo_retorno;
    ctx.max_reg = 0;

    int offset_actual = 0;

    // 1. Buscar la declaración de la función en la Tabla de Símbolos
    Simbolo *func_sym = NULL;
    if (ts && ts->global) {
        Simbolo *curr = ts->global->simbolos;
        while (curr != NULL) {
            if (curr->flag == FLAG_FUNCION && strcmp(curr->nombre, m->nombre) == 0) {
                func_sym = curr;
                break;
            }
            curr = curr->sig;
        }
    }

    // 2. Registrar los parámetros formales en el marco de pila (-8, -16, ...)
    if (func_sym != NULL) {
        Parametro *p = func_sym->parametros;
        while (p != NULL) {
            if (buscar_offset_local(&ctx, p->nombre) == -1 && ctx.cant_locales < 256) {
                offset_actual += 8;
                ctx.locales[ctx.cant_locales].nombre = strdup(p->nombre);
                ctx.locales[ctx.cant_locales].offset = offset_actual;
                ctx.locales[ctx.cant_locales].es_param = 1;
                ctx.cant_locales++;
            }
            p = p->sig;
        }

        // 3. Recolectar variables locales del AST del método
        recolectar_simbolos_ast(func_sym->ast, &ctx, &offset_actual);
    }

    // 4. Analizar la lista de instrucciones 3D para registrar variables adicionales y temporales
    Instruccion3D *ins = m->instrucciones ? m->instrucciones->primero : NULL;
    while (ins != NULL) {
        const char *var_candidata = NULL;
        if (ins->tipo == INSTR_LOAD_MEM) {
            var_candidata = ins->op1;
        } else if (ins->tipo == INSTR_STORE_MEM) {
            var_candidata = ins->resultado;
        }

        if (var_candidata != NULL) {
            if (buscar_offset_local(&ctx, var_candidata) == -1 &&
                !es_variable_global(ts, var_candidata) &&
                ctx.cant_locales < 256) {
                offset_actual += 8;
                ctx.locales[ctx.cant_locales].nombre = strdup(var_candidata);
                ctx.locales[ctx.cant_locales].offset = offset_actual;
                ctx.locales[ctx.cant_locales].es_param = 0;
                ctx.cant_locales++;
            }
        }

        int r1 = obtener_indice_registro(ins->op1);
        int r2 = obtener_indice_registro(ins->op2);
        int r3 = obtener_indice_registro(ins->resultado);
        if (r1 > ctx.max_reg) ctx.max_reg = r1;
        if (r2 > ctx.max_reg) ctx.max_reg = r2;
        if (r3 > ctx.max_reg) ctx.max_reg = r3;

        ins = ins->sig;
    }

    // 5. Asignar posiciones en la pila para todos los registros temporales R1 .. R<max_reg>
    for (int k = 1; k <= ctx.max_reg && k < 512; k++) {
        offset_actual += 8;
        ctx.reg_offsets[k] = offset_actual;
    }

    // 6. Calcular la cantidad de palabras cuádruples (8 bytes) para el enter $(8 * N), $0
    ctx.total_quadwords = offset_actual / 8;
    if (ctx.total_quadwords < 2) ctx.total_quadwords = 2;
    // Alineación a número par de quadwords (múltiplo de 16 bytes para la ABI)
    if (ctx.total_quadwords % 2 != 0) ctx.total_quadwords++;

    // --------------------------------------------------------------------------
    // Emisión de Código Ensamblador según el Tutorial
    // --------------------------------------------------------------------------
    fprintf(salida, "\n    .globl %s\n", m->nombre);
    fprintf(salida, "    .type %s, @function\n", m->nombre);
    fprintf(salida, "%s:\n", m->nombre);

    // Prólogo exacto del Tutorial: enter $(8 * N), $0
    fprintf(salida, "    enter $(8 * %d), $0\n", ctx.total_quadwords);

    // Guardar los primeros 6 argumentos recibidos en los registros de la ABI
    int cant_params = (func_sym != NULL) ? func_sym->cant_parametros : 0;
    for (int i = 0; i < cant_params && i < MAX_ARGS_REGISTROS; i++) {
        int off = ctx.locales[i].offset;
        fprintf(salida, "    mov %s, -%d(%%rbp)\n", REGISTROS_ARG_64[i], off);
    }
    for (int i = MAX_ARGS_REGISTROS; i < cant_params; i++) {
        int off = ctx.locales[i].offset;
        // Los argumentos más allá de 6 se reciben en la pila del llamador (16 + (i - 6) * 8)
        fprintf(salida, "    mov %d(%%rbp), %%r10\n", 16 + (i - MAX_ARGS_REGISTROS) * 8);
        fprintf(salida, "    mov %%r10, -%d(%%rbp)\n", off);
    }

    // Pila para acumular argumentos de llamadas pendientes (Param Rn ... Call)
    char *params_pendientes[64];
    int cant_params_pendientes = 0;

    // Recorrer y traducir cada instrucción 3D usando %r10 y %r11
    ins = m->instrucciones ? m->instrucciones->primero : NULL;
    while (ins != NULL) {
        char op1_mem[64];
        char op2_mem[64];
        char res_mem[64];
        char lbl_buf[64];

        switch (ins->tipo) {
            case INSTR_LOAD_CONST: {
                long val = 0;
                if (ins->op1 != NULL) {
                    if (strcmp(ins->op1, "true") == 0) val = 1;
                    else if (strcmp(ins->op1, "false") == 0) val = 0;
                    else val = atol(ins->op1);
                }
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov $%ld, %%r10\n", val);
                fprintf(salida, "    mov %%r10, %s\n", res_mem);
                break;
            }

            case INSTR_LOAD_MEM: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov %s, %%r10\n", op1_mem);
                fprintf(salida, "    mov %%r10, %s\n", res_mem);
                break;
            }

            case INSTR_STORE_MEM: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov %s, %%r10\n", op1_mem);
                fprintf(salida, "    mov %%r10, %s\n", res_mem);
                break;
            }

            case INSTR_ADD_REG: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                obtener_operando_mem(&ctx, ts, ins->op2, op2_mem, sizeof(op2_mem));
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov %s, %%r10\n", op1_mem);
                fprintf(salida, "    mov %s, %%r11\n", op2_mem);
                fprintf(salida, "    add %%r11, %%r10\n");
                fprintf(salida, "    mov %%r10, %s\n", res_mem);
                break;
            }

            case INSTR_SUB_REG: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                obtener_operando_mem(&ctx, ts, ins->op2, op2_mem, sizeof(op2_mem));
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov %s, %%r10\n", op1_mem);
                fprintf(salida, "    mov %s, %%r11\n", op2_mem);
                fprintf(salida, "    sub %%r11, %%r10\n");
                fprintf(salida, "    mov %%r10, %s\n", res_mem);
                break;
            }

            case INSTR_MUL_REG: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                obtener_operando_mem(&ctx, ts, ins->op2, op2_mem, sizeof(op2_mem));
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov %s, %%r10\n", op1_mem);
                fprintf(salida, "    mov %s, %%r11\n", op2_mem);
                fprintf(salida, "    imul %%r11, %%r10\n");
                fprintf(salida, "    mov %%r10, %s\n", res_mem);
                break;
            }

            case INSTR_DIV_REG: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                obtener_operando_mem(&ctx, ts, ins->op2, op2_mem, sizeof(op2_mem));
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov %s, %%rax\n", op1_mem);
                fprintf(salida, "    cqto\n");
                fprintf(salida, "    mov %s, %%r10\n", op2_mem);
                fprintf(salida, "    idivq %%r10\n");
                fprintf(salida, "    mov %%rax, %s\n", res_mem);
                break;
            }

            case INSTR_MOD_REG: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                obtener_operando_mem(&ctx, ts, ins->op2, op2_mem, sizeof(op2_mem));
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov %s, %%rax\n", op1_mem);
                fprintf(salida, "    cqto\n");
                fprintf(salida, "    mov %s, %%r10\n", op2_mem);
                fprintf(salida, "    idivq %%r10\n");
                fprintf(salida, "    mov %%rdx, %s\n", res_mem);
                break;
            }

            case INSTR_NEG_REG: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov %s, %%r10\n", op1_mem);
                fprintf(salida, "    neg %%r10\n");
                fprintf(salida, "    mov %%r10, %s\n", res_mem);
                break;
            }

            case INSTR_NOT_REG: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov %s, %%r10\n", op1_mem);
                fprintf(salida, "    mov $0, %%r11\n");
                fprintf(salida, "    cmp %%r11, %%r10\n");
                fprintf(salida, "    mov $0, %%r11\n");
                fprintf(salida, "    mov $1, %%r10\n");
                fprintf(salida, "    cmove %%r10, %%r11\n");
                fprintf(salida, "    mov %%r11, %s\n", res_mem);
                break;
            }

            // Comparaciones booleanas usando el esquema cmov del Tutorial (página 4)
            case INSTR_EQ_REG: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                obtener_operando_mem(&ctx, ts, ins->op2, op2_mem, sizeof(op2_mem));
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov %s, %%r10\n", op1_mem);
                fprintf(salida, "    mov %s, %%r11\n", op2_mem);
                fprintf(salida, "    cmp %%r11, %%r10\n");
                fprintf(salida, "    mov $0, %%r11\n");
                fprintf(salida, "    mov $1, %%r10\n");
                fprintf(salida, "    cmove %%r10, %%r11\n");
                fprintf(salida, "    mov %%r11, %s\n", res_mem);
                break;
            }

            case INSTR_LT_REG: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                obtener_operando_mem(&ctx, ts, ins->op2, op2_mem, sizeof(op2_mem));
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov %s, %%r10\n", op1_mem);
                fprintf(salida, "    mov %s, %%r11\n", op2_mem);
                fprintf(salida, "    cmp %%r11, %%r10\n");
                fprintf(salida, "    mov $0, %%r11\n");
                fprintf(salida, "    mov $1, %%r10\n");
                fprintf(salida, "    cmovl %%r10, %%r11\n");
                fprintf(salida, "    mov %%r11, %s\n", res_mem);
                break;
            }

            case INSTR_GT_REG: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                obtener_operando_mem(&ctx, ts, ins->op2, op2_mem, sizeof(op2_mem));
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov %s, %%r10\n", op1_mem);
                fprintf(salida, "    mov %s, %%r11\n", op2_mem);
                fprintf(salida, "    cmp %%r11, %%r10\n");
                fprintf(salida, "    mov $0, %%r11\n");
                fprintf(salida, "    mov $1, %%r10\n");
                fprintf(salida, "    cmovg %%r10, %%r11\n");
                fprintf(salida, "    mov %%r11, %s\n", res_mem);
                break;
            }

            case INSTR_AND_REG: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                obtener_operando_mem(&ctx, ts, ins->op2, op2_mem, sizeof(op2_mem));
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov %s, %%r10\n", op1_mem);
                fprintf(salida, "    mov %s, %%r11\n", op2_mem);
                fprintf(salida, "    and %%r11, %%r10\n");
                fprintf(salida, "    mov %%r10, %s\n", res_mem);
                break;
            }

            case INSTR_OR_REG: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                obtener_operando_mem(&ctx, ts, ins->op2, op2_mem, sizeof(op2_mem));
                obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                fprintf(salida, "    mov %s, %%r10\n", op1_mem);
                fprintf(salida, "    mov %s, %%r11\n", op2_mem);
                fprintf(salida, "    or %%r11, %%r10\n");
                fprintf(salida, "    mov %%r10, %s\n", res_mem);
                break;
            }

            case INSTR_LABEL: {
                limpiar_etiqueta(ins->resultado, lbl_buf, sizeof(lbl_buf));
                fprintf(salida, ".L_%s_%s:\n", ctx.nombre_func, lbl_buf);
                break;
            }

            case INSTR_JUMP: {
                limpiar_etiqueta(ins->resultado, lbl_buf, sizeof(lbl_buf));
                fprintf(salida, "    jmp .L_%s_%s\n", ctx.nombre_func, lbl_buf);
                break;
            }

            // Saltos condicionales basados en comparación (Tutorial, página 5)
            case INSTR_JUMP_FALSE: {
                limpiar_etiqueta(ins->resultado, lbl_buf, sizeof(lbl_buf));
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                fprintf(salida, "    mov %s, %%r10\n", op1_mem);
                fprintf(salida, "    mov $0, %%r11\n");
                fprintf(salida, "    cmp %%r11, %%r10\n");
                fprintf(salida, "    je .L_%s_%s\n", ctx.nombre_func, lbl_buf);
                break;
            }

            case INSTR_JUMP_TRUE: {
                limpiar_etiqueta(ins->resultado, lbl_buf, sizeof(lbl_buf));
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                fprintf(salida, "    mov %s, %%r10\n", op1_mem);
                fprintf(salida, "    mov $0, %%r11\n");
                fprintf(salida, "    cmp %%r11, %%r10\n");
                fprintf(salida, "    jne .L_%s_%s\n", ctx.nombre_func, lbl_buf);
                break;
            }

            case INSTR_PARAM: {
                if (cant_params_pendientes < 64) {
                    params_pendientes[cant_params_pendientes++] = ins->op1 ? strdup(ins->op1) : NULL;
                }
                break;
            }

            case INSTR_CALL: {
                int cant_args = ins->op2 ? atoi(ins->op2) : 0;
                int start_idx = cant_params_pendientes - cant_args;
                if (start_idx < 0) start_idx = 0;

                for (int i = 0; i < cant_args && i < MAX_ARGS_REGISTROS; i++) {
                    const char *arg_reg_name = params_pendientes[start_idx + i];
                    obtener_operando_mem(&ctx, ts, arg_reg_name, op1_mem, sizeof(op1_mem));
                    fprintf(salida, "    mov %s, %s\n", op1_mem, REGISTROS_ARG_64[i]);
                }

                // Según página 8 del tutorial: %rax = 0 antes de call
                fprintf(salida, "    mov $0, %%rax\n");
                fprintf(salida, "    call %s\n", ins->op1 ? ins->op1 : "func");

                if (ins->resultado != NULL) {
                    obtener_operando_mem(&ctx, ts, ins->resultado, res_mem, sizeof(res_mem));
                    fprintf(salida, "    mov %%rax, %s\n", res_mem);
                }

                // Liberar los parámetros consumidos por este call
                for (int i = start_idx; i < cant_params_pendientes; i++) {
                    if (params_pendientes[i]) free(params_pendientes[i]);
                }
                cant_params_pendientes = start_idx;
                break;
            }

            case INSTR_RETURN_REG: {
                obtener_operando_mem(&ctx, ts, ins->op1, op1_mem, sizeof(op1_mem));
                fprintf(salida, "    mov %s, %%rax\n", op1_mem);
                fprintf(salida, "    leave\n");
                fprintf(salida, "    ret\n");
                break;
            }

            case INSTR_RETURN: {
                if (strcmp(ctx.nombre_func, "main") == 0) {
                    fprintf(salida, "    mov $0, %%rax\n");
                }
                fprintf(salida, "    leave\n");
                fprintf(salida, "    ret\n");
                break;
            }

            case INSTR_BEGIN_FUNC:
            case INSTR_END_FUNC:
                break;
        }

        ins = ins->sig;
    }

    // Limpiar cualquier parámetro pendiente no consumido
    for (int i = 0; i < cant_params_pendientes; i++) {
        if (params_pendientes[i]) free(params_pendientes[i]);
    }

    // Epílogo de seguridad por si la función no tuvo return explícito
    if (ctx.tipo_retorno == TIPO_VOID && strcmp(ctx.nombre_func, "main") == 0) {
        fprintf(salida, "    mov $0, %%rax\n");
    }
    fprintf(salida, "    leave\n");
    fprintf(salida, "    ret\n");
    fprintf(salida, "    .size %s, .-%s\n", m->nombre, m->nombre);

    // Liberar memoria local de la función
    for (int i = 0; i < ctx.cant_locales; i++) {
        if (ctx.locales[i].nombre) free(ctx.locales[i].nombre);
    }
}

// ------------------------------------------------------------------------------
// Emisión de Variables Globales (.data)
// ------------------------------------------------------------------------------

static void generar_variables_globales(const TablaSimbolos *ts, FILE *salida) {
    if (!ts || !ts->global) return;

    int hay_globales = 0;
    Simbolo *curr = ts->global->simbolos;
    while (curr != NULL) {
        if (curr->flag == FLAG_VARIABLE) {
            hay_globales = 1;
            break;
        }
        curr = curr->sig;
    }

    if (!hay_globales) return;

    fprintf(salida, "\n    # ==================================================================\n");
    fprintf(salida, "    # Variables Globales (.data)\n");
    fprintf(salida, "    # ==================================================================\n");
    fprintf(salida, "    .data\n");

    curr = ts->global->simbolos;
    while (curr != NULL) {
        if (curr->flag == FLAG_VARIABLE) {
            fprintf(salida, "    .globl %s\n", curr->nombre);
            fprintf(salida, "    .align 8\n");
            fprintf(salida, "    .type %s, @object\n", curr->nombre);
            fprintf(salida, "    .size %s, 8\n", curr->nombre);
            fprintf(salida, "%s:\n", curr->nombre);
            fprintf(salida, "    .quad 0\n");
        }
        curr = curr->sig;
    }
}

// ------------------------------------------------------------------------------
// Emisión de Rutinas Auxiliares del Curso (printInt y get_int)
// ------------------------------------------------------------------------------

static void generar_rutinas_auxiliares(const CodigoIntermedio *ci, FILE *salida) {
    int tiene_printInt = 0;
    int tiene_get_int = 0;

    const Metodo3D *m = ci ? ci->metodos : NULL;
    while (m != NULL) {
        if (strcmp(m->nombre, "printInt") == 0) tiene_printInt = 1;
        if (strcmp(m->nombre, "get_int") == 0) tiene_get_int = 1;
        m = m->sig;
    }

    // Rutina printInt (página 8 del Tutorial)
    if (!tiene_printInt) {
        fprintf(salida, "\n    # ------------------------------------------------------------------\n");
        fprintf(salida, "    # printInt(int x): imprime un entero por terminal seguido de salto\n");
        fprintf(salida, "    # ------------------------------------------------------------------\n");
        fprintf(salida, "    .globl printInt\n");
        fprintf(salida, "    .type printInt, @function\n");
        fprintf(salida, "printInt:\n");
        fprintf(salida, "    enter $(8 * 2), $0\n");
        fprintf(salida, "    mov %%rdi, -8(%%rbp)\n");
        fprintf(salida, "    mov -8(%%rbp), %%rsi\n");
        fprintf(salida, "    leaq .L_fmt_printInt(%%rip), %%rdi\n");
        fprintf(salida, "    mov $0, %%rax\n");
        fprintf(salida, "    call printf\n");
        fprintf(salida, "    mov $0, %%rax\n");
        fprintf(salida, "    leave\n");
        fprintf(salida, "    ret\n");
        fprintf(salida, "    .size printInt, .-printInt\n");
    }

    // Rutina get_int (página 8 del Tutorial)
    if (!tiene_get_int) {
        fprintf(salida, "\n    # ------------------------------------------------------------------\n");
        fprintf(salida, "    # get_int(): lee un entero desde la terminal y lo retorna en %%rax\n");
        fprintf(salida, "    # ------------------------------------------------------------------\n");
        fprintf(salida, "    .globl get_int\n");
        fprintf(salida, "    .type get_int, @function\n");
        fprintf(salida, "get_int:\n");
        fprintf(salida, "    enter $(8 * 2), $0\n");
        fprintf(salida, "    leaq -8(%%rbp), %%rsi\n");
        fprintf(salida, "    leaq .L_fmt_get_int(%%rip), %%rdi\n");
        fprintf(salida, "    mov $0, %%rax\n");
        fprintf(salida, "    call scanf\n");
        fprintf(salida, "    mov -8(%%rbp), %%rax\n");
        fprintf(salida, "    leave\n");
        fprintf(salida, "    ret\n");
        fprintf(salida, "    .size get_int, .-get_int\n");
    }

    // Cadenas de formato para printf y scanf
    fprintf(salida, "\n    .section .rodata\n");
    fprintf(salida, ".L_fmt_printInt:\n");
    fprintf(salida, "    .string \"%%ld\\n\"\n");
    fprintf(salida, ".L_fmt_get_int:\n");
    fprintf(salida, "    .string \"%%ld\"\n");
}

// ------------------------------------------------------------------------------
// Función Principal
// ------------------------------------------------------------------------------

void generar_codigo_assembler(const CodigoIntermedio *ci, const TablaSimbolos *ts, FILE *salida, const char *nombre_archivo_fuente) {
    if (!ci || !salida) return;

    fprintf(salida, "    # ==================================================================\n");
    fprintf(salida, "    # Codigo Objeto Ensamblador x86-64 (GNU Assembler - AT&T)\n");
    fprintf(salida, "    # Generado segun Tutorial Code Assembly Generation (C-TDS)\n");
    if (nombre_archivo_fuente) {
        fprintf(salida, "    # Archivo fuente original: %s\n", nombre_archivo_fuente);
        fprintf(salida, "    .file \"%s\"\n", nombre_archivo_fuente);
    }
    fprintf(salida, "    # ==================================================================\n\n");

    // Sección de código (.text)
    fprintf(salida, "    .text\n");

    // Generar funciones del programa
    const Metodo3D *m = ci->metodos;
    while (m != NULL) {
        generar_funcion_asm(m, ts, salida);
        m = m->sig;
    }

    // Generar variables globales (.data) si existen
    generar_variables_globales(ts, salida);

    // Generar funciones auxiliares printInt y get_int
    generar_rutinas_auxiliares(ci, salida);

    // Directiva de seguridad para pila no ejecutable
    fprintf(salida, "\n    .section .note.GNU-stack,\"\",@progbits\n");
}

int exportar_archivo_assembler(const CodigoIntermedio *ci, const TablaSimbolos *ts, const char *ruta_salida, const char *nombre_archivo_fuente) {
    if (!ci || !ruta_salida) return 0;

    FILE *f = fopen(ruta_salida, "w");
    if (!f) {
        fprintf(stderr, "Error al crear el archivo assembler '%s': ", ruta_salida);
        perror("");
        return 0;
    }

    generar_codigo_assembler(ci, ts, f, nombre_archivo_fuente);
    fclose(f);
    return 1;
}
