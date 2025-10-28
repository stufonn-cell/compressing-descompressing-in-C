# File Compression/Decompression Tool

Herramienta de compresión y descompresión de archivos usando BWT + RLE con paralelización OpenMP e interfaz gráfica GTK.

## Miembros del equipo

- Jerónimo Acosta Acevedo
- Miguel Ángel Cano Salinas
- Delvin José Rodríguez Jiménez
- Wendy Daniela Benítez Gómez

## Características

- Algoritmos: Burrows-Wheeler Transform (BWT) + Run-Length Encoding (RLE)
- Paralelización con OpenMP
- Interfaz gráfica GTK3
- Línea de comandos CLI
- Soporte para archivos individuales y directorios completos

## Requisitos

- GCC con soporte OpenMP
- GTK+ 3.0
- pkg-config

## Compilación

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
