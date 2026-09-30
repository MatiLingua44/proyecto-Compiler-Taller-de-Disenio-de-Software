#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Crea e inicializa un nodo base del AST
NodoAST* crear_nodo(TipoNodoAST tipo, int linea, int columna) {
    NodoAST *nodo = (NodoAST *)malloc(sizeof(NodoAST));
    if (!nodo) {
        fprintf(stderr, "Error fatal: no se pudo asignar memoria para nodo AST.\n");
        exit(EXIT_FAILURE);
    }
    nodo->tipo_nodo = tipo;
    nodo->simbolo = NULL;
    nodo->hijo_izq = NULL;
    nodo->hijo_centro = NULL;
    nodo->hijo_der = NULL;
    nodo->op = OP_NINGUNO;
    nodo->tipo_dato = TIPO_DESCONOCIDO;
    memset(&(nodo->valor), 0, sizeof(nodo->valor));
    nodo->linea = linea;
    nodo->columna = columna;
    return nodo;
}

// Crea un nodo de secuencia de sentencias ';'
NodoAST* crear_nodo_secuencia(NodoAST *izq, NodoAST *der) {
    int l = izq ? izq->linea : (der ? der->linea : 0);
    int c = izq ? izq->columna : (der ? der->columna : 0);
    NodoAST *nodo = crear_nodo(NODO_SECUENCIA, l, c);
    nodo->hijo_izq = izq;
    nodo->hijo_der = der;
    return nodo;
}

/* Concatena sentencias preservando la estructura en árbol de la Diapositiva 6:
        [;]
       /   \
     s1    [;]
          /   \
         s2    s3 (asociativo por derecha) */
NodoAST* concatenar_sentencias(NodoAST *s1, NodoAST *s2) {
    if (!s1) return s2;
    if (!s2) return s1;

    if (s1->tipo_nodo != NODO_SECUENCIA) {
        return crear_nodo_secuencia(s1, s2);
    }

    NodoAST *curr = s1;
    while (curr->hijo_der != NULL && curr->hijo_der->tipo_nodo == NODO_SECUENCIA) {
        curr = curr->hijo_der;
    }
    curr->hijo_der = crear_nodo_secuencia(curr->hijo_der, s2);
    return s1;
}

// Crea nodo de asignación '='
NodoAST* crear_nodo_asignacion(NodoAST *var, NodoAST *expr, int linea, int columna) {
    NodoAST *nodo = crear_nodo(NODO_ASIGNACION, linea, columna);
    nodo->hijo_izq = var;
    nodo->hijo_der = expr;
    if (var && var->simbolo) {
        nodo->simbolo = var->simbolo;
        nodo->tipo_dato = var->simbolo->tipo;
    }
    return nodo;
}

// Crea nodo if ternario: cond, then_block, else_block
NodoAST* crear_nodo_if(NodoAST *cond, NodoAST *then_b, NodoAST *else_b, int linea, int columna) {
    NodoAST *nodo = crear_nodo(NODO_IF, linea, columna);
    nodo->hijo_izq = cond;        // Condición
    nodo->hijo_centro = then_b;   // Rama Then
    nodo->hijo_der = else_b;      // Rama Else (o NULL si no hay)
    return nodo;
}

// Crea nodo while: cond, cuerpo
NodoAST* crear_nodo_while(NodoAST *cond, NodoAST *cuerpo, int linea, int columna) {
    NodoAST *nodo = crear_nodo(NODO_WHILE, linea, columna);
    nodo->hijo_izq = cond;        // Condición
    nodo->hijo_centro = cuerpo;   // Cuerpo del bucle
    return nodo;
}

// Crea nodo return
NodoAST* crear_nodo_return(NodoAST *expr, int linea, int columna) {
    NodoAST *nodo = crear_nodo(NODO_RETURN, linea, columna);
    nodo->hijo_izq = expr;
    if (expr) {
        nodo->tipo_dato = expr->tipo_dato;
    } else {
        nodo->tipo_dato = TIPO_VOID;
    }
    return nodo;
}

// Crea nodo de llamada a método como sentencia
NodoAST* crear_nodo_llamada_stmt(Simbolo *metodo, NodoAST *args, int linea, int columna) {
    NodoAST *nodo = crear_nodo(NODO_LLAMADA_STMT, linea, columna);
    nodo->simbolo = metodo;
    nodo->hijo_izq = args;
    if (metodo) {
        nodo->tipo_dato = metodo->tipo;
    }
    return nodo;
}

