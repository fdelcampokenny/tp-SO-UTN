#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <commons/memory.h>
#include <dirent.h>
#include <stdbool.h>
#include "sockets.h"
#include "estructuras.h"
#include "shared_utils.h"
#include "serializacion.h"
#include <sys/types.h>
#include <sys/mman.h>
#include <commons/collections/dictionary.h>
#include <sys/stat.h>
#include <fcntl.h>


#define PATH_LOG_FILESYSTEM "./cfg/fileSystem.log"
#define PATH_CONFIG_FILESYSTEM "./cfg/fileSystem.config"

int servidor_fileSystem; 
int	conexion_kernel; 
int	conexion_memoria; 
pthread_t manejo_peticiones;
pthread_mutex_t mutex;
t_dictionary* diccionario_fcbs;

typedef struct{
    char* ip_memoria;
	char* puerto_memoria;
    char* ip_fileSystem;
	char* puerto_fileSystem;
    char* path_fcb;
    char* path_archivo_bitmap;
    char* path_archivo_superbloques;
    char* path_archivo_bloques;
}t_config_fileSystem;

uint32_t file_descriptor;
typedef struct{
    int BLOCK_SIZE;
    int BLOCK_COUNT;
}t_superBloque;

t_config_fileSystem config_fileSystem;
t_log *logger_fileSystem;
t_config* config4;
t_superBloque super_bloque;
t_config* config_superBloque;
//t_fcb* fcb; 

void inicializar_fileSystem(char**);
void imprimir_bitmap(t_bitarray* bitmap);
void levantar_bitmap();
void levantar_superbloque();
char* f_read(char* nombre_archivo, uint32_t puntero, int tamanio_a_leer);
void f_write(uint32_t puntero, int tamanio_a_escribir, char *nombre_archivo, char *informacion);
void f_truncate(char* nombre_archivo, int bytes);
int f_open(char *nombre_archivo);
void crearBloques();
void escribir_archivo(char *archivo_bloques, uint32_t tamanio_a_escribir, char *informacion, int byte_del_archivo, t_config *config_fcb);
void procesador_peticiones();
void crear_fcb(char* nombre_archivo);
void inicializar_procesador_peticiones();
void iniciar_lista_global_fcbs();
//t_list* buscar_bloques_libres(char* nombre_archivo, int bytes);
t_list* buscar_bloques_libres();
void levantar_directorio_fcbs();

//int encontrar_puntero_en_bloque(uint32_t puntero ,uint32_t puntero_indirecto, char* archivo_fcb);







//RUTAS GLOBALES
char* ruta_directorio_fcb = "/home/utnso/tp-2023-1c-Grupo-/fileSystem/fcbs/";

#endif