#include "sockets.h"
#include "estructuras.h"

int iniciar_servidor(char *ip, char* puerto, t_log *logger)
{
	struct addrinfo hints, *server_info;
	int socket_servidor, s;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	s = getaddrinfo(ip, puerto, &hints, &server_info);
	if(s!=0){
		exit(EXIT_FAILURE);
	}

	socket_servidor = socket(server_info->ai_family,
	                    server_info->ai_socktype,
	                    server_info->ai_protocol);

	if(socket_servidor == -1){
		close(socket_servidor);
		exit(EXIT_FAILURE);
	}

	if(bind(socket_servidor, server_info->ai_addr, server_info->ai_addrlen) == -1){
		close(socket_servidor);
		exit(EXIT_FAILURE);
	}

	if(listen(socket_servidor, SOMAXCONN) == -1){
		close(socket_servidor);
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(server_info);
	log_info(logger, "Servidor iniciado");

	return socket_servidor;
} 


int iniciar_cliente(char* ip, char* puerto, t_log* logger)
{
	struct addrinfo hints;
	struct addrinfo *server_info;
	int socket_cliente, s;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	s = getaddrinfo(ip, puerto, &hints, &server_info);

	if(s!=0){ //Error al obtener la info del ip y del puerto
		log_error(logger, "ERROR INFO IP PUERTO");
		exit(EXIT_FAILURE);
		
	}
	
	socket_cliente = socket(server_info->ai_family,
	                    server_info->ai_socktype,
	                    server_info->ai_protocol);

	if(socket_cliente == -1){
		close(socket_cliente);
		exit(EXIT_FAILURE);
		
	}

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen)!= 0){
		close(socket_cliente);
		exit(EXIT_FAILURE);
		
	}

	freeaddrinfo(server_info);
    log_info(logger, "Se creo la conexion cliente");

	return socket_cliente;
}

int esperar_cliente(int socket_servidor, t_log* logger)
{
	int socket_cliente;

	socket_cliente = accept(socket_servidor, NULL, NULL);

	log_info(logger, "Se conecto un cliente!");

	return socket_cliente;
}

int escuchar_cliente(int socket_servidor, void (procesar_conexion)(void*), t_log* logger){
	int cliente = esperar_cliente(socket_servidor, logger);

	//posiblemente haya que mandar como argumento otras cosas ademas del file descriptor "cliente"
	if(cliente != -1){
		pthread_t hilo;
		pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) &cliente);
        pthread_detach(hilo);
		return 1;
	}
	return 0;

}

t_opcode recibir_operacion(int socket_cliente)
{
	t_opcode cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(t_opcode), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void terminar_programa(t_log* logger, t_config* config)
{
	if(logger != NULL){
		log_destroy(logger);
	}

	if(config != NULL){
		config_destroy(config);
	}
}