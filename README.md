# Compresión y descompresión de archivos en C (con OpenMP)

Proyecto de ejemplo para comprimir y descomprimir archivos usando el lenguaje C. El proyecto aprovecha OpenMP para paralelizar las partes del algoritmo que pueden beneficiarse de ejecución concurrente en máquinas multi‑núcleo.

El objetivo principal es proporcionar una implementación eficiente de compresión/descompresión, mostrando cómo integrar paralelismo mediante OpenMP, mostrando mejoras de rendimiento y aprovechamiento a nivel de procesamiento computacional.

## Miembros del equipo

- Jerónimo Acosta Acevedo
- Miguel Ángel Cano Salinas
- Delvin José Rodríguez Jiménez
- Wendy Daniela Benítez Gómez

## Características

- Implementación en C
- Soporte para paralelismo con OpenMP.
- Herramientas para medir tiempo y velocidad de compresión/descompresión.

## Requisitos

- Compilador GCC
- Entorno Linux / POSIX.

Recomendado:

- gcc >= 5.0 (con `-fopenmp`).
- OMP_NUM_THREADS ajustable según el número de núcleos disponibles.

## Compilación
A continuación se muestra como se debería compilar el proyecto:

```bash
make
```

## Uso

### Interfaz Gráfica

```bash
./build/file_compressor
```

### Línea de Comandos

```bash
# Ayuda
./build/file_compressor --help

# Comprimir archivo
./build/file_compressor -c input.txt output.w

# Comprimir directorio
./build/file_compressor -c mydirectory/ archive.w

# Descomprimir
./build/file_compressor -d archive.w extracted/
```

---