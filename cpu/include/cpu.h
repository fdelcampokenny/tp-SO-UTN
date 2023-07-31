#ifndef CPU_H
#define CPU_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
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

#define PATH_LOG_CPU "./cfg/cpu.log"
#define PATH_CONFIG_CPU "./cfg/cpu.config"

typedef struct{
    int retardo_instruccion;
    int tam_max_segmento;
    char* ip_memoria;
	char* puerto_memoria;
    char* ip_cpu;
	char* puerto_cpu;
}t_config_cpu;

t_config_cpu config_cpu;
t_log *logger_cpu;
t_config* config3;


pthread_t hilo_ciclo_cpu;

//sockets globales
int servidor_cpu;
int conexion_kernel;
int conexion_memoria;




void inicializar_cpu(char**);
void ciclo_cpu();
void inicializar_ciclo_cpu();

void cicloInstruccion(t_pcb* pcb, t_list* instrucciones);

t_list* parsearInstrucciones(char* lista_instrucciones);
void agregar_a_registro(char* registro, char* valor_c, t_pcb* pcb);


//int iniciar_conexion();

char* obtener_valor_registro( char*, t_pcb*);
//MMU
double traducir_direccion_logica(char* direccion_logica, t_pcb* pcb, uint32_t tamanio);

uint32_t calcular_tamanio_registro(char* registro);
#endif