// Crea nodo bloque
NodoAST* crear_nodo_bloque(NodoAST *sentencias, int linea, int columna) {
    NodoAST *nodo = crear_nodo(NODO_BLOQUE, linea, columna);
    nodo->hijo_izq = sentencias;
    return nodo;
}

// Crea nodo de operación binaria
NodoAST* crear_nodo_binario(OperadorAST op, NodoAST *izq, NodoAST *der, int linea, int columna) {
    NodoAST *nodo = crear_nodo(NODO_EXPR_BINARIA, linea, columna);
    nodo->op = op;
    nodo->hijo_izq = izq;
    nodo->hijo_der = der;
    return nodo;
}

// Crea nodo de operación unaria
NodoAST* crear_nodo_unario(OperadorAST op, NodoAST *expr, int linea, int columna) {
    NodoAST *nodo = crear_nodo(NODO_EXPR_UNARIA, linea, columna);
    nodo->op = op;
    nodo->hijo_izq = expr;
    return nodo;
}

// Crea nodo hoja para variable o parámetro con referencia directa al símbolo en la TS
NodoAST* crear_nodo_var(Simbolo *s, int linea, int columna) {
    NodoAST *nodo = crear_nodo(NODO_EXPR_VAR, linea, columna);
    nodo->simbolo = s;
    if (s) {
        nodo->tipo_dato = s->tipo;
    } else {
        nodo->tipo_dato = TIPO_ERROR;
    }
    return nodo;
}

// Crea nodo literal entero
NodoAST* crear_nodo_literal_int(int val, int linea, int columna) {
    NodoAST *nodo = crear_nodo(NODO_EXPR_LITERAL, linea, columna);
    nodo->tipo_dato = TIPO_INT;
    nodo->valor.val_int = val;
    return nodo;
}

// Crea nodo literal flotante
NodoAST* crear_nodo_literal_float(float val, int linea, int columna) {
    NodoAST *nodo = crear_nodo(NODO_EXPR_LITERAL, linea, columna);
    nodo->tipo_dato = TIPO_FLOAT;
    nodo->valor.val_float = val;
    return nodo;
}

// Crea nodo literal booleano
NodoAST* crear_nodo_literal_bool(int val, int linea, int columna) {
    NodoAST *nodo = crear_nodo(NODO_EXPR_LITERAL, linea, columna);
    nodo->tipo_dato = TIPO_BOOLEAN;
    nodo->valor.val_bool = val;
    return nodo;
}

// Crea nodo de llamada a función dentro de una expresión
NodoAST* crear_nodo_expr_llamada(Simbolo *metodo, NodoAST *args, int linea, int columna) {
    NodoAST *nodo = crear_nodo(NODO_EXPR_LLAMADA, linea, columna);
    nodo->simbolo = metodo;
    nodo->hijo_izq = args;
    if (metodo) {
        nodo->tipo_dato = metodo->tipo;
    }
    return nodo;
}

// Crea nodo lista de argumentos para llamadas a funciones
NodoAST* crear_nodo_lista_args(NodoAST *arg, NodoAST *sig) {
    int l = arg ? arg->linea : 0;
    int c = arg ? arg->columna : 0;
    NodoAST *nodo = crear_nodo(NODO_LISTA_ARGS, l, c);
    nodo->hijo_izq = arg;
    nodo->hijo_der = sig;
    return nodo;
}

// Agrega un argumento al final de la lista de argumentos
NodoAST* agregar_arg_a_lista(NodoAST *lista, NodoAST *nuevo_arg) {
    if (!lista) return crear_nodo_lista_args(nuevo_arg, NULL);

    NodoAST *curr = lista;
    while (curr->hijo_der != NULL) {
        curr = curr->hijo_der;
    }
    curr->hijo_der = crear_nodo_lista_args(nuevo_arg, NULL);
    return lista;
}

// Retorna la representación en cadena de texto de un operador
const char* OperadorAString(OperadorAST op) {
    switch (op) {
        case OP_SUMA: return "+";
        case OP_RESTA: return "-";
        case OP_MULT: return "*";
        case OP_DIV: return "/";
        case OP_MOD: return "%";
        case OP_MENOR: return "<";
        case OP_MAYOR: return ">";
        case OP_IGUAL: return "==";
        case OP_AND: return "&&";
        case OP_OR: return "||";
        case OP_NEGACION: return "!";
        case OP_MENOS_UNARIO: return "- (unario)";
        default: return "?";
    }
}

