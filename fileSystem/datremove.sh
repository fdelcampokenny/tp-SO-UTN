#!/bin/bash

# Directorio base
directorio_base="/home/utnso/tp-2023-1c-Grupo-"

# Ruta completa de la carpeta cfg
ruta_cfg="${directorio_base}/fileSystem/cfg"

# Ir al directorio cfg
cd "$ruta_cfg" || exit 1

# Borra todos los archivos .dat en la carpeta cfg
rm -f *.dat 2>/dev/null
