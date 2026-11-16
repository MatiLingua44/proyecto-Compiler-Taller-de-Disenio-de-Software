#include <stdio.h>
#include <stdbool.h>

// --- Funciones para extern_basico ---
int f() {
    return 1;
}

int get() {
    return 3;
}

void print(int x) {
    printf("[extern_basico] Valor recibido por print: %d\n", x);
}

// --- Funciones para extern_multi_params ---
int calcular(int a, int b, int c) {
    return a + b + c;
}

int en_rango(int val, int min_val, int max_val) {
    return (val >= min_val && val <= max_val) ? 1 : 0;
}

void log_status(int ok, int code) {
    printf("[extern_multi] Status: %s, Code: %d\n", ok ? "OK" : "ERROR", code);
}

// --- Funciones para extern_anon_params ---
int sumar(int a, int b) {
    return a + b;
}

int verificar(int r, int flag) {
    return (r > 30 && flag) ? 1 : 0;
}

void mostrar(int x) {
    printf("[extern_anon] Resultado: %d\n", x);
}

// --- Funciones para extern_con_funciones_locales ---
int get_val() {
    return 4;
}

void put_val(int x) {
    printf("[extern_local] Acumulador final: %d\n", x);
}

// --- Funciones para extern_6_params ---
int suma6(int a, int b, int c, int d, int e, int f) {
    return a + b + c + d + e + f;
}

void print_res(int total) {
    printf("[extern_6_params] Suma de 6 argumentos: %d\n", total);
}

