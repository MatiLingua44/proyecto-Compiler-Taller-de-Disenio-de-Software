#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tabla_simbolos.h"
#include "ast.h"

// Variables y funciones externas de Flex y Bison
extern int yyparse(void);
extern FILE *yyin;

// Puntero global a la Tabla de Símbolos
TablaSimbolos *tabla_simbolos = NULL;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo_fuente.txt>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s programs/corrects/ex2.txt\n", argv[0]);
        return 1;
    }

    const char *archivo_entrada = argv[1];
    FILE *archivo = fopen(archivo_entrada, "r");
    if (!archivo) {
        fprintf(stderr, "Error al abrir el archivo '%s': ", archivo_entrada);
        perror("");
        return 1;
    }

    yyin = archivo;

    printf("\n============================================================\n");
    printf("Compilador C-TDS - Etapa: Arbol AST y Tabla de Simbolos (TS)\n");
    printf("Procesando: %s\n", archivo_entrada);
    printf("============================================================\n");

    // 1. Inicializar la Tabla de Símbolos (Pila de Niveles)
    tabla_simbolos = InicializarTS();

    // 2. Ejecutar el análisis sintáctico y construir el AST y la TS
    int resultado = yyparse();
    fclose(archivo);

    if (resultado != 0) {
        fprintf(stderr, "\n❌ El archivo contiene errores sintacticos.\n");
        LiberarTS(tabla_simbolos);
        return 1;
    }

    // 3. Mostrar la Tabla de Símbolos organizada por ámbitos (Niveles)
    ImprimirTS(tabla_simbolos);

    // 4. Mostrar el Árbol Sintáctico Abstracto (AST) de cada método
    imprimir_ast_todos_los_metodos(tabla_simbolos);

    // 5. Generar archivo Graphviz DOT y compilar automáticamente la imagen PNG
    const char *archivo_dot = "ast.dot";
    generar_ast_dot(tabla_simbolos, archivo_dot);
    printf("\n[Graphviz] Archivo DOT generado: %s\n", archivo_dot);

    if (system("dot -Tpng ast.dot -o ast.png 2>/dev/null") == 0) {
        printf("[Graphviz] Imagen del AST generada exitosamente: ast.png\n");
        printf("           (Puedes abrir 'ast.png' para ver el arbol graficamente)\n");
    } else {
        printf("[Graphviz] Para generar la imagen manualmente: dot -Tpng ast.dot -o ast.png\n");
    }

    printf("\n✅ AST y Tabla de Simbolos generados exitosamente.\n");

    // 6. Liberar memoria
    LiberarTS(tabla_simbolos);

    return 0;
}