// Función recursiva para imprimir el AST en forma de árbol legible con enlaces a la TS
void imprimir_ast(const NodoAST *nodo, int indent, int es_ultimo, const char *prefijo) {
    if (!nodo) return;

    char nuevo_prefijo[512];
    printf("%s%s", prefijo, es_ultimo ? "└── " : "├── ");

    switch (nodo->tipo_nodo) {
        case NODO_SECUENCIA:
            printf("[ ; ] (Secuencia)\n");
            break;
        case NODO_ASIGNACION:
            printf("[ = ] (Asignacion)\n");
            break;
        case NODO_IF:
            printf("[ IF ] (Condicional)\n");
            break;
        case NODO_WHILE:
            printf("[ WHILE ] (Bucle)\n");
            break;
        case NODO_RETURN:
            printf("[ RETURN ] (tipo: %s)\n", TipoDatoAString(nodo->tipo_dato));
            break;
        case NODO_LLAMADA_STMT:
            printf("[ LLAMADA_STMT: '%s' ] -> [TS: flag=%s, retorno=%s]\n",
                   nodo->simbolo ? nodo->simbolo->nombre : "(indefinido)",
                   nodo->simbolo ? TipoSimboloAString(nodo->simbolo->flag) : "?",
                   nodo->simbolo ? TipoDatoAString(nodo->simbolo->tipo) : "?");
            break;
        case NODO_BLOQUE:
            printf("[ BLOQUE ]\n");
            break;
        case NODO_EXPR_BINARIA:
            printf("[ OP: %s ]\n", OperadorAString(nodo->op));
            break;
        case NODO_EXPR_UNARIA:
            printf("[ OP_UNARIO: %s ]\n", OperadorAString(nodo->op));
            break;
        case NODO_EXPR_VAR:
            if (nodo->simbolo) {
                printf("[ ID: '%s' ] -> [TS: flag=%s, tipo=%s, nivel=%d, dec_linea=%d]\n",
                       nodo->simbolo->nombre,
                       TipoSimboloAString(nodo->simbolo->flag),
                       TipoDatoAString(nodo->simbolo->tipo),
                       nodo->simbolo->nivel,
                       nodo->simbolo->linea);
            } else {
                printf("[ ID: (no declarado o error) ]\n");
            }
            break;
        case NODO_EXPR_LITERAL:
            if (nodo->tipo_dato == TIPO_INT) {
                printf("[ LITERAL: %d (int) ]\n", nodo->valor.val_int);
            } else if (nodo->tipo_dato == TIPO_FLOAT) {
                printf("[ LITERAL: %f (float) ]\n", nodo->valor.val_float);
            } else if (nodo->tipo_dato == TIPO_BOOLEAN) {
                printf("[ LITERAL: %s (boolean) ]\n", nodo->valor.val_bool ? "true" : "false");
            } else {
                printf("[ LITERAL ]\n");
            }
            break;
        case NODO_EXPR_LLAMADA:
            printf("[ EXPR_LLAMADA: '%s' ] -> [TS: flag=%s, retorno=%s]\n",
                   nodo->simbolo ? nodo->simbolo->nombre : "(indefinido)",
                   nodo->simbolo ? TipoSimboloAString(nodo->simbolo->flag) : "?",
                   nodo->simbolo ? TipoDatoAString(nodo->simbolo->tipo) : "?");
            break;
        case NODO_LISTA_ARGS:
            printf("[ ARG ]\n");
            break;
        default:
            printf("[ NODO DESCONOCIDO ]\n");
            break;
    }

    snprintf(nuevo_prefijo, sizeof(nuevo_prefijo), "%s%s", prefijo, es_ultimo ? "    " : "│   ");

    // Recorrer los hijos según el tipo de nodo
    if (nodo->tipo_nodo == NODO_IF) {
        if (nodo->hijo_izq) {
            printf("%s├── (Condicion):\n", nuevo_prefijo);
            imprimir_ast(nodo->hijo_izq, indent + 1, 1, nuevo_prefijo);
        }
        if (nodo->hijo_centro) {
            printf("%s├── (Then):\n", nuevo_prefijo);
            imprimir_ast(nodo->hijo_centro, indent + 1, nodo->hijo_der ? 0 : 1, nuevo_prefijo);
        }
        if (nodo->hijo_der) {
            printf("%s└── (Else):\n", nuevo_prefijo);
            imprimir_ast(nodo->hijo_der, indent + 1, 1, nuevo_prefijo);
        }
    } else if (nodo->tipo_nodo == NODO_WHILE) {
        if (nodo->hijo_izq) {
            printf("%s├── (Condicion):\n", nuevo_prefijo);
            imprimir_ast(nodo->hijo_izq, indent + 1, 0, nuevo_prefijo);
        }
        if (nodo->hijo_centro) {
            printf("%s└── (Cuerpo):\n", nuevo_prefijo);
            imprimir_ast(nodo->hijo_centro, indent + 1, 1, nuevo_prefijo);
        }
    } else {
        int cant_hijos = 0;
        if (nodo->hijo_izq) cant_hijos++;
        if (nodo->hijo_centro) cant_hijos++;
        if (nodo->hijo_der) cant_hijos++;

        int h = 0;
        if (nodo->hijo_izq) {
            h++;
            imprimir_ast(nodo->hijo_izq, indent + 1, (h == cant_hijos), nuevo_prefijo);
        }
        if (nodo->hijo_centro) {
            h++;
            imprimir_ast(nodo->hijo_centro, indent + 1, (h == cant_hijos), nuevo_prefijo);
        }
        if (nodo->hijo_der) {
            h++;
            imprimir_ast(nodo->hijo_der, indent + 1, (h == cant_hijos), nuevo_prefijo);
        }
    }
}

