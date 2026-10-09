#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tabla_simbolos.h"
#include "ast.h"
#include "semantica.h"

// Variables y funciones externas de Flex / Bison
extern int yyparse(void);
extern FILE *yyin;

// Puntero global a la Tabla de Símbolos
TablaSimbolos *tabla_simbolos = NULL;

// Variables de configuración de la línea de comandos (según Especificación, Tabla 1)
int modo_debug = 0;
char *target_etapa = NULL;
char *archivo_salida = NULL;
char *archivo_entrada = NULL;
char *archivo_dot = NULL;

static void imprimir_uso(const char *prog) {
    fprintf(stderr, "Uso: %s [opcion] <archivo_fuente>\n", prog);
    fprintf(stderr, "Opciones:\n");
    fprintf(stderr, "  -debug               Imprime informacion de depuracion (TS, AST y genera ast.dot/ast.png).\n");
    fprintf(stderr, "  -dot [archivo.dot]   Genera el archivo Graphviz DOT para visualizar el AST.\n");
    fprintf(stderr, "  -target <etapa>      Fase hasta la que procede la compilacion (scan, parse, sem, etc.).\n");
    fprintf(stderr, "  -o <salida>          Nombre del archivo de salida.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        imprimir_uso(argv[0]);
        return 1;
    }

    // Procesar argumentos de la línea de comandos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-debug") == 0) {
            modo_debug = 1;
        } else if (strcmp(argv[i], "-dot") == 0) {
            if (i + 1 < argc && argv[i + 1][0] != '-' && strstr(argv[i + 1], ".dot") != NULL) {
                archivo_dot = argv[++i];
            } else {
                archivo_dot = "ast.dot";
            }
        } else if (strcmp(argv[i], "-target") == 0) {
            if (i + 1 < argc) {
                target_etapa = argv[++i];
            } else {
                fprintf(stderr, "Error: la opcion -target requiere un argumento.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                archivo_salida = argv[++i];
            } else {
                fprintf(stderr, "Error: la opcion -o requiere un argumento.\n");
                return 1;
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Opcion desconocida: %s\n", argv[i]);
            imprimir_uso(argv[0]);
            return 1;
        } else {
            archivo_entrada = argv[i];
        }
    }

    if (!archivo_entrada) {
        fprintf(stderr, "Error: no se especifico archivo de entrada.\n");
        imprimir_uso(argv[0]);
        return 1;
    }

    FILE *archivo = fopen(archivo_entrada, "r");
    if (!archivo) {
        fprintf(stderr, "Error al abrir el archivo '%s': ", archivo_entrada);
        perror("");
        return 1;
    }

    yyin = archivo;

    if (modo_debug) {
        printf("============================================================\n");
        printf("Compilador C-TDS - Analizador Semantico\n");
        printf("Iniciando compilacion de '%s'...\n", archivo_entrada);
        printf("============================================================\n");
    }

    // 1. Inicializar componentes semánticos y la Tabla de Símbolos
    tabla_simbolos = InicializarTS();
    inicializar_semantica();

    // 2. Ejecutar el parser (análisis sintáctico y construcción guiada por sintaxis del AST y TS)
    int resultado_parse = yyparse();

    // 3. Si el análisis sintáctico fue exitoso, ejecutar el análisis semántico sobre el AST
    if (resultado_parse == 0) {
        chequear_tipos_programa(tabla_simbolos);
    }

    // Total de errores semánticos detectados (en TS + en reglas de semántica)
    int cant_errores = obtener_cant_errores_semanticos() + tabla_simbolos->total_errores;

    // Si se activó -debug y no se especificó nombre para dot, usar ast.dot por defecto
    if (modo_debug && !archivo_dot) {
        archivo_dot = "ast.dot";
    }

    // Generar archivo DOT si fue solicitado o en modo debug (solo si la sintaxis fue correcta)
    if (archivo_dot != NULL && resultado_parse == 0) {
        generar_ast_dot(tabla_simbolos, archivo_dot);
        if (modo_debug) {
            printf("[Graphviz] Archivo DOT generado: %s\n", archivo_dot);
        }

        // Generar imagen PNG con dot
        char cmd[512];
        char img_name[256];
        snprintf(img_name, sizeof(img_name), "%.*s.png",
                 (int)(strlen(archivo_dot) > 4 && strcmp(archivo_dot + strlen(archivo_dot) - 4, ".dot") == 0
                           ? strlen(archivo_dot) - 4
                           : strlen(archivo_dot)),
                 archivo_dot);
        if (strcmp(img_name, ".png") == 0 || strlen(img_name) == 4) {
            strcpy(img_name, "ast.png");
        }
        snprintf(cmd, sizeof(cmd), "dot -Tpng %s -o %s 2>/dev/null", archivo_dot, img_name);
        if (system(cmd) == 0 && modo_debug) {
            printf("[Graphviz] Imagen del AST generada exitosamente: %s\n", img_name);
        }
    }

    // Si se activó -debug, imprimir la Tabla de Símbolos y los Árboles AST
    if (modo_debug) {
        ImprimirTS(tabla_simbolos);
        imprimir_ast_todos_los_metodos(tabla_simbolos);

        if (resultado_parse == 0 && cant_errores == 0) {
            printf("\n✅ Compilacion exitosa: sin errores sintacticos ni semanticos.\n");
        } else {
            printf("\n❌ Compilacion finalizo con errores (Sintacticos: %d, Semanticos: %d).\n",
                   resultado_parse != 0 ? 1 : 0, cant_errores);
        }
    }

    // Cerrar archivo
    fclose(archivo);

    // Liberar memoria
    LiberarTS(tabla_simbolos);

    // Salida según la especificación: código 0 si fue exitoso, 1 si hubo algún error
    if (resultado_parse != 0 || cant_errores > 0) {
        return 1;
    }

    return 0;
}
