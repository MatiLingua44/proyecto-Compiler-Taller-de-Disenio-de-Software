@echo off
echo === 1. Compilando el proyecto ===

@REM  echo [Bison] Generando parser...
@REM  bison -d bison.y
@REM  if %errorlevel% neq 0 (echo ❌ Error en Bison && exit /b %errorlevel%)

echo [Flex] Generando lexer...
flex lex.l
if %errorlevel% neq 0 (echo ❌ Error en Flex && exit /b %errorlevel%)

echo [GCC] Compilando binarios...
@REM  gcc -Wall -Wextra -g -o compilador main.c bison.tab.c lex.yy.c ast.c lista.c semantica.c assembly.c
gcc -Wall -Wextra -g -o compilador lex.yy.c
if %errorlevel% neq 0 (echo ❌ Error en GCC && exit /b %errorlevel%)
echo Compilacion exitosa.
