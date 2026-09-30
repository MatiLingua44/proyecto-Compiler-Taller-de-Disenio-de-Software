#include "tabla_simbolos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Inicializa la tabla de símbolos abriendo el Nivel 1 (ámbito global)
TablaSimbolos* InicializarTS(void) {
    TablaSimbolos *ts = (TablaSimbolos *)malloc(sizeof(TablaSimbolos));
    if (!ts) {
        fprintf(stderr, "Error fatal: no se pudo asignar memoria para la Tabla de Simbolos.\n");
        exit(EXIT_FAILURE);
    }

    Nivel *nivel_global = (Nivel *)malloc(sizeof(Nivel));
    if (!nivel_global) {
        fprintf(stderr, "Error fatal: no se pudo asignar memoria para el nivel global.\n");
        exit(EXIT_FAILURE);
    }

    nivel_global->numero = 1;
    nivel_global->simbolos = NULL;
    nivel_global->anterior = NULL;
    nivel_global->siguiente_historial = NULL;

    ts->tope = nivel_global;
    ts->global = nivel_global;
    ts->nivel_actual = 1;
    ts->historial_niveles = nivel_global;
    ts->total_errores = 0;

    return ts;
}

// Abre un nuevo nivel (apila un nuevo ámbito local o bloque)
void AbrirNivel(TablaSimbolos *ts) {
    if (!ts) return;

    ts->nivel_actual++;
    Nivel *nuevo = (Nivel *)malloc(sizeof(Nivel));
    if (!nuevo) {
        fprintf(stderr, "Error fatal: no se pudo asignar memoria para el nuevo nivel.\n");
        exit(EXIT_FAILURE);
    }

    nuevo->numero = ts->nivel_actual;
    nuevo->simbolos = NULL;
    nuevo->anterior = ts->tope; // Enlace hacia abajo en la pila activa

    // Añadir al historial para preservar los símbolos en memoria para el AST
    nuevo->siguiente_historial = ts->historial_niveles;
    ts->historial_niveles = nuevo;

    ts->tope = nuevo; // El nuevo nivel pasa a ser la cima activa
}

// Cierra el nivel actual (desapila el ámbito del alcance activo)
void CerrarNivel(TablaSimbolos *ts) {
    if (!ts || !ts->tope) return;

    if (ts->tope == ts->global) {
        fprintf(stderr, "Advertencia: intento de cerrar el nivel global.\n");
        return;
    }

    // Se desapila el nivel actual de la búsqueda activa
    // NOTA: Los símbolos NO se liberan aquí porque el AST mantiene punteros a ellos.
    ts->tope = ts->tope->anterior;
    ts->nivel_actual--;
}

// Busca un símbolo únicamente en el nivel actual (tope)
Simbolo* BuscarSimboloNivelActual(TablaSimbolos *ts, const char *nombre) {
    if (!ts || !ts->tope || !nombre) return NULL;

    Simbolo *curr = ts->tope->simbolos;
    while (curr != NULL) {
        if (strcmp(curr->nombre, nombre) == 0) {
            return curr;
        }
        curr = curr->sig;
    }
    return NULL;
}

