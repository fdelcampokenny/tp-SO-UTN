#ifndef SERIALIZACION_H
#define SERIALIZACION_H

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include "shared_utils.h"
#include "estructuras.h"
#include "sockets.h"

void* serializar_paquete(t_paquete* paquete);
void eliminar_paquete(t_paquete* paquete);
void enviar_paquete(int socket_destino, uint8_t opcode, t_paquete* paquete);
t_paquete* recibir_paquete(int socket_cliente);
t_paquete* serializar_pcb(t_pcb* pcb);
t_pcb* deserializar_pcb(t_buffer* buffer);
t_paquete* serializar_instrucciones(char* lista_ins);
char* deserializar_instrucciones(t_buffer* buffer);




t_tabla_segmentos* deserializar_tabla_segmentos(t_buffer* buffer);
t_paquete* serializar_tabla_segmentos(t_tabla_segmentos* tabla);
int calcular_bytes_tabla_segmentos(t_tabla_segmentos* tabla);


t_paquete* serializar_tabla_global_segmentos(t_list* tabla_global);
t_list* deserializar_tabla_global_segmentos(t_buffer* buffer);

t_paquete* serializar_escritura_memoria(char* valor, double direccionFisica, uint32_t tamanio_registro, uint32_t pid);
t_escritura_memoria* deserializar_escritura_memoria(t_buffer* asd);


t_paquete* serializar_lectura_memoria(double dirFisica, uint32_t tamanio, uint32_t pid);
t_lectura_memoria* deserializar_lectura_memoria(t_buffer*);

t_paquete* serializar_mensaje_filesystem(uint32_t puntero, uint32_t tamanio, double direccion_fisica, char* nombre, uint32_t pid);
t_mensaje_filesystem* deserializar_mensaje_filesystem(t_buffer* buffer);


t_paquete* mensaje_only(int32_t contenido);
int32_t des_mensaje_only (t_buffer* buffer);

t_paquete* serializar_lista_archivos(t_list* lista_archivos);
t_list* deserializar_lista_archivos(t_buffer* buffer);

#endif