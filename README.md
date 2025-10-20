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
gcc -O3 -fopenmp -march=native -o compressor *.c
```

Notas:

- `-O3` activa optimizaciones para mejor rendimiento.
- `-fopenmp` habilita el soporte de OpenMP.
- `-march=native` (opcional) habilita instrucciones específicas de la CPU host.

## Uso

La herramienta resultante suele aceptar al menos dos modos básicos: `compress` y `decompress`. Ejemplos (ajusta los nombres de binarios y rutas según tu proyecto):

Comprimir un archivo:

```bash
./compressor compress archivo_entrada.bin archivo_salida.comp
```
---