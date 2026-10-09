#!/bin/bash

# Detener el script inmediatamente si ocurre un error
set -e

echo "=== Compilando el proyecto (Compilador C-TDS con Analizador Semantico) ==="
echo "[Bison] Generando parser..."
bison -d bison.y

echo "[Flex] Generando lexer..."
flex lex.l

echo "[GCC] Compilando binarios..."
gcc -Wall -Wextra -g -o compilador main.c bison.tab.c lex.yy.c tabla_simbolos.c ast.c semantica.c
cp -f compilador c-tds 2>/dev/null || true
echo "✅ Compilacion exitosa."
