@echo off
echo === Compilando el proyecto (Compilador C-TDS con Generador Assembler x86-64) ===

echo [Bison] Generando parser...
bison -d bison.y
if %errorlevel% neq 0 (echo ❌ Error en Bison && exit /b %errorlevel%)

echo [Flex] Generando lexer...
flex lex.l
if %errorlevel% neq 0 (echo ❌ Error en Flex && exit /b %errorlevel%)

echo [GCC] Compilando binarios...
gcc -Wall -Wextra -g -o compilador main.c bison.tab.c lex.yy.c tabla_simbolos.c ast.c semantica.c codigo_intermedio.c generador_assembler.c
if %errorlevel% neq 0 (echo ❌ Error en GCC && exit /b %errorlevel%)
echo Compilacion exitosa.