// Inserta un nuevo símbolo en el nivel actual verificando redeclaración
Simbolo* InsertarSimbolo(TablaSimbolos *ts, const char *nombre, TipoSimbolo flag, TipoDato tipo, int linea, int columna) {
    if (!ts || !ts->tope || !nombre) return NULL;

    // Regla 1: Ningún identificador es declarado dos veces en un mismo bloque.
    Simbolo *existente = BuscarSimboloNivelActual(ts, nombre);
    if (existente != NULL) {
        fprintf(stderr, "Error semantico en linea %d, columna %d: Identificador '%s' ya declarado en este mismo bloque (previamente en linea %d).\n",
                linea, columna, nombre, existente->linea);
        ts->total_errores++;
        return NULL;
    }

    Simbolo *nuevo = (Simbolo *)malloc(sizeof(Simbolo));
    if (!nuevo) {
        fprintf(stderr, "Error fatal: fallo al asignar memoria para el simbolo '%s'.\n", nombre);
        exit(EXIT_FAILURE);
    }

    nuevo->nombre = strdup(nombre);
    nuevo->flag = flag;
    nuevo->tipo = tipo;
    nuevo->nivel = ts->tope->numero;
    nuevo->linea = linea;
    nuevo->columna = columna;
    nuevo->parametros = NULL;
    nuevo->cant_parametros = 0;
    nuevo->ast = NULL;
    memset(&(nuevo->valor), 0, sizeof(nuevo->valor));

    // Insertar al final de la lista del nivel para mantener el orden de declaración
    nuevo->sig = NULL;
    if (ts->tope->simbolos == NULL) {
        ts->tope->simbolos = nuevo;
    } else {
        Simbolo *aux = ts->tope->simbolos;
        while (aux->sig != NULL) {
            aux = aux->sig;
        }
        aux->sig = nuevo;
    }

    return nuevo;
}

// Busca un símbolo desde el tope de la pila hacia abajo (Nivel 1), soportando sombreado (shadowing)
Simbolo* BuscarSimbolo(TablaSimbolos *ts, const char *nombre) {
    if (!ts || !nombre) return NULL;

    Nivel *nivel_actual = ts->tope;
    while (nivel_actual != NULL) {
        Simbolo *s = nivel_actual->simbolos;
        while (s != NULL) {
            if (strcmp(s->nombre, nombre) == 0) {
                return s; // Primer símbolo encontrado (el más cercano en el alcance)
            }
            s = s->sig;
        }
        nivel_actual = nivel_actual->anterior;
    }

    return NULL; // No encontrado
}

// Agrega un parámetro a la lista de parámetros formales de un método
void AgregarParametroAFuncion(Simbolo *funcion, const char *nombre, TipoDato tipo) {
    if (!funcion || funcion->flag != FLAG_FUNCION || !nombre) return;

    Parametro *p = (Parametro *)malloc(sizeof(Parametro));
    if (!p) {
        fprintf(stderr, "Error fatal: no se pudo asignar memoria para el parametro '%s'.\n", nombre);
        exit(EXIT_FAILURE);
    }

    p->nombre = strdup(nombre);
    p->tipo = tipo;
    p->sig = NULL;

    if (funcion->parametros == NULL) {
        funcion->parametros = p;
    } else {
        Parametro *aux = funcion->parametros;
        while (aux->sig != NULL) {
            aux = aux->sig;
        }
        aux->sig = p;
    }
    funcion->cant_parametros++;
}

// Helpers para lista de IDs en declaraciones (ej: int a, b, c;)
ListaIDs* crear_lista_ids(char *nombre, int linea, int columna) {
    ListaIDs *lista = (ListaIDs *)malloc(sizeof(ListaIDs));
    NodoID *nodo = (NodoID *)malloc(sizeof(NodoID));
    nodo->nombre = strdup(nombre);
    nodo->linea = linea;
    nodo->columna = columna;
    nodo->sig = NULL;

    lista->primero = nodo;
    lista->ultimo = nodo;
    return lista;
}

ListaIDs* agregar_id_a_lista(ListaIDs *lista, char *nombre, int linea, int columna) {
    if (!lista) return crear_lista_ids(nombre, linea, columna);

    NodoID *nodo = (NodoID *)malloc(sizeof(NodoID));
    nodo->nombre = strdup(nombre);
    nodo->linea = linea;
    nodo->columna = columna;
    nodo->sig = NULL;

    lista->ultimo->sig = nodo;
    lista->ultimo = nodo;
    return lista;
}

void liberar_lista_ids(ListaIDs *lista) {
    if (!lista) return;
    NodoID *curr = lista->primero;
    while (curr != NULL) {
        NodoID *sig = curr->sig;
        free(curr->nombre);
        free(curr);
        curr = sig;
    }
    free(lista);
}

