#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "shared_utils.h"

typedef struct {
    uint32_t pid;
    t_list* segmentos;
} t_tabla_segmentos;

typedef struct {
    uint32_t pid; //serializado
    uint32_t pc; //serializado
    char* instrucciones; //serializado
    double estimacion_actual; //serializado
    double llegada_ready; //serializado
    double llegada_running; //serializado
    t_list* tabla_segmentos;
    char* AX;
    char* BX;
    char* CX;
    char* DX;
    char* EAX;
    char* EBX;
    char* ECX;
    char* EDX;
    char* RAX;
    char* RBX;
    char* RCX;
    char* RDX;
    
    int estado_actual; //serializado
    int fd_conexion; //serializado

    //RECURSOS
    bool primera_aparicion; //serializado
    double rafaga_anterior_real; //serializado
    char* recurso_solicitado; //serializado
    double tiempo_recurso; //serializado

    // MEMORIA
    uint32_t id_segmento; //serializado
    uint32_t tamanio_segmento; //serializado
    //FILESYSTEM
    t_list* tabla_archivo_x_proceso; //TODO cambiar por t_list de structs de nombre + puntero y serializar o
                                            //TODO, ver como serializar un diccionario
}t_pcb;


typedef enum {
    NEW,
    READY,
    EXEC,
    BLOCKED,
	EXITED,
    INIT_PROCESO,
    TABLA_INICIALIZADA,
    TABLA_ACTUALIZADA,
    ACTUALIZAR_TABLA_SEG_GLOBAL,
    NUEVAS_INSTRUCCIONES,
    PROCESO_A_RUNNING,
    OUT_OF_MEMORY,
    F_READ,
    F_WRITE,
    SET,
    MOV_IN,
    MOV_OUT,
    F_TRUNCATE,
    F_SEEK,
    CREATE_SEGMENT,
    FIN_PROCESO,
    LEER_PARA_FILESYSTEM,
    I_O,
    WAIT,
    SIGNAL,
    F_OPEN,
    F_CLOSE,
    DELETE_SEGMENT,
    EXIT,
    YIELD,
    COMPACTAR,
    SEG_FAULT,
    ARCHIVO_INEXISTENTE, 
    F_CREATE,
    OK,
    INVALID_RESOURCE
} t_opcode; //TODO: Nombres de las operaciones, poner nombres representativos

typedef struct {
    uint32_t size; //tamaño del payload, ver si tiene un espacio generico o depende la implementacion. ojo con el tipo, hacer que coincida en serializacion.c
    void* stream; //contenido del payload
} t_buffer;

typedef struct {
    uint8_t opcode; 
    t_buffer* buffer;
} t_paquete;


typedef struct{
	int tamanioMensaje;
	void* mensaje;
}t_mensaje;

typedef struct  {
	t_opcode identificador;
	char* parametro1;
	char* parametro2;
    char* parametro3;
}t_instruccion;

typedef struct {
    uint32_t id_segmento;
    int32_t direccion_base;
    uint32_t tamanio; 
}t_segmento;



typedef struct {
    int numero_segmento;
    int desplazamiento;
} t_direccion_logica;

typedef struct {
    double direccion_fisica;
    char* valor_a_escribir;
    uint32_t tamanio_a_escribir;
    uint32_t pid;
} t_escritura_memoria;

typedef struct {
    double direccion_fisica;
    uint32_t tamanio_registro;
    uint32_t pid;
} t_lectura_memoria;

//FILESYSTEM
typedef struct{
    char* nombre_archivo;
    int tamanio_archivo;
    uint32_t puntero_directo;
    uint32_t puntero_indirecto;
}t_fcb;


typedef struct {
    char* nombre_archivo;
    uint32_t puntero; //este dato lo buscas en la tabla de archivos por proceso
    int cantidad_bytes; //Cantidad a leer (parametro de la instruccion)
    double direccion_fisica_memoria; //lo que lei en el archivo, lo escribo en memoria en esta df 
} t_lectura_archivos;

typedef struct {
    uint32_t puntero; 
    uint32_t tamanio; 
    double direccion_fisica; 
    char* nombre_archivo;
    uint32_t pid; 
} t_mensaje_filesystem; 

typedef struct {
    uint32_t instancia;
    char* nombre_recurso;
} t_recurso; 

typedef struct {
    char* nombre_archivo;
    uint32_t puntero;
} t_archivo_x_proceso;

#endif