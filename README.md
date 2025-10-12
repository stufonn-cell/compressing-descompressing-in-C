# Compresión y descompresión de archivos en C (con OpenMP)

Proyecto de ejemplo para comprimir y descomprimir archivos usando el lenguaje C. El proyecto aprovecha OpenMP para paralelizar las partes del algoritmo que pueden beneficiarse de ejecución concurrente en máquinas multi‑núcleo.

El objetivo principal es proporcionar una implementación didáctica y eficiente de compresión/descompresión, mostrando cómo integrar paralelismo mediante OpenMP y cómo medir mejoras de rendimiento.

## Características

- Implementación en C (portátil y ligera).
- Soporte para paralelismo con OpenMP (configurable mediante OMP_NUM_THREADS).
- Herramientas para medir tiempo y velocidad de compresión/descompresión.
- Ejemplos y comandos para compilar y ejecutar en Linux.

## Requisitos

- Compilador C compatible con OpenMP (por ejemplo, gcc o clang con soporte OpenMP).
- Make (opcional, recomendado si se incluye `Makefile`).
- Entorno Linux / POSIX.

Recomendado:

- gcc >= 5.0 (con `-fopenmp`).
- OMP_NUM_THREADS ajustable según el número de núcleos disponibles.

## Compilación

Si el proyecto contiene un `Makefile`, lo más sencillo es usar:

```bash
make
```

Si no hay `Makefile`, puedes compilar todos los archivos `.c` del repositorio con un comando genérico (ajusta según la estructura del proyecto):

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

Descomprimir un archivo:

```bash
./compressor decompress archivo_salida.comp archivo_recuperado.bin
```

Opciones comunes (ejemplos que tu implementación puede soportar):

- `-t N` o `--threads N`: fuerza el número de hilos OpenMP a usar.
- `-v` o `--verbose`: salida detallada con estadísticas.
- `-l` o `--level`: nivel de compresión (si aplica).

Ejemplo configurando hilos por variable de entorno:

```bash
export OMP_NUM_THREADS=4
./compressor compress input.txt output.comp
```

## Algoritmo y formato de archivo

Indica aquí brevemente el algoritmo elegido (por ejemplo, Huffman, LZ77, RLE, o una combinación) y el formato de fichero de salida. Si el formato es propio, documenta la estructura de cabecera y bloques de datos para facilitar interoperabilidad y pruebas.

Ejemplo (plantilla):

- Cabecera: firma mágica (4 bytes), versión (1 byte), flags (1 byte), tamaño original (8 bytes), información adicional...
- Bloques: cada bloque contiene tamaño comprimido, tamaño original y datos comprimidos.

Si aún no has definido un formato fijo, apunta a un documento `docs/FORMAT.md` o comenta en el código las estructuras utilizadas.

## Rendimiento y OpenMP

Consejos para medir y optimizar:

- Usa `clock_gettime(CLOCK_MONOTONIC, ...)` o `gettimeofday` para medir tiempos con precisión.
- Mide velocidad en MB/s (tamaño_original / tiempo_total).
- Prueba con distintos valores de `OMP_NUM_THREADS` y datos de diferentes tamaños para ver la escalabilidad.
- Asegúrate de que las regiones paralelas sean lo suficientemente costosas para amortizar la sobrecarga de creación/sincronización de hilos.

Advertencias:

- El paralelismo puede cambiar comportamiento en orden de ejecución — asegúrate de que las secciones críticas estén protegidas (por ejemplo, reducciones y actualizaciones a estructuras compartidas).
- Si usas asignaciones grandes por hilo, ten en cuenta el consumo de memoria.

## Pruebas y verificación

Pruebas mínimas que debes ejecutar:

- Comprimir y luego descomprimir varios archivos (texto, binarios, imágenes) y verificar que la salida coincida con la entrada (por ejemplo con `cmp` o `sha256sum`).
- Medir tiempos y comparar versiones con/sin OpenMP.

Ejemplo de test simple:

```bash
./compressor compress archivo.dat archivo.dat.comp
./compressor decompress archivo.dat.comp archivo_recovered.dat
sha256sum archivo.dat archivo_recovered.dat
# deben coincidir
```

## Estructura sugerida del repositorio

- `src/` - código fuente en C
- `include/` - headers públicos
- `tests/` - casos de prueba y scripts para medir rendimiento
- `docs/` - documentación adicional (FORMATO, benchmark, diseño)
- `Makefile` - reglas de compilación y pruebas

Si no tienes esa estructura, considera reorganizar los archivos para mejorar mantenibilidad.

## Contribuciones

Pull requests y issues son bienvenidos. Para cambios importantes, abre primero un issue describiendo la propuesta. Mantén el estilo de código C coherente y añade pruebas para funcionalidades nuevas.

## Licencia

Incluye aquí la licencia que desees (por ejemplo, MIT, GPL-3.0). Si no has decidido, añade un `LICENSE` con la licencia elegida.

## Miembros del equipo

- Jerónimo Acosta Acevedo
- Miguel Ángel Cano Salinas
- Delvin José Rodríguez Jiménez
- Wendy Daniela Benítez Gómez

Si prefieres nombres en mayúsculas o con acentos distintos, dímelo y lo ajusto.

## Contacto y agradecimientos

Para preguntas, reportes de bugs o mejoras, abre un issue en este repositorio o contacta al equipo responsable.

---

Archivo generado automáticamente: README para el proyecto de compresión/descompresión en C con OpenMP.
# compressing-descompressing-in-C