#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tabla_simbolos.h"
#include "ast.h"
#include "semantica.h"
#include "codigo_intermedio.h"
#include "generador_assembler.h"

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
    fprintf(stderr, "  -debug               Imprime informacion de depuracion (TS, AST, Arbol Reescrito, IR, Assembler y graficos).\n");
    fprintf(stderr, "  -dot [archivo.dot]   Genera el archivo Graphviz DOT para visualizar el AST.\n");
    fprintf(stderr, "  -target <etapa>      Fase hasta la que procede la compilacion (scan, parse, sem, ir/3ac, asm).\n");
    fprintf(stderr, "  -o <salida>          Nombre del archivo de salida (.s para assembler, o archivo IR).\n");
}

static void obtener_nombre_archivo_asm(const char *entrada, char *salida, size_t max_len) {
    if (!entrada || !salida) return;
    const char *base = entrada;
    const char *p = entrada;
    while (*p) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
        p++;
    }
    const char *punto = strrchr(base, '.');
    if (punto != NULL) {
        size_t len = (size_t)(punto - base);
        snprintf(salida, max_len, "%.*s.s", (int)len, base);
    } else {
        snprintf(salida, max_len, "%s.s", base);
    }
}

static void obtener_nombre_ejecutable(const char *archivo_asm, char *salida, size_t max_len) {
    if (!archivo_asm || !salida) return;
    const char *base = archivo_asm;
    const char *p = archivo_asm;
    while (*p) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
        p++;
    }
    const char *punto = strrchr(base, '.');
    if (punto != NULL && strcmp(punto, ".s") == 0) {
        size_t len = (size_t)(punto - base);
        snprintf(salida, max_len, "%.*s", (int)len, base);
    } else {
        snprintf(salida, max_len, "%s.out", base);
    }
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
        printf("Compilador C-TDS - Analizador Semantico y Generador de Codigo\n");
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

    // 4. Si el análisis sintáctico y semántico fue exitoso, generar Código Intermedio (3 Direcciones)
    CodigoIntermedio *ci = NULL;
    char archivo_asm[512] = {0};

    if (resultado_parse == 0 && cant_errores == 0) {
        int detener_en_frontend = 0;
        if (target_etapa != NULL) {
            if (strcmp(target_etapa, "scan") == 0 ||
                strcmp(target_etapa, "parse") == 0 ||
                strcmp(target_etapa, "sem") == 0 ||
                strcmp(target_etapa, "ast") == 0) {
                detener_en_frontend = 1;
            }
        }

        if (!detener_en_frontend) {
            ci = generar_codigo_intermedio(tabla_simbolos);

            // Verificar si el objetivo termina en Código Intermedio (IR / 3AC)
            if (target_etapa != NULL && (strcmp(target_etapa, "ir") == 0 ||
                                         strcmp(target_etapa, "3ac") == 0 ||
                                         strcmp(target_etapa, "tac") == 0)) {
                if (archivo_salida != NULL) {
                    FILE *f_salida = fopen(archivo_salida, "w");
                    if (!f_salida) {
                        fprintf(stderr, "Error al abrir archivo de salida '%s': ", archivo_salida);
                        perror("");
                    } else {
                        imprimir_codigo_intermedio(ci, f_salida);
                        fclose(f_salida);
                        if (modo_debug) {
                            printf("[IR] Codigo de tres direcciones guardado exitosamente en: %s\n", archivo_salida);
                        }
                    }
                } else {
                    imprimir_codigo_intermedio(ci, stdout);
                }
            } else {
                // 5. Generación de Código Objeto Ensamblador x86-64
                if (archivo_salida != NULL) {
                    strncpy(archivo_asm, archivo_salida, sizeof(archivo_asm) - 1);
                } else {
                    obtener_nombre_archivo_asm(archivo_entrada, archivo_asm, sizeof(archivo_asm));
                }

                // Generar y guardar el archivo .s en disco
                exportar_archivo_assembler(ci, tabla_simbolos, archivo_asm, archivo_entrada);

                // Si no está en modo debug, mostrar el código assembler en la terminal
                if (!modo_debug) {
                    if (target_etapa != NULL && (strcmp(target_etapa, "asm") == 0 || strcmp(target_etapa, "codegen") == 0)) {
                        generar_codigo_assembler(ci, tabla_simbolos, stdout, archivo_entrada);
                    } else {
                        char ejecutable[256];
                        obtener_nombre_ejecutable(archivo_asm, ejecutable, sizeof(ejecutable));

                        printf("======================================================================\n");
                        printf("                 CODIGO ASSEMBLER GENERADO (x86-64)\n");
                        printf("======================================================================\n\n");
                        generar_codigo_assembler(ci, tabla_simbolos, stdout, archivo_entrada);
                        printf("\n======================================================================\n");
                        printf("✅ Archivo assembler generado exitosamente: %s\n", archivo_asm);
                        printf("Para compilarlo con GCC y ejecutarlo:\n");
                        printf("    gcc %s -o %s\n", archivo_asm, ejecutable);
                        printf("    ./%s\n", ejecutable);
                        printf("======================================================================\n");
                    }
                }
            }
        }
    }

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

    // Si se activó -debug y se generó código intermedio, generar visualización DOT del árbol reescrito
    if (modo_debug && ci != NULL) {
        generar_ci_dot(ci, "ir.dot");
        printf("[Graphviz] Archivo DOT del Codigo Intermedio generado: ir.dot\n");
        if (system("dot -Tpng ir.dot -o ir.png 2>/dev/null") == 0) {
            printf("[Graphviz] Imagen del Codigo Intermedio generada exitosamente: ir.png\n");
        }
    }

    // Si se activó -debug, imprimir la Tabla de Símbolos, AST, Árboles Reescritos, Instrucciones 3D y Assembler
    if (modo_debug) {
        ImprimirTS(tabla_simbolos);
        imprimir_ast_todos_los_metodos(tabla_simbolos);

        if (ci != NULL) {
            imprimir_arbol_reescrito_todos(ci, stdout);

            printf("======================================================================\n");
            printf("        SECUENCIA LINEAL DE CODIGO DE TRES DIRECCIONES (3AC)\n");
            printf("======================================================================\n\n");
            imprimir_codigo_intermedio(ci, stdout);
            printf("======================================================================\n");

            if (archivo_asm[0] != '\0') {
                char ejecutable[256];
                obtener_nombre_ejecutable(archivo_asm, ejecutable, sizeof(ejecutable));

                printf("======================================================================\n");
                printf("                 CODIGO ASSEMBLER GENERADO (x86-64)\n");
                printf("======================================================================\n\n");
                generar_codigo_assembler(ci, tabla_simbolos, stdout, archivo_entrada);
                printf("\n======================================================================\n");
                printf("✅ Archivo assembler guardado exitosamente en: %s\n", archivo_asm);
                printf("Para compilarlo con GCC y ejecutarlo:\n");
                printf("    gcc %s -o %s\n", archivo_asm, ejecutable);
                printf("    ./%s\n", ejecutable);
                printf("======================================================================\n");
            }
        }

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
    if (ci != NULL) {
        liberar_codigo_intermedio(ci);
    }
    LiberarTS(tabla_simbolos);

    // Salida según la especificación: código 0 si fue exitoso, 1 si hubo algún error
    if (resultado_parse != 0 || cant_errores > 0) {
        return 1;
    }

    return 0;
}
