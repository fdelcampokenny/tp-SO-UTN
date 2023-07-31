#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdio.h>
#include <commons/log.h>
#include <commons/string.h>
#include <math.h>
#include <commons/bitarray.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <stdbool.h>
#include <semaphore.h>
#include "sockets.h"
#include "estructuras.h"
#include "shared_utils.h"
#include "serializacion.h"

#define PATH_LOG_MEMORIA "./cfg/memoria.log"
#define PATH_CONFIG_MEMORIA "./cfg/memoria.config"

sem_t sem_peticiones_kernel;
sem_t sem_peticiones_cpu;
sem_t sem_peticiones_fileSystem;

sem_t sem_cpu_fs;
sem_t sem_fs_cpu;

pthread_mutex_t mutex_lectura;
pthread_mutex_t mutex_escritura;

typedef struct{
    char* ip_memoria;
    char* puerto_memoria;
    int tam_memoria;
    int tam_segment_cero;
    int cantidad_segment;
    double retardo_memoria;
    double retardo_compactacion;
    char* algoritmo_asignacion;
}t_config_memoria;

int id_global;
int tamanio;
int id_segmento;
t_bitarray* bitmap_memoria;

t_config_memoria config_memoria;
t_log *logger_memoria;
t_config* config5;

//sockets globales
int servidor_memoria;
int conexion_kernel;
int conexion_cpu;
int conexion_fileSystem;

pthread_t hilo_procesador_kernel;
pthread_t hilo_procesador_cpu;
pthread_t hilo_procesador_fileSystem;

t_list* tabla_segmentos; //por cada proceso
t_list* tabla_segmentos_global; //lista de muchos t_tabla_segmentos

t_bitarray segmentos_libres;
t_segmento* segmento_0;

void* memoria;

t_list* delete_segment(int id_segmento, int pid);
void inicializar_segmentacion();
//void* copiar_segmento(t_segmento* segmento);
t_list* copiar_segmento(t_list* segmentos);
void inicializar_memoria(char**);

void inicializar_procesador_peticiones_kernel();
void inicializar_procesador_peticiones_cpu();
void inicializar_procesador_peticiones_fileSystem();

int32_t create_segment(int id, int tamanio, int pid);
void agregar_ordenado_por_id(t_list* segmentos, t_segmento* nuevo_segmento);
void compactar_CRACKED();
//void resetear_bitmap(t_bitarray* bitmap, int base, int tamanio);
void resetear_bitmap();
void actualizar_bitmap(t_bitarray* bitmap_seg, int numero_segmento);
void actualizar_bitmap_a_cero(t_bitarray* bitmap, t_segmento* segmento);
void actualizar_bitmap_a_uno(t_bitarray* bitmap, t_segmento* segmento);
void actualizar_compactacion(t_list* seg_no_compactados, t_list* seg_compactados);
void actualizar_cada_segmento(t_segmento* segmento_viejo, t_segmento* segmento_nuevo);
void unificar_huecos();
void guardar_en_memoria(void* contenido, t_segmento* segmento, int tamanio);
void ocupar_memoria(void* contenido,int base, int tamanio);
void mov_out(char* valor, double direccion_fisica, uint32_t tamanio_a_escribir);
t_list* limpiar_segmentos_en_memoria(t_list* segmentos, t_segmento* segmento);
void limpiar_tabla_segmentos(t_list* segmentos, t_segmento* segmento);
void  limpiar_tabla(int pid);
int mismo_id(t_segmento* un_segmento);
int contar_bytes_ocupados_desde(t_bitarray* bitmap, int a);
int contar_bytes_libres_desde(t_bitarray* bitmap, int a);
int generar_id_segmento();
void inicializar_segmento_0();
int bitsToBytes(int bits);
t_segmento* buscar_segmento(int tamanio);
t_segmento* buscar_hueco(int* base);
t_segmento* elegir_segun_criterio(t_list* segmentos_candidatos, int tamanio);
t_segmento* segmento_best_fit(t_list* segmentos_candidatos, int tamanio);
t_segmento* segmento_worst_fit(t_list* segmentos_candidatos, int tamanio);
void* segmento_mayor_tamanio(void* segmento, void* otro);
void* segmento_menor_tamanio(void* segmento, void* otro);
t_segmento* guardar_contenido(void* contenido, int tamanio);
t_list* buscar_huecos();
t_list* pueden_guardar(int tamanio, t_list* segmentos_libres);
void resultado_compactacion();
//t_list* copiar_contenido_segmentos(t_list* segmentos_ocupados);

char* crear_bitmap_memoria(int bytes);

char* get_value(double direccion_fisica, uint32_t tamanio_registro);
void write_value(void* ram_auxiliar, char* contenido, double direccion_fisica, uint32_t tamanio_a_escribir);
void write_value_segmento(void* memoria, char* contenido, t_segmento* asd);


bool tiene_mismo_id(int pid);
t_tabla_segmentos* buscar_tabla(int pid);
void procesador_peticiones_cpu();
void procesador_peticiones_kernel();
void procesador_peticiones_fileSystem();
void liberar_tabla(int pid);
t_list* tabla_segmentos_inicial();
t_segmento* buscar_segmento_por_id(t_list* segmentos, int id_segmento);
t_tabla_segmentos* inicializar_tabla_segmentos(int pid);
//borrar:
void imprimir_bitmap();
bool sirve_compactar(t_list* huecos_candidatos, int tamanio);

#endif