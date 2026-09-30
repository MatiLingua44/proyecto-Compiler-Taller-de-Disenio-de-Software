#!/bin/bash

# Detener el script inmediatamente si ocurre un error
set -e

echo "=== 1. Compilando el proyecto (Entrega AST y TS) ==="
echo "[Bison] Generando parser..."
bison -d bison.y

echo "[Flex] Generando lexer..."
flex lex.l

echo "[GCC] Compilando binarios..."
gcc -Wall -Wextra -g -o compilador main.c bison.tab.c lex.yy.c tabla_simbolos.c ast.c
cp -f compilador c-tds 2>/dev/null || true
echo "✅ Compilacion exitosa."

# Nota: Para compilar la version con analisis semantico completo (proxima semana):
# gcc -Wall -Wextra -g -o compilador_semantica main_semantica.c bison.tab.c lex.yy.c tabla_simbolos.c ast.c semantica.c
