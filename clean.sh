#!/bin/bash

# Directorio base
directorio_base="/home/utnso/tp-2023-1c-Grupo-"

# Array con los nombres de los directorios
directorios=("consola" "cpu" "fileSystem" "kernel" "memoria")

# Función para limpiar un directorio
limpiar_directorio() {
  cd "$1" || exit 1
  make clean
  rm -f cfg/*.log 2>/dev/null
  cd ..
}

# Cambiar al directorio base
cd "$directorio_base" || exit 1

# Limpiar cada directorio
for dir in "${directorios[@]}"; do
  limpiar_directorio "$dir"
done


