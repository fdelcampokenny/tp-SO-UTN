#ifndef KERNEL_H
#define KERNEL_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include "sockets.h"
#include "shared_utils.h"
#include "serializacion.h"
#include "estructuras.h"
#include <semaphore.h>
#include <pthread.h>


#define PATH_LOG_KERNEL "./cfg/kernel.log"
#define PATH_CONFIG_KERNEL "./cfg/kernel.config"

typedef struct{
    char* ip_memoria;
	char* puerto_memoria;
    char* ip_file_system;
	char* puerto_file_system;
    char* ip_cpu;
	char* puerto_cpu;
    char* ip_kernel;
    char* puerto_kernel;
    char* algoritmo;
    double estimacion_inicial;
    double hrrn_alfa; //ojo con si es double o float o int qcyo
    int grado_max_multiprogramacion;
    char** recursos;
    char** instancias_recursos;

}t_config_kernel;

//variables globales
t_config_kernel config_kernel;
t_log* logger_kernel;
t_config* config2;
t_dictionary *tabla_global_archivos;
t_list *colas_bloqueados_recursos;
t_dictionary* diccionario_recursos;
t_dictionary* diccionario_recursos_bloqueados;
t_dictionary* diccionario_proceso_x_recurso;
t_list *tabla_x_proceso;
t_dictionary *colas_archivos; 


uint32_t PID=0;
pthread_t hilo_largo_plazo;
pthread_t hilo_corto_plazo;
pthread_t hilo_mensajes_cpu;
pthread_t hilo_mensajes_memoria;
pthread_t hilo_mensajes_fileSystem;

pthread_mutex_t mutex_PID;
pthread_mutex_t mutex_LISTA_NEW;
pthread_mutex_t mutex_LISTA_READY;
pthread_mutex_t mutex_LISTA_BLOCKED;
pthread_mutex_t mutex_LISTA_EXEC;
pthread_mutex_t mutex_LISTA_EXIT;

pthread_mutex_t mutex_INSTANCIAS_RECURSOS;
pthread_mutex_t mutex_COLAS_BLOCKEADOS;
pthread_mutex_t mutex_RECURSOS; 
pthread_mutex_t mutex_ARCHIVOS;

sem_t sem_planificador_largo_plazo;
sem_t sem_planificador_corto_plazo;
sem_t sem_cpu;
sem_t contador_multiprogramacion;
sem_t sem_compactacion;

//listas globales de estados
t_list* LISTA_NEW;
t_list* LISTA_READY;
t_list* LISTA_EXEC;
t_list* LISTA_BLOCKED;
t_list* LISTA_EXIT;

//sockets globales
int servidor_kernel;
int conexion_cpu;
int conexion_memoria;
int conexion_file_system;
int tiempo;

void inicializar_kernel(char**);
void procesar_consola(void* socket_cliente);
t_pcb* crear_pcb(char* instrucciones, int socket_consola);


void iniciar_planificador_largo_plazo();
void planificador_largo_plazo();

void iniciar_planificador_corto_plazo();
void planificador_corto_plazo();


t_pcb* algoritmo_HRRN();
t_pcb* algoritmo_FIFO();
void* hrrn_mayor(t_pcb* pcb1, t_pcb* pcb2);
double estimador_rafaga(t_pcb* pcb);
int timenow();

void procesar_bloqueados(t_pcb* pcb);
void procesar_bloqueados_truncate(t_pcb* pcb);
void procesar_bloqueados_write(t_pcb* pcb);
void procesar_bloqueados_read(t_pcb* pcb);

void iniciar_receptor_mensajes_cpu();
void receptor_mensajes_cpu();


void cambiar_estado(t_pcb* pcb, int estado_nuevo);

void inicializar_listas_recursos();
t_dictionary* diccionario_recurso_en_cero();

void actualizar_tablas_segmentos_x_proceso(t_list* tabla_global);

char* loggear_lista_ready();
void liberar_todas_instancias(t_pcb* pcb);

void wait(char* pid, char* recurso);
void signal_d(char *pid, char* recurso);
void asignar_instancias(char* pid, char* recurso);

void actualizar_rafagas();


#endif