// Imprime el AST de un método individual
void imprimir_ast_metodo(const Simbolo *metodo) {
    if (!metodo || metodo->flag != FLAG_FUNCION) return;

    printf("\nAST de la funcion '%s' (tipo retorno: %s):\n",
           metodo->nombre, TipoDatoAString(metodo->tipo));
    printf("Parametros: (");
    Parametro *p = metodo->parametros;
    while (p != NULL) {
        printf("%s %s%s", TipoDatoAString(p->tipo), p->nombre, p->sig ? ", " : "");
        p = p->sig;
    }
    printf(")\n");

    if (metodo->ast != NULL) {
        imprimir_ast(metodo->ast, 0, 1, "");
    } else {
        printf("    (Cuerpo vacio / sin sentencias)\n");
    }
}

// Imprime el AST de cada una de las funciones del programa ("Un AST por función")
void imprimir_ast_todos_los_metodos(const TablaSimbolos *ts) {
    if (!ts || !ts->global) return;

    printf("\n======================================================================\n");
    printf("              ARBOLES SINTACTICOS ABSTRACTOS (AST)\n");
    printf("                    (Un AST por funcion)\n");
    printf("======================================================================\n");

    Simbolo *s = ts->global->simbolos;
    int count = 0;
    while (s != NULL) {
        if (s->flag == FLAG_FUNCION) {
            imprimir_ast_metodo(s);
            count++;
        }
        s = s->sig;
    }

    if (count == 0) {
        printf("No se encontraron metodos declarados en el programa.\n");
    }
    printf("======================================================================\n");
}

// Libera recursivamente un subárbol del AST
void liberar_ast(NodoAST *nodo) {
    if (!nodo) return;
    liberar_ast(nodo->hijo_izq);
    liberar_ast(nodo->hijo_centro);
    liberar_ast(nodo->hijo_der);
    free(nodo);
}

