#ifndef CONSOLA_H
#define CONSOLA_H


#include<commons/collections/list.h>
#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include<stdbool.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include"sockets.h"
#include"shared_utils.h"
#include"serializacion.h"
#include"estructuras.h"


#define PATH_LOG_CONSOLA "./cfg/consola.log"
#define PATH_CONFIG_CONSOLA "./cfg/consola.config"

typedef struct{
    char* ip_kernel;
    char* puerto_kernel;
}t_config_consola;

t_config_consola config_consola;
t_log* logger_consola;
t_config* config1;

void inicializar_consola();
void procesar_entrada(int argc,char** argv, t_log* logger);
char* armar_lista_instrucciones(char* pathArchivo, t_log* logger);

//t_pcb crear_pcb();


#endif