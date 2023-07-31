cd /home/utnso/
# MODIF ACA

verde=$(tput setaf 2)
cian=$(tput setaf 6)

predet=$(tput sgr0)

echo "${verde} REMOVER COMMONS/TPS ANTIGUAS ${predet}"


rm -rf so-commons-library




# Instalacion de las commons
echo "${verde} Instalando Commons${predet}"
sleep 1

git clone https://github.com/sisoputnfrba/so-commons-library.git
cd so-commons-library
make install

cd ../

echo "${verde} COMMONS DONE ${predet}"

echo "${verde} Clonando REPO...${predet}"

sleep 1

directorio_base="/home/utnso/tp-2023-1c-Grupo-"
# MODIF ACA

## LIMPIAR MODULOS
echo "${verde}Limpiando modulos${predet}"
sleep 1

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


# Ejecutar el comando ifconfig
ifconfig_output=$(ifconfig)

# Obtener la IP "inet" usando expresiones regulares
ip_inet=$(echo "$ifconfig_output" | grep -oP 'inet \K[\d.]+')

# Imprimir la IP "inet"
echo "${cian}La dirección IP 'inet' es: $ip_inet ${predet}"



echo "${verde} TODO LISTO PAPAAA ${predet}"
