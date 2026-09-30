@echo off
echo === 1. Compilando el proyecto (Entrega AST y TS) ===

echo [Bison] Generando parser...
bison -d bison.y
if %errorlevel% neq 0 (echo ❌ Error en Bison && exit /b %errorlevel%)

echo [Flex] Generando lexer...
flex lex.l
if %errorlevel% neq 0 (echo ❌ Error en Flex && exit /b %errorlevel%)

echo [GCC] Compilando binarios...
gcc -Wall -Wextra -g -o compilador main.c bison.tab.c lex.yy.c tabla_simbolos.c ast.c
if %errorlevel% neq 0 (echo ❌ Error en GCC && exit /b %errorlevel%)
echo Compilacion exitosa.

REM Nota: Para compilar la version con analisis semantico completo (proxima semana):
REM gcc -Wall -Wextra -g -o compilador_semantica main_semantica.c bison.tab.c lex.yy.c tabla_simbolos.c ast.c semantica.c