// Genera un nodo y sus aristas en formato Graphviz DOT recursivamente
static int generar_nodo_dot_rec(const NodoAST *nodo, FILE *f, int *id_counter) {
    if (!nodo) return -1;

    int mi_id = (*id_counter)++;

    switch (nodo->tipo_nodo) {
        case NODO_SECUENCIA:
            fprintf(f, "    n%d [label=\";\", shape=box, style=\"filled,rounded\", fillcolor=\"#E9ECEF\", color=\"#495057\"];\n", mi_id);
            break;
        case NODO_ASIGNACION:
            fprintf(f, "    n%d [label=\"=\", shape=box, style=\"filled,rounded\", fillcolor=\"#FFF3BF\", color=\"#FAB005\"];\n", mi_id);
            break;
        case NODO_IF:
            fprintf(f, "    n%d [label=\"if\", shape=diamond, style=\"filled\", fillcolor=\"#FFE3E3\", color=\"#FA5252\"];\n", mi_id);
            break;
        case NODO_WHILE:
            fprintf(f, "    n%d [label=\"while\", shape=diamond, style=\"filled\", fillcolor=\"#FFE3E3\", color=\"#FA5252\"];\n", mi_id);
            break;
        case NODO_RETURN:
            fprintf(f, "    n%d [label=\"return\", shape=box, style=\"filled,rounded\", fillcolor=\"#C5F6FA\", color=\"#15AABF\"];\n", mi_id);
            break;
        case NODO_BLOQUE:
            fprintf(f, "    n%d [label=\"bloque\", shape=box, style=\"filled,rounded\", fillcolor=\"#F1F3F5\", color=\"#868E96\"];\n", mi_id);
            break;
        case NODO_EXPR_BINARIA:
            fprintf(f, "    n%d [label=\"%s\", shape=circle, style=\"filled\", fillcolor=\"#E7F5FF\", color=\"#339AF0\"];\n",
                    mi_id, OperadorAString(nodo->op));
            break;
        case NODO_EXPR_UNARIA:
            fprintf(f, "    n%d [label=\"%s\", shape=circle, style=\"filled\", fillcolor=\"#E7F5FF\", color=\"#339AF0\"];\n",
                    mi_id, OperadorAString(nodo->op));
            break;
        case NODO_EXPR_VAR:
            if (nodo->simbolo) {
                fprintf(f, "    n%d [label=\"%s\\n(%s)\", shape=ellipse, style=\"filled\", fillcolor=\"#D3F9D8\", color=\"#40C057\"];\n",
                        mi_id, nodo->simbolo->nombre, TipoDatoAString(nodo->simbolo->tipo));
            } else {
                fprintf(f, "    n%d [label=\"id (error)\", shape=ellipse, style=\"filled\", fillcolor=\"#FFC9C9\", color=\"#FA5252\"];\n", mi_id);
            }
            break;
        case NODO_EXPR_LITERAL:
            if (nodo->tipo_dato == TIPO_INT) {
                fprintf(f, "    n%d [label=\"%d\", shape=ellipse, style=\"filled\", fillcolor=\"#FFF9DB\", color=\"#F59F00\"];\n",
                        mi_id, nodo->valor.val_int);
            } else if (nodo->tipo_dato == TIPO_FLOAT) {
                fprintf(f, "    n%d [label=\"%.2f\", shape=ellipse, style=\"filled\", fillcolor=\"#FFF9DB\", color=\"#F59F00\"];\n",
                        mi_id, nodo->valor.val_float);
            } else if (nodo->tipo_dato == TIPO_BOOLEAN) {
                fprintf(f, "    n%d [label=\"%s\", shape=ellipse, style=\"filled\", fillcolor=\"#FFF9DB\", color=\"#F59F00\"];\n",
                        mi_id, nodo->valor.val_bool ? "true" : "false");
            } else {
                fprintf(f, "    n%d [label=\"lit\", shape=ellipse, style=\"filled\", fillcolor=\"#FFF9DB\"];\n", mi_id);
            }
            break;
        case NODO_EXPR_LLAMADA:
        case NODO_LLAMADA_STMT:
            fprintf(f, "    n%d [label=\"call: %s()\", shape=box, style=\"filled,rounded\", fillcolor=\"#EBE4FF\", color=\"#7950F2\"];\n",
                    mi_id, nodo->simbolo ? nodo->simbolo->nombre : "desconocido");
            break;
        case NODO_LISTA_ARGS:
            fprintf(f, "    n%d [label=\"arg\", shape=point];\n", mi_id);
            break;
        default:
            fprintf(f, "    n%d [label=\"nodo\"];\n", mi_id);
            break;
    }

    if (nodo->tipo_nodo == NODO_IF) {
        if (nodo->hijo_izq) {
            int id_cond = generar_nodo_dot_rec(nodo->hijo_izq, f, id_counter);
            fprintf(f, "    n%d -> n%d [label=\"cond\"];\n", mi_id, id_cond);
        }
        if (nodo->hijo_centro) {
            int id_then = generar_nodo_dot_rec(nodo->hijo_centro, f, id_counter);
            fprintf(f, "    n%d -> n%d [label=\"then\"];\n", mi_id, id_then);
        }
        if (nodo->hijo_der) {
            int id_else = generar_nodo_dot_rec(nodo->hijo_der, f, id_counter);
            fprintf(f, "    n%d -> n%d [label=\"else\"];\n", mi_id, id_else);
        }
    } else if (nodo->tipo_nodo == NODO_WHILE) {
        if (nodo->hijo_izq) {
            int id_cond = generar_nodo_dot_rec(nodo->hijo_izq, f, id_counter);
            fprintf(f, "    n%d -> n%d [label=\"cond\"];\n", mi_id, id_cond);
        }
        if (nodo->hijo_centro) {
            int id_body = generar_nodo_dot_rec(nodo->hijo_centro, f, id_counter);
            fprintf(f, "    n%d -> n%d [label=\"body\"];\n", mi_id, id_body);
        }
    } else if (nodo->tipo_nodo == NODO_EXPR_LLAMADA || nodo->tipo_nodo == NODO_LLAMADA_STMT) {
        NodoAST *curr_arg = nodo->hijo_izq;
        int arg_num = 1;
        while (curr_arg != NULL) {
            if (curr_arg->hijo_izq != NULL) {
                int id_arg = generar_nodo_dot_rec(curr_arg->hijo_izq, f, id_counter);
                fprintf(f, "    n%d -> n%d [label=\"arg %d\"];\n", mi_id, id_arg, arg_num++);
            }
            curr_arg = curr_arg->hijo_der;
        }
    } else {
        if (nodo->hijo_izq) {
            int id_izq = generar_nodo_dot_rec(nodo->hijo_izq, f, id_counter);
            fprintf(f, "    n%d -> n%d;\n", mi_id, id_izq);
        }
        if (nodo->hijo_centro) {
            int id_cen = generar_nodo_dot_rec(nodo->hijo_centro, f, id_counter);
            fprintf(f, "    n%d -> n%d;\n", mi_id, id_cen);
        }
        if (nodo->hijo_der) {
            int id_der = generar_nodo_dot_rec(nodo->hijo_der, f, id_counter);
            fprintf(f, "    n%d -> n%d;\n", mi_id, id_der);
        }
    }

    return mi_id;
}