// Conversiones de tipo a string
const char* TipoDatoAString(TipoDato t) {
    switch (t) {
        case TIPO_INT: return "int";
        case TIPO_FLOAT: return "float";
        case TIPO_BOOLEAN: return "boolean";
        case TIPO_VOID: return "void";
        case TIPO_ERROR: return "error";
        default: return "desconocido";
    }
}

const char* TipoSimboloAString(TipoSimbolo f) {
    switch (f) {
        case FLAG_VARIABLE: return "variable";
        case FLAG_FUNCION: return "funcion";
        case FLAG_PARAMETRO: return "param";
        default: return "desconocido";
    }
}

TipoDato StringATipoDato(const char *str) {
    if (!str) return TIPO_DESCONOCIDO;
    if (strcmp(str, "int") == 0) return TIPO_INT;
    if (strcmp(str, "float") == 0) return TIPO_FLOAT;
    if (strcmp(str, "boolean") == 0) return TIPO_BOOLEAN;
    if (strcmp(str, "void") == 0) return TIPO_VOID;
    return TIPO_DESCONOCIDO;
}

// Imprime un símbolo individual
void ImprimirSimbolo(const Simbolo *s) {
    if (!s) return;
    printf("    [%-8s] nombre: %-12s | tipo: %-7s | nivel: %d | linea: %d, col: %d",
           TipoSimboloAString(s->flag), s->nombre, TipoDatoAString(s->tipo), s->nivel, s->linea, s->columna);
    if (s->flag == FLAG_FUNCION) {
        printf(" | params: (");
        Parametro *p = s->parametros;
        while (p != NULL) {
            printf("%s %s%s", TipoDatoAString(p->tipo), p->nombre, p->sig ? ", " : "");
            p = p->sig;
        }
        printf(") [AST: %s]", s->ast ? "si" : "no");
    }
    printf("\n");
}

// Imprime el contenido completo de la Tabla de Símbolos organizada por niveles
void ImprimirTS(const TablaSimbolos *ts) {
    if (!ts) return;

    printf("\n======================================================================\n");
    printf("                       TABLA DE SIMBOLOS\n");
    printf("======================================================================\n");

    // Recorremos todos los niveles en el historial desde el global hacia arriba
    // Como el historial se enlazó agregando a la cabeza, recolectamos en un array para imprimir ordenado
    int max_niveles = 256;
    Nivel *arr[256];
    int count = 0;
    Nivel *cur = ts->historial_niveles;
    while (cur != NULL && count < max_niveles) {
        arr[count++] = cur;
        cur = cur->siguiente_historial;
    }

    // Invertir para imprimir desde el primer nivel creado (Nivel 1)
    for (int i = count - 1; i >= 0; i--) {
        Nivel *n = arr[i];
        if (n->simbolos == NULL) continue; // No imprimir niveles vacíos

        if (n->numero == 1) {
            printf("--- Nivel 1 (Ambito Global) ---\n");
        } else {
            printf("--- Nivel %d (Ambito Local) ---\n", n->numero);
        }

        Simbolo *s = n->simbolos;
        while (s != NULL) {
            ImprimirSimbolo(s);
            s = s->sig;
        }
        printf("\n");
    }
    printf("======================================================================\n");
}

// Libera toda la memoria ocupada por la tabla de símbolos
void LiberarTS(TablaSimbolos *ts) {
    if (!ts) return;

    Nivel *n = ts->historial_niveles;
    while (n != NULL) {
        Nivel *n_sig = n->siguiente_historial;
        Simbolo *s = n->simbolos;
        while (s != NULL) {
            Simbolo *s_sig = s->sig;
            free(s->nombre);
            // Liberar parámetros si es función
            Parametro *p = s->parametros;
            while (p != NULL) {
                Parametro *p_sig = p->sig;
                free(p->nombre);
                free(p);
                p = p_sig;
            }
            free(s);
            s = s_sig;
        }
        free(n);
        n = n_sig;
    }
    free(ts);
}
