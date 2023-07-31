#ifndef SOCKETS_H
#define SOCKETS_H

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

int iniciar_servidor(char *ip, char* puerto, t_log *logger);
int iniciar_cliente(char *ip, char* puerto, t_log* logger);
int esperar_cliente(int socket_servidor, t_log* logger);
int escuchar_cliente(int socket_servidor, void (procesar_conexion)(void*), t_log* logger);
void terminar_programa(t_log* logger, t_config* config);
t_opcode recibir_operacion(int socket_cliente);


#endif