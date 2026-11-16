# Compilador C-TDS — Taller de Diseño de Software

Compilador para el lenguaje **C-TDS**, desarrollado en C utilizando **Flex** y **Bison**.

# Integrantes del Grupo
Trimboli Ricardo,
Dosantos Agustin,
Lingua Matias

# Para correr este proyecto se necesita tener instalado:

- flex
- bison
- gcc
- graphviz (Opcional, para obtener graficos del arbol)

## Compilación y Ejecución

### En Linux:
Si es la primera vez que se va a compilar el proyecto hay que darle permiso al archivo compilar.sh con el siguiente comando:
```bash
chmod +x compiler.sh
```
Una vez ejecutado el comando compilar y probar con:
```bash
./compiler.sh
./compilador programs/corrects/slide_ejemplo.txt
```

### En Windows:
```cmd
./compiler.bat
./compilador programs/corrects/slide_ejemplo.txt
```

## Etapa Actual
- **Análisis Léxico:** Reconocimiento de tokens, comentarios anidados y seguimiento de línea/columna (`lex.l`).
- **Análisis Sintáctico:** Gramática formal y precedencias (`bison.y`).
- **Tabla de Símbolos (TS):** Estructura en pila de niveles para soporte de alcances anidados y sombreado de variables (`tabla_simbolos.c`, `tabla_simbolos.h`).
- **Árbol Sintáctico Abstracto (AST):** Árboles ternarios por función con sentencias, expresiones y enlaces directos a la TS (`ast.c`, `ast.h`).
- **Código Intermedio (3AC / TAC):** Generación de instrucciones de tres direcciones a partir del AST (`codigo_intermedio.c`, `codigo_intermedio.h`).
- **Generación de Código Assembler x86-64:** Generación de ensamblador AT&T para x86-64 y soporte de funciones externas (`generador_assembler.c`, `generador_assembler.h`).
- **Visualizador Graphviz:** Generación automática de diagramas en formato DOT e imagen PNG (`ast.dot`, `ast.png`).

---
