%{
#include <stdio.h>
%}

/* Reglas básicas de ejemplo */
%%
[0-9]+          { printf("Numero encontrado: %s\n", yytext); }
[a-zA-Z]+       { printf("Palabra encontrada: %s\n", yytext); }
"\n"            { /* Ignorar saltos de línea */ }
[ \t]           { /* Ignorar espacios vacios y tabulaciones */ }
.               { /* Ignorar cualquier otro carácter */ }
%%

int yywrap() {
    return 1;
}

int main(int argc, char *argv[]) {
    // Verificar si se pasó el nombre del archivo como argumento
    if (argc < 2) {
        printf("Uso: %s <archivo_de_entrada>\n", argv[0]);
        return 1;
    }

    // Abrir el archivo en modo lectura
    FILE *archivo = fopen(argv[1], "r");
    if (!archivo) {
        printf("Error: No se pudo abrir el archivo %s\n", argv[1]);
        return 1;
    }

    // Redireccionar la entrada de Flex al archivo
    yyin = archivo;

    // Iniciar el análisis léxico
    yylex();

    // Cerrar el archivo al terminar
    fclose(archivo);
    return 0;
}
