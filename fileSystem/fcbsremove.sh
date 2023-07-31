#!/bin/bash

# Directorio base
directorio_base="/home/utnso/tp-2023-1c-Grupo-"

# Ruta completa de la carpeta fcbs
ruta_fcbs="${directorio_base}/fileSystem/fcbs"

# Verificar que la carpeta fcbs exista
if [ -d "$ruta_fcbs" ]; then
  # Eliminar la carpeta fcbs y su contenido
  rm -rf "$ruta_fcbs"
fi

# Crear nuevamente la carpeta fcbs vacía
mkdir "$ruta_fcbs"