// Genera un archivo en formato Graphviz DOT representando el AST completo del programa
void generar_ast_dot(const TablaSimbolos *ts, const char *nombre_archivo) {
    if (!ts || !ts->global || !nombre_archivo) return;

    FILE *f = fopen(nombre_archivo, "w");
    if (!f) {
        fprintf(stderr, "Error: no se pudo crear el archivo DOT '%s'\n", nombre_archivo);
        return;
    }

    fprintf(f, "digraph AST {\n");
    fprintf(f, "    rankdir=TB;\n");
    fprintf(f, "    graph [ordering=\"out\", splines=polyline];\n");
    fprintf(f, "    node [fontname=\"Helvetica\", fontsize=11];\n");
    fprintf(f, "    edge [fontname=\"Helvetica\", fontsize=9, color=\"#495057\"];\n\n");

    int id_counter = 1;

    // Nodo raíz del programa que agrupa los métodos
    int id_prog = id_counter++;
    fprintf(f, "    n%d [label=\"Programa\", shape=box, style=\"filled\", fillcolor=\"#339AF0\", fontcolor=\"white\", fontsize=13];\n", id_prog);

    Simbolo *s = ts->global->simbolos;
    while (s != NULL) {
        if (s->flag == FLAG_FUNCION) {
            int id_metodo = id_counter++;
            fprintf(f, "    n%d [label=\"%s\\n(%s)\", shape=box, style=\"filled,rounded\", fillcolor=\"#D0EBFF\", color=\"#1971C2\", penwidth=2];\n",
                    id_metodo, s->nombre, TipoDatoAString(s->tipo));
            fprintf(f, "    n%d -> n%d;\n", id_prog, id_metodo);

            if (s->ast != NULL) {
                int id_cuerpo = generar_nodo_dot_rec(s->ast, f, &id_counter);
                fprintf(f, "    n%d -> n%d;\n", id_metodo, id_cuerpo);
            }
        }
        s = s->sig;
    }

    fprintf(f, "}\n");
    fclose(f);
}

