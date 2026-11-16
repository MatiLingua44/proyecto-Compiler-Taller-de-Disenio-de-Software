#!/bin/bash
# ==============================================================================
# Script de pruebas automatizadas para funciones externas (extern) en C-TDS
# ==============================================================================

# Colores para la terminal
GREEN="\033[0;32m"
RED="\033[0;31m"
BLUE="\033[0;34m"
YELLOW="\033[1;33m"
RESET="\033[0m"

echo -e "${BLUE}======================================================================${RESET}"
echo -e "${BLUE}     SUITE DE PRUEBAS AUTOMATIZADAS: FUNCIONES EXTERNAS (extern)      ${RESET}"
echo -e "${BLUE}======================================================================${RESET}"
echo ""

# Recompilar el compilador si es necesario
if [ ! -f ./compilador ]; then
    echo "Compilando el proyecto C-TDS..."
    ./compiler.sh > /dev/null 2>&1
fi

mkdir -p tests_extern/bin
PASSED=0
TOTAL=0

# Función para verificar tests positivos
run_positive_test() {
    local test_name="$1"
    local src_file="programs/corrects/$test_name.txt"
    local asm_file="$test_name.s"
    local bin_file="tests_extern/bin/$test_name"
    local expected_output="$2"

    TOTAL=$((TOTAL + 1))
    echo -n "Test Positivo #$TOTAL [$test_name] ... "

    # 1. Compilación C-TDS a assembler x86-64
    ./compilador "$src_file" > /dev/null 2>&1
    if [ $? -ne 0 ] || [ ! -f "$asm_file" ]; then
        echo -e "${RED}[FALLO] - Error en la fase de compilación C-TDS${RESET}"
        return 1
    fi

    # 2. Ensamble y enlace con gcc y librería C
    gcc "$asm_file" tests_extern/lib_extern_tests.c -o "$bin_file" > /dev/null 2>&1
    if [ $? -ne 0 ] || [ ! -f "$bin_file" ]; then
        echo -e "${RED}[FALLO] - Error al enlazar con gcc y lib_extern_tests.c${RESET}"
        return 1
    fi

    # 3. Ejecución del binario
    local actual_output
    actual_output=$("$bin_file")
    local exit_code=$?

    if [ $exit_code -eq 0 ] && [ "$actual_output" == "$expected_output" ]; then
        echo -e "${GREEN}[PASO]${RESET} -> Salida: \"$actual_output\""
        PASSED=$((PASSED + 1))
        # Limpiar assembler temporal
        rm -f "$asm_file"
        return 0
    else
        echo -e "${RED}[FALLO]${RESET}"
        echo "   Esperado: \"$expected_output\""
        echo "   Obtenido: \"$actual_output\" (exit code: $exit_code)"
        return 1
    fi
}

# Función para verificar tests negativos (errores semánticos detectados)
run_negative_test() {
    local test_name="$1"
    local src_file="programs/incorrect/$test_name.txt"
    local expected_msg_regex="$2"

    TOTAL=$((TOTAL + 1))
    echo -n "Test Negativo (Error Semántico) #$TOTAL [$test_name] ... "

    # Compilación que debe fallar semánticamente
    local output
    output=$(./compilador "$src_file" 2>&1)
    local exit_code=$?

    if [ $exit_code -ne 0 ] && echo "$output" | grep -qE "$expected_msg_regex"; then
        echo -e "${GREEN}[PASO]${RESET} (Error semántico detectado correctamente)"
        PASSED=$((PASSED + 1))
        return 0
    else
        echo -e "${RED}[FALLO]${RESET} - No se detectó el error semántico esperado"
        echo "   Salida del compilador:"
        echo "$output"
        return 1
    fi
}

echo -e "${YELLOW}--- 1. Pruebas de Funcionamiento Correcto y Enlace con GCC ---${RESET}"
run_positive_test "extern_basico" "[extern_basico] Valor recibido por print: 4"
run_positive_test "extern_multi_params" "[extern_multi] Status: OK, Code: 60"
run_positive_test "extern_anon_params" "[extern_anon] Resultado: 40"
run_positive_test "extern_con_funciones_locales" "[extern_local] Acumulador final: 20"
run_positive_test "extern_6_params" "[extern_6_params] Suma de 6 argumentos: 21"

echo ""
echo -e "${YELLOW}--- 2. Pruebas de Detección de Errores Semánticos para 'extern' ---${RESET}"
run_negative_test "extern_llamada_args_insuficientes" "Cantidad incorrecta de argumentos"
run_negative_test "extern_llamada_args_excesivos" "Cantidad incorrecta de argumentos"
run_negative_test "extern_llamada_tipo_incompatible" "tipo.*incorrecto|esperaba"
run_negative_test "extern_void_en_expresion" "es void y no puede ser usado dentro de una expresion"
run_negative_test "extern_redefinicion_local" "ya declarado en este mismo bloque"

echo ""
echo -e "${BLUE}======================================================================${RESET}"
if [ $PASSED -eq $TOTAL ]; then
    echo -e "${GREEN}  TODAS LAS PRUEBAS PASARON EXITOSAMENTE: $PASSED / $TOTAL tests OK  ${RESET}"
else
    echo -e "${RED}  ALGUNAS PRUEBAS FALLARON: $PASSED / $TOTAL tests OK  ${RESET}"
fi
echo -e "${BLUE}======================================================================${RESET}"

# Retornar código de salida acorde
[ $PASSED -eq $TOTAL ]
exit $?
