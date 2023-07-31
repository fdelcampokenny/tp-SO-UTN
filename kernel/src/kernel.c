#include "kernel.h" 
#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

int main(int argc, char **argv)
{
	/*
	
	printf("El tiempo en segundos actual del dia es");
	
	int tiempo = 0;
	tiempo= timenow();
	printf("El tiempo en segundos actual del dia es: %d", tiempo);
	sleep(10);
	tiempo=timenow();
	printf(" \n El tiempo en segundos actual del dia es: %d", tiempo);
	sleep(6);
	tiempo=timenow();
	printf(" \n El tiempo en segundos actual del dia es: %d", tiempo);
	sleep(100);
*/

	inicializar_kernel(argv);
	inicializar_listas_recursos();


	servidor_kernel = iniciar_servidor(config_kernel.ip_kernel, config_kernel.puerto_kernel, logger_kernel);
	conexion_cpu = iniciar_cliente(config_kernel.ip_cpu, config_kernel.puerto_cpu, logger_kernel);
	conexion_memoria = iniciar_cliente(config_kernel.ip_memoria, config_kernel.puerto_memoria, logger_kernel);
	conexion_file_system = iniciar_cliente(config_kernel.ip_file_system, config_kernel.puerto_file_system, logger_kernel);

	iniciar_planificador_largo_plazo();
	iniciar_planificador_corto_plazo();

	iniciar_receptor_mensajes_cpu();
		

	while (escuchar_cliente(servidor_kernel, procesar_consola, logger_kernel));

}

// esta funcion es la que llamará el hilo. Lo que hace es recibir como argumento el fd
// de "cliente", recibe el pcb del cliente y manda un entero como respuesta.
void procesar_consola(void* socket_cliente){
	int cliente = *((int*)socket_cliente);
	
	t_paquete* paquete = recibir_paquete(cliente); //recibe un paquete generico

	char* lista_ins = deserializar_instrucciones(paquete->buffer);
	t_pcb* pcb = crear_pcb(lista_ins, cliente);
	free(lista_ins);

	cambiar_estado(pcb, NEW);
	eliminar_paquete(paquete);

}

void planificador_largo_plazo(){ 
	while(1){
		sem_wait(&sem_planificador_largo_plazo);
		sem_wait(&contador_multiprogramacion);


		pthread_mutex_lock(&mutex_LISTA_NEW);
		t_pcb* pcb = (t_pcb*) list_remove(LISTA_NEW, 0);
		pthread_mutex_unlock(&mutex_LISTA_NEW);
	
		// ? Inicializa esctructuras de segmentos del proceso
		t_paquete* paquete = serializar_pcb(pcb);
		enviar_paquete(conexion_memoria, INIT_PROCESO, paquete);
		// ? Recibimos la tabla de segmentos (solo con segmento 0)
		paquete = recibir_paquete(conexion_memoria);
		t_tabla_segmentos* tabla = deserializar_tabla_segmentos(paquete->buffer);
		
		pcb->tabla_segmentos = tabla->segmentos;

		cambiar_estado(pcb, READY);
		
	}
}

void planificador_corto_plazo(){

	while(1){
		sem_wait(&sem_planificador_corto_plazo);
		//sem_wait(&sem_cpu);	

		t_pcb* pcb;

	
		if(!strcmp(config_kernel.algoritmo, "HRRN")) {
			pcb = algoritmo_HRRN();
		} else if (!strcmp(config_kernel.algoritmo, "FIFO")){
			pcb = algoritmo_FIFO();
		} else {
			log_error(logger_kernel, "El algoritmo de planificacion ingresado no existe\n");
		}

		if(pcb->primera_aparicion){
			pcb->primera_aparicion = false;
		}

		cambiar_estado(pcb, EXEC);

		t_paquete* paquete = serializar_pcb(pcb);

		enviar_paquete(conexion_cpu, PROCESO_A_RUNNING, paquete);

	}
}

t_pcb* algoritmo_HRRN(){

	pthread_mutex_lock(&mutex_LISTA_READY);
	tiempo = timenow();
	t_pcb* pcb= (t_pcb*) list_get_maximum(LISTA_READY, (void*)hrrn_mayor);
	int pid = pcb->pid;
	pthread_mutex_unlock(&mutex_LISTA_READY);

 	bool _remover_por_pid(void* elemento) {
			return (((t_pcb*)elemento)->pid == pcb->pid);
	}
	
	pthread_mutex_lock(&mutex_LISTA_READY);
	list_remove_by_condition(LISTA_READY,_remover_por_pid);
	pthread_mutex_unlock(&mutex_LISTA_READY);

	return pcb;
}

void actualizar_rafagas(){
	double estimacion_new;
	t_list_iterator* iterador_ready = list_iterator_create(LISTA_READY);
	while(list_iterator_has_next(iterador_ready)) {
		t_pcb* pcb = list_iterator_next(iterador_ready);


		estimacion_new = estimador_rafaga(pcb);
		pcb->estimacion_actual= estimacion_new;
	}	

	list_iterator_destroy(iterador_ready);

}

void *hrrn_mayor(t_pcb *pcb1, t_pcb *pcb2)
{
	double s1 = estimador_rafaga(pcb1);
	double s2 = estimador_rafaga(pcb2);
	double w1 = tiempo - pcb1->llegada_ready;
	double w2 = tiempo - pcb2->llegada_ready;
	


	
	//log_warning(logger_kernel, "El tiempo de  PID <%d>:  <%f> PID <%d>:  <%f>",pcb1->pid,w1,pcb2->pid,w2);

	double rr1 = 1 + (w1 / s1);
	double rr2 = 1 + (w2 / s2);

	log_warning(logger_kernel, "El estimador rafaga de  PID <%d>:  <%f> PID <%d>:  <%f>",pcb1->pid,s1,pcb2->pid,s2);
	log_error(logger_kernel, "El ratio de  PID <%d>:  <%f> PID <%d>:  <%f>",pcb1->pid,rr1,pcb2->pid,rr2);
	

	if (rr1 >= rr2)
	{
		return ((t_pcb *)pcb1);
	}
	else
	{
		return ((t_pcb *)pcb2);
	}
}

double estimador_rafaga(t_pcb *pcb)
{
	double r = pcb->estimacion_actual;
	double e = pcb->rafaga_anterior_real;
	double a = config_kernel.hrrn_alfa;
	double estimador;

	//log_info(logger_kernel, "PID <%d> los valores para el calculo son: Est_actual <%f>, raf_real <%f>, alpha: <%f>", pcb->pid,r,e,a);
	if(!pcb->primera_aparicion){
		estimador= a * r + (1 - a) * e;
	}else{
		estimador = r;
	}
		
	return estimador;
}

t_pcb *algoritmo_FIFO()
{
	pthread_mutex_lock(&mutex_LISTA_READY);
	t_pcb *pcb = (t_pcb *)list_remove(LISTA_READY, 0);
	pthread_mutex_unlock(&mutex_LISTA_READY);
	return pcb;
}

void receptor_mensajes_cpu()
{
	//t_queue *cola_del_archivo = queue_create();
	//t_paquete *paquete_fs;
	t_paquete *paquete;
	t_mensaje_filesystem *mensaje_filesystem;
	char* nombre_archivo;

	while (1)
	{
		nombre_archivo=string_new();
		paquete = recibir_paquete(conexion_cpu);
		t_pcb *pcb = deserializar_pcb(paquete->buffer);
		
		tabla_x_proceso = pcb->tabla_archivo_x_proceso;
		char* piddd= string_itoa(pcb->pid);

		// * FUNCIONES LAMBDA
		bool _es_segmento(void* un_segmento) {
        		return (((t_segmento*)un_segmento)->id_segmento == pcb->id_segmento);
    	}
		
		bool tiene_archivo(void* tabla_archivo){
			return strcmp(((t_archivo_x_proceso*)tabla_archivo)->nombre_archivo, nombre_archivo) == 0;
		}
		
		switch (paquete->opcode)
		{
			case EXIT:
			{
				cambiar_estado(pcb, EXITED);

				break;
			}// DONE
			case YIELD:
			{
				cambiar_estado(pcb, READY);
				break;
			}// DONE
			case F_OPEN:
			{
				t_paquete * paquete_fs = recibir_paquete(conexion_cpu);
				mensaje_filesystem = deserializar_mensaje_filesystem(paquete_fs->buffer);
				nombre_archivo = string_duplicate(mensaje_filesystem->nombre_archivo);

				log_info(logger_kernel, "PID <%d> - Abrir archivo <%s>", pcb->pid, nombre_archivo);

				if (dictionary_has_key(tabla_global_archivos, nombre_archivo))
				{ // ! SI esta en la global
					log_info(logger_kernel, "Agrego archivo <%s> a tabla del proceso <%d>.", nombre_archivo, pcb->pid);
					
					t_archivo_x_proceso *archivo = malloc(sizeof(t_archivo_x_proceso));
					archivo->nombre_archivo = string_new();
					archivo->nombre_archivo = string_duplicate(nombre_archivo);
					archivo->puntero = 0;
					list_add(tabla_x_proceso, archivo);
					
					pthread_mutex_lock(&mutex_ARCHIVOS);	
					t_queue* cola_del_archivo = dictionary_get(colas_archivos, nombre_archivo);
					queue_push(cola_del_archivo, pcb);
					pthread_mutex_unlock(&mutex_ARCHIVOS);

					
					log_info(logger_kernel, "PID: <%d> - Bloqueado por <%s>", pcb->pid, nombre_archivo);
					cambiar_estado(pcb, BLOCKED);
					//dictionary_put(colas_archivos, nombre_archivo, cola_del_archivo);
					
					
					
					break;
				}
				else //! NO esta en la global
					{ 
						pthread_mutex_lock(&mutex_ARCHIVOS);
						dictionary_put(tabla_global_archivos, nombre_archivo, 1);
						t_queue* cola_vacia = queue_create();
						dictionary_put(colas_archivos, nombre_archivo, cola_vacia); //? ya le pongo la cola vacia
						pthread_mutex_unlock(&mutex_ARCHIVOS);

						//? LE DIGO A FILESYSTEM QUE LO ABRA
						t_paquete * paquete_fs = serializar_mensaje_filesystem(0, 0, 0, nombre_archivo, pcb->pid);
						enviar_paquete(conexion_file_system, F_OPEN, paquete_fs);
						
						//? ESPERO RTA DE FILESYSTEM
						paquete_fs = recibir_paquete(conexion_file_system);
						mensaje_filesystem = deserializar_mensaje_filesystem(paquete_fs->buffer);

						if (paquete_fs->opcode == ARCHIVO_INEXISTENTE) //? No existe archivo
						{
							log_info(logger_kernel, "Archivo Inexistente");
							paquete_fs = serializar_mensaje_filesystem(0, 0, 0, nombre_archivo, pcb->pid);
							enviar_paquete(conexion_file_system, F_CREATE, paquete_fs);
							log_info(logger_kernel, "Solicito a FS crear archivo <%s>", nombre_archivo);
						}else if(paquete_fs->opcode == OK) {
							log_info(logger_kernel, "Recibo OK de FS, Archivo abierto");
						}
						t_archivo_x_proceso *archivo = malloc(sizeof(t_archivo_x_proceso));
						archivo->nombre_archivo = string_duplicate(nombre_archivo);
						archivo->puntero = 0;
						list_add(tabla_x_proceso, archivo);

						cambiar_estado(pcb, READY);
						//sem_post(&sem_cpu);
						break;
					}
						

				break;
			}
			case F_CLOSE:
			{
				t_paquete* paquete_fs = recibir_paquete(conexion_cpu);
				mensaje_filesystem = deserializar_mensaje_filesystem(paquete_fs->buffer);
				nombre_archivo = string_duplicate(mensaje_filesystem->nombre_archivo);

				t_archivo_x_proceso* archivo_a_eliminar = list_find(tabla_x_proceso, tiene_archivo);
					
				//list_remove(tabla_x_proceso, archivo_a_eliminar);

				bool _remover_por_nombre(void* elemento) {
					return (strcmp(((t_archivo_x_proceso*)elemento)->nombre_archivo, archivo_a_eliminar->nombre_archivo ) == 0);
				}
				list_remove_by_condition(tabla_x_proceso, _remover_por_nombre);
							
				// ? Administracion de tabla GLOBAL
				if(dictionary_has_key(tabla_global_archivos,nombre_archivo)){ //* Comprobacion medio absurda pero util 
					pthread_mutex_lock(&mutex_ARCHIVOS);
					t_queue* cola_del_archivo= dictionary_get(colas_archivos,nombre_archivo);
					pthread_mutex_unlock(&mutex_ARCHIVOS);			
					if(queue_is_empty(cola_del_archivo)){
						log_info(logger_kernel, "Archivo <%s> Cerrado - Ningun PID esperando", nombre_archivo);
						dictionary_remove(tabla_global_archivos, nombre_archivo);
					}
					else if(queue_size(cola_del_archivo) != 0){
						pthread_mutex_lock(&mutex_ARCHIVOS);			
						cola_del_archivo= dictionary_get(colas_archivos,nombre_archivo);
						t_pcb* pcb_desbloqueado = queue_pop(cola_del_archivo);
						pthread_mutex_unlock(&mutex_ARCHIVOS);
					
						log_info(logger_kernel, "Archivo <%s> Cerrado - PID <%d> Desbloqueado", nombre_archivo, pcb_desbloqueado->pid);			
						cambiar_estado(pcb_desbloqueado,READY);
						//sem_post(&sem_cpu);
					}
				}	
				paquete = serializar_pcb(pcb);
				enviar_paquete(conexion_cpu, PROCESO_A_RUNNING, paquete);
				break;
			}
			case F_SEEK:
			{
				t_paquete* paquete_fs = recibir_paquete(conexion_cpu);
				mensaje_filesystem = deserializar_mensaje_filesystem(paquete_fs->buffer);
				nombre_archivo = string_duplicate(mensaje_filesystem->nombre_archivo);
				uint32_t puntero_new = mensaje_filesystem->puntero;
					
				t_archivo_x_proceso* archivo_a_eliminar = list_find(tabla_x_proceso, tiene_archivo);
				//list_remove(tabla_x_proceso, archivo_a_eliminar);

				bool _remover_por_nombre(void* elemento) {
					return (strcmp(((t_archivo_x_proceso*)elemento)->nombre_archivo, archivo_a_eliminar->nombre_archivo ) == 0);
				}
				list_remove_by_condition(tabla_x_proceso, _remover_por_nombre);
				
				t_archivo_x_proceso* archivo_agregar= malloc(sizeof(t_archivo_x_proceso));
				archivo_agregar->nombre_archivo= nombre_archivo;
				archivo_agregar->puntero = puntero_new;
				
				list_add(tabla_x_proceso, archivo_agregar); // TODO ver que onda

				log_info(logger_kernel, "PID: <%d> - Actualizar puntero Archivo: <%s> - Puntero <%d>",pcb->pid, nombre_archivo,puntero_new);

				paquete = serializar_pcb(pcb);
				enviar_paquete(conexion_cpu, PROCESO_A_RUNNING, paquete);			
				break;
			}
			case F_TRUNCATE:
			{
				
				t_paquete* paquete_fs = recibir_paquete(conexion_cpu);
				mensaje_filesystem = deserializar_mensaje_filesystem(paquete_fs->buffer);
				//nombre_archivo = mensaje_filesystem->nombre_archivo;
				nombre_archivo = string_duplicate(mensaje_filesystem->nombre_archivo);
				uint32_t tamanio = mensaje_filesystem->tamanio;
				log_info(logger_kernel, "PID: <%d> - Archivo: <%s> - Tamaño: <%d>", pcb->pid, nombre_archivo, tamanio);
				
				paquete_fs = serializar_mensaje_filesystem(0, tamanio, 0, nombre_archivo, pcb->pid);
				enviar_paquete(conexion_file_system, F_TRUNCATE, paquete_fs);
				//cambiar_estado(pcb, BLOCKED);

				
				pthread_t hilo_bloqueados;
				pthread_create(&hilo_bloqueados, NULL, (void *)procesar_bloqueados_truncate, pcb);
				pthread_detach(hilo_bloqueados);
				//cambiar_estado(pcb, READY);
				//sem_post(&sem_cpu);

				break;
			}
			case F_READ:
			{
				sem_wait(&sem_compactacion);
				

				t_paquete* paquete_fs = recibir_paquete(conexion_cpu);
				mensaje_filesystem = deserializar_mensaje_filesystem(paquete_fs->buffer);

				//nombre_archivo = mensaje_filesystem->nombre_archivo;
				nombre_archivo = string_duplicate(mensaje_filesystem->nombre_archivo);
				uint32_t tamanio = mensaje_filesystem->tamanio;
				double dirFisica = mensaje_filesystem->direccion_fisica;
				t_archivo_x_proceso* archivo = list_find(tabla_x_proceso, tiene_archivo);
				uint32_t* puntero = archivo->puntero;

				log_info(logger_kernel, "PID: <%d> - Leer Archivo: <%s> - Puntero <%d> - Dirección Memoria <%f> - Tamaño <%d>",pcb->pid, nombre_archivo, puntero, dirFisica, tamanio);

				paquete_fs = serializar_mensaje_filesystem(puntero, tamanio, dirFisica, nombre_archivo,pcb->pid);
				enviar_paquete(conexion_file_system, F_READ, paquete_fs);
				

	
				
				sem_post(&sem_compactacion);
				
				pthread_t hilo_bloqueados;
				pthread_create(&hilo_bloqueados, NULL, (void *)procesar_bloqueados_read, pcb);
				pthread_detach(hilo_bloqueados);
				break;
			}
			case F_WRITE:
			{
				
				sem_wait(&sem_compactacion);
				t_paquete* paquete_fs = recibir_paquete(conexion_cpu);
				mensaje_filesystem = deserializar_mensaje_filesystem(paquete_fs->buffer);
				nombre_archivo = string_duplicate(mensaje_filesystem->nombre_archivo);

				uint32_t tamanio = mensaje_filesystem->tamanio;
				double dirFisica = mensaje_filesystem->direccion_fisica;
				uint32_t puntero;
				t_list_iterator* iterador = list_iterator_create(tabla_x_proceso);
				int i = 0;
				int tamanio_lista = list_size(tabla_x_proceso);
				//log_info(logger_kernel, "Cant tablas del proceso: %d", tamanio_lista);

				//DEJAR ESTO, SUSTITUYE AL LIST FIND DEL DIABLO
				while(i<tamanio_lista) {
					t_archivo_x_proceso* arch =  list_get(tabla_x_proceso, i);
					if(strcmp(arch->nombre_archivo, nombre_archivo) == 0){
					puntero = arch->puntero;
				//	log_info(logger_kernel, "encontre el archivo, puntero obtenido");
					}
					i++;
				}
				
				//t_archivo_x_proceso* archivo = list_find(tabla_x_proceso, tiene_archivo); //MUERTE AL LIST FIND
				//uint32_t puntero = archivo->puntero;
				

				log_info(logger_kernel, "PID: <%d> - Escribir Archivo: <%s> - Puntero <%d> - Dirección Memoria <%f> - Tamaño <%d>",pcb->pid, nombre_archivo, puntero, dirFisica, tamanio);
				paquete_fs = serializar_mensaje_filesystem(puntero, tamanio, dirFisica, nombre_archivo, pcb->pid);
				enviar_paquete(conexion_file_system, F_WRITE, paquete_fs);
				
				sem_post(&sem_compactacion);


				pthread_t hilo_bloqueados;
				pthread_create(&hilo_bloqueados, NULL, (void *)procesar_bloqueados_write, pcb);
				pthread_detach(hilo_bloqueados);


				break;
			}
			case SEG_FAULT:
			{
			cambiar_estado(pcb,SEG_FAULT);
			break;
			}
			case CREATE_SEGMENT:
			{
				log_info(logger_kernel, "PID: <%d> - Crear Segmento - Id: <%d> - Tamaño: <%d>", pcb->pid, pcb->id_segmento, pcb->tamanio_segmento);
				
				t_paquete *paquete = serializar_pcb(pcb);
				enviar_paquete(conexion_memoria, CREATE_SEGMENT, paquete);

				int32_t base;
				recv(conexion_memoria, &base, sizeof(int32_t),0);
				log_info(logger_kernel, "Recibi la base: %d", base);
				
				//t_paquete *paquete_mem= recibir_paquete(conexion_memoria); 
				//base = des_mensaje_only(paquete_mem->buffer);
			

				if (base == -1)
				{ //? Out of memory chaval		
					cambiar_estado(pcb, OUT_OF_MEMORY);
					break;
				}
				else if (base == -2)
				{ // ? COMPACTAR
					sem_wait(&sem_compactacion);
					
					log_info(logger_kernel, "Compactacion: <se solicitó compactación> "); // TODO ver el otro caso esperando finalizar operaciones fs

					paquete = serializar_pcb(pcb);
					enviar_paquete(conexion_memoria, COMPACTAR, paquete);
					
					paquete = recibir_paquete(conexion_memoria);
					t_list* tabla_segmentos_global = deserializar_tabla_global_segmentos(paquete->buffer);

					log_info(logger_kernel, "Se finalizo el proceso de compactacion");
					
					
					
					pthread_mutex_lock(&mutex_LISTA_EXEC);
					list_remove(LISTA_EXEC, 0);
					list_add(LISTA_EXEC, pcb);
					pthread_mutex_unlock(&mutex_LISTA_EXEC);
					
					actualizar_tablas_segmentos_x_proceso(tabla_segmentos_global); //falta hacer, habra que recorrer las listas de ready block y exec
					
					

					log_info(logger_kernel, "A crear segmento: %d con tamanio: %d", pcb->id_segmento, pcb->tamanio_segmento);
					
					
					t_paquete *paquete = serializar_pcb(pcb);

					//VUELVO A MANDAR CREATE_SEGMENT
					enviar_paquete(conexion_memoria, CREATE_SEGMENT, paquete); // vuelvo a pedir create_segment post compactacion y actualiacion
																			// ahora si voy a recibir bien la base del segmento creado
					recv(conexion_memoria, &base, sizeof(int32_t), 0); //recibo la base de nuevo segmento
					t_segmento* niu_segmento = malloc(sizeof(t_segmento));
					niu_segmento->direccion_base = base;
					niu_segmento->id_segmento = pcb->id_segmento;
					niu_segmento->tamanio = pcb->tamanio_segmento;
					list_add(pcb->tabla_segmentos, niu_segmento);

					sem_post(&sem_compactacion); //permito que filesystem pueda volver a acceder a memoria

					log_info(logger_kernel, "se creo el segmento sin problemas");
					
					
					cambiar_estado(pcb, READY);
					
					break;
					
				}
				else
				{
					//sem_wait(&sem_compactacion);
					log_info(logger_kernel, "se creo el segmento sin problemas");

					t_segmento* niu_segmento = malloc(sizeof(t_segmento));
					niu_segmento->direccion_base = base;
					niu_segmento->id_segmento = pcb->id_segmento;
					niu_segmento->tamanio = pcb->tamanio_segmento;

					list_add(pcb->tabla_segmentos, niu_segmento);

					log_info(logger_kernel, "tamanio tabla: %d", list_size(pcb->tabla_segmentos));


					//paquete = serializar_pcb(pcb);
					//enviar_paquete(conexion_cpu, PROCESO_A_RUNNING, paquete);
					cambiar_estado(pcb, READY);
					//sem_post(&sem_cpu);

					//sem_post(&sem_compactacion);
					break;
				}

				break;
			}
			case DELETE_SEGMENT:
			{
				//sem_wait(&sem_compactacion); // TODO semaforos
				
				log_info(logger_kernel, "PID: <%d> - Eliminar Segmento - Id: <%d>", pcb->pid,pcb->id_segmento);
				t_paquete *paquete = serializar_pcb(pcb);
				enviar_paquete(conexion_memoria, DELETE_SEGMENT, paquete);
				
				//list_remove_by_condition(pcb->tabla_segmentos, _es_segmento);
				
				t_paquete *paquete_tabla = recibir_paquete(conexion_memoria);
				t_tabla_segmentos *tabla_actualizada = deserializar_tabla_segmentos(paquete_tabla->buffer);
				
				log_info(logger_kernel, "Recibi la tabla actualizada del proceso: %d", tabla_actualizada->pid);
				
				pcb->tabla_segmentos = tabla_actualizada->segmentos; // quiero creer que asi se actualiza y no hay que hacer memcpy ni nada raro
				
				//paquete = serializar_pcb(pcb);
				//enviar_paquete(conexion_cpu, PROCESO_A_RUNNING, paquete);
				cambiar_estado(pcb, READY);
				//sem_post(&sem_cpu);
				
				//sem_post(&sem_compactacion);
				
				break;
			}

			case I_O:
			{
				
				pthread_t hilo_bloqueados;
				pthread_create(&hilo_bloqueados, NULL, (void *)procesar_bloqueados, pcb);
				pthread_detach(hilo_bloqueados);
				break;
			}
			case WAIT:
			{
				char *recurso = pcb->recurso_solicitado;
				char* pid= string_itoa(pcb->pid);
				t_queue* cola_bloqueados= queue_create();
				if (dictionary_has_key(diccionario_recursos, recurso))
				{
					pthread_mutex_lock(&mutex_RECURSOS);
					int instancias = dictionary_get(diccionario_recursos, recurso);
					pthread_mutex_unlock(&mutex_RECURSOS);
					if (instancias - 1 < 0)
					{
						wait(pid,recurso);
						
						log_info(logger_kernel, "PID: <%d> - Bloqueado por: <%s>",pcb->pid ,recurso);
						cambiar_estado(pcb,BLOCKED);
						

						pthread_mutex_lock(&mutex_COLAS_BLOCKEADOS);
						cola_bloqueados = dictionary_get(diccionario_recursos_bloqueados, recurso);	
						queue_push(cola_bloqueados, pcb);


						dictionary_put(diccionario_recursos_bloqueados, recurso, cola_bloqueados);
						pthread_mutex_unlock(&mutex_COLAS_BLOCKEADOS);
												

						//queue_destroy(cola_bloqueados);
						break;

					}
					else // SI SE PUEDE ASIGNAR
					{
						wait(pid, recurso);
						asignar_instancias(pid, recurso);

						t_paquete *paquete = serializar_pcb(pcb);
						enviar_paquete(conexion_cpu, PROCESO_A_RUNNING, paquete);						
						break;	
					}
				}
				else
				{
					log_info(logger_kernel, "Recurso: %s No encontrado - <INVALID_RESOURCE>",recurso);
					cambiar_estado(pcb, INVALID_RESOURCE);
					//sem_post(&sem_cpu);
					break;
				}
			}
			case SIGNAL:
			{
				char* recurso= pcb->recurso_solicitado;
				char* pid= string_itoa(pcb->pid);
				t_queue* cola_bloq= queue_create();
				
				if(dictionary_has_key(diccionario_recursos, recurso))
				{
					signal_d(pid, recurso); // hago el signal
					pthread_mutex_lock(&mutex_COLAS_BLOCKEADOS);
					cola_bloq = dictionary_get(diccionario_recursos_bloqueados,recurso);
					pthread_mutex_unlock(&mutex_COLAS_BLOCKEADOS);

					if(queue_size(cola_bloq) != 0){
						pthread_mutex_unlock(&mutex_COLAS_BLOCKEADOS);
						t_pcb* pcb_ready= queue_pop(cola_bloq);
						pthread_mutex_unlock(&mutex_COLAS_BLOCKEADOS);
						char* pid_ready=string_itoa(pcb_ready->pid);

						log_info(logger_kernel, "Proceso desbloqueado %s", pid_ready);

						cambiar_estado(pcb_ready,READY);
						//sem_post(&sem_cpu);
						
					}
				}
				else{
					log_info(logger_kernel, "Recurso: %s NO encontrado - <INVALID_RESOURCE>",recurso);
					cambiar_estado(pcb, INVALID_RESOURCE);
					//sem_post(&sem_cpu);
					break;
				}

				t_paquete *paquete = serializar_pcb(pcb);
				enviar_paquete(conexion_cpu, PROCESO_A_RUNNING, paquete);
				break;
			}
			
		}
	}
}

//hay que revisar que pasa con la tabla del proceso actual
void actualizar_tablas_segmentos_x_proceso(t_list* tabla_global){
	t_list_iterator* iterador_tabla = list_iterator_create(tabla_global);
	
	//------------------------------------------------------//
	int tamanio_tabla_global = list_size(tabla_global);
	int tamanio_ready = list_size(LISTA_READY);
	int tamanio_blocked = list_size(LISTA_BLOCKED);
	int tamanio_exec = list_size(LISTA_EXEC); // tiene que ser 1
	log_info(logger_kernel, "tamanio tabla global: %d", tamanio_tabla_global);
	log_info(logger_kernel, "tamanio tabla global: %d", tamanio_ready);
	log_info(logger_kernel, "tamanio tabla global: %d", tamanio_blocked);
	log_info(logger_kernel, "tamanio tabla global: %d", tamanio_exec);
	int i = 0;
	int r = 0;
	int b = 0;
	int e = 0;
	//------------------------------------------------------//
	//aca recorro toda la tabla global, agarrando por iteracion una tabla de un proceso
    while(list_iterator_has_next(iterador_tabla)) {
        t_tabla_segmentos* tabla = list_iterator_next(iterador_tabla);
		//------------------------------------------------------//
		log_info(logger_kernel, "iteracion tabla global: %d", i);
		i++;
	    //------------------------------------------------------//
		bool _mismo_pid(void* pcb) {
        	return (((t_pcb*)pcb)->pid == tabla->pid);
    	}

		if(list_any_satisfy(LISTA_READY, _mismo_pid)){
			t_list_iterator* iterador_ready = list_iterator_create(LISTA_READY);
			while(list_iterator_has_next(iterador_ready)) {
				//------------------------------------------------------//
				log_info(logger_kernel, "iteracion tabla READY: %d", r);
				r++;
				//------------------------------------------------------//
				t_pcb* pcb = list_iterator_next(iterador_ready);
				if(pcb->pid == tabla->pid){
					pcb->tabla_segmentos = tabla->segmentos;
				}
			
			}
			list_iterator_destroy(iterador_ready);
		}else if(list_any_satisfy(LISTA_BLOCKED, _mismo_pid)){
			t_list_iterator* iterador_blocked = list_iterator_create(LISTA_BLOCKED);
			while(list_iterator_has_next(iterador_blocked)) {
				//------------------------------------------------------//
				log_info(logger_kernel, "iteracion tabla BLOCKED: %d", b);
				b++;
				//------------------------------------------------------//
				t_pcb* pcb = list_iterator_next(iterador_blocked);
				if(pcb->pid == tabla->pid){
					pcb->tabla_segmentos = tabla->segmentos;
				}
			
			}
			list_iterator_destroy(iterador_blocked);
		}else{
			t_list_iterator* iterador_exec = list_iterator_create(LISTA_EXEC);
			while(list_iterator_has_next(iterador_exec)) {
				//------------------------------------------------------//
				log_info(logger_kernel, "iteracion tabla EXEC: %d", e);
				e++;
				//------------------------------------------------------//
				t_pcb* pcb = list_iterator_next(iterador_exec);
				if(pcb->pid == tabla->pid){
					pcb->tabla_segmentos = tabla->segmentos;
				}
				
			}
			list_iterator_destroy(iterador_exec);
		}
        
    }

    list_iterator_destroy(iterador_tabla);
	
}

void procesar_bloqueados(t_pcb* pcb){ //puede que haya que meter a los bloqueados de wait y signal (no quiero hacer eso)
	log_info(logger_kernel, "PID: <%d> - Ejecuta IO: <%f>", pcb->pid, pcb->tiempo_recurso);
	log_info(logger_kernel, "PID <%d> - Bloqueado por: I/O", pcb->pid);
	cambiar_estado(pcb, BLOCKED);
    sleep(pcb->tiempo_recurso);
	cambiar_estado(pcb, READY);
	//sem_post(&sem_cpu);
}

void procesar_bloqueados_truncate(t_pcb* pcb){ //puede que haya que meter a los bloqueados de wait y signal (no quiero hacer eso)
	cambiar_estado(pcb, BLOCKED);
	//sem_post(&sem_cpu);
	//sem_post(&sem_cpu);
	//t_paquete* paquete_fs = recibir_paquete(conexion_file_system);

	u_int32_t valor;
	recv(conexion_file_system, &valor, sizeof(u_int32_t),0);
	log_info(logger_kernel, "Filesystem: OK - Truncate terminado");
	cambiar_estado(pcb, READY);
	//sem_post(&sem_cpu);
	
}

void procesar_bloqueados_read(t_pcb* pcb){ //puede que haya que meter a los bloqueados de wait y signal (no quiero hacer eso)
	cambiar_estado(pcb, BLOCKED);
	//sem_post(&sem_cpu);
	//sem_post(&sem_cpu);
	//sem_post(&sem_compactacion);
	//t_paquete* paquete_fs = recibir_paquete(conexion_file_system);

		u_int32_t valor;
		recv(conexion_file_system, &valor, sizeof(u_int32_t),0);
		log_info(logger_kernel, "Filesystem: OK - F_READ Finalizado");
		cambiar_estado(pcb, READY);
		//sem_post(&sem_cpu);
}

void procesar_bloqueados_write(t_pcb* pcb){ //puede que haya que meter a los bloqueados de wait y signal (no quiero hacer eso)
	cambiar_estado(pcb, BLOCKED);
	//sem_post(&sem_cpu);
	//sem_post(&sem_cpu);
	//sem_post(&sem_compactacion);
	//t_paquete* paquete_fs = recibir_paquete(conexion_file_system);
		u_int32_t valor;
		recv(conexion_file_system, &valor, sizeof(u_int32_t),0);
		log_info(logger_kernel, "Filesystem: OK - F_WRITE Finalizado");
		cambiar_estado(pcb, READY);
		//sem_post(&sem_cpu);
		

}



void cambiar_estado(t_pcb* pcb, int estado_nuevo) 
{
	int estado_anterior = pcb->estado_actual;
	pcb->estado_actual = estado_nuevo;
	char* global_estado_anterior;
	t_paquete* paquete;
	if (estado_anterior == EXEC && estado_nuevo != EXITED)
	{
		pcb->rafaga_anterior_real = timenow() - pcb->llegada_running;
	}
	bool _remover_por_pid(void* elemento) {
			return (((t_pcb*)elemento)->pid == pcb->pid);
	}

	switch (estado_anterior){ //! ESTADO ANTERIOR
		case NEW:
		{
			global_estado_anterior="NEW";
			pthread_mutex_lock(&mutex_LISTA_NEW);
			list_remove_by_condition(LISTA_NEW, _remover_por_pid);
			pthread_mutex_unlock(&mutex_LISTA_NEW);
			break;
		}
		case READY:{
			global_estado_anterior="READY";
			pthread_mutex_lock(&mutex_LISTA_READY);
			list_remove_by_condition(LISTA_READY, _remover_por_pid);
			pthread_mutex_unlock(&mutex_LISTA_READY);
			break;
		}
		case EXEC:
		{
			actualizar_rafagas();
			global_estado_anterior="EXEC";
			pthread_mutex_lock(&mutex_LISTA_EXEC);
			list_remove_by_condition(LISTA_EXEC, _remover_por_pid);
			pthread_mutex_unlock(&mutex_LISTA_EXEC);
			
		
			break;
		}
		case BLOCKED:
		{
			global_estado_anterior="BLOCKED";
			pthread_mutex_lock(&mutex_LISTA_BLOCKED);
			list_remove_by_condition(LISTA_BLOCKED, _remover_por_pid);
			pthread_mutex_unlock(&mutex_LISTA_BLOCKED);		
			break;
		}
		}
	switch(estado_nuevo){ // ! ESTADO NUEVO
		case NEW:
		{
			pthread_mutex_lock(&mutex_LISTA_NEW);
			list_add(LISTA_NEW, pcb);
			log_info(logger_kernel, "Se crea el proceso <%d> en NEW" ,pcb->pid);
			pthread_mutex_unlock(&mutex_LISTA_NEW);
	
			sem_post(&sem_planificador_largo_plazo);
			break;
		}
		case READY:
		{
			pcb->llegada_ready=timenow();

			
			pthread_mutex_lock(&mutex_LISTA_READY);
			log_info(logger_kernel, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <READY>", pcb->pid,global_estado_anterior);
			list_add(LISTA_READY, pcb);
			pthread_mutex_unlock(&mutex_LISTA_READY);
			sem_post(&sem_planificador_corto_plazo);
			
			char* lista_ready= string_new();
			lista_ready= loggear_lista_ready();
			
			log_info(logger_kernel, "Cola ready <%s> : %s",config_kernel.algoritmo,lista_ready);
			
			break;
		}
		case EXEC:
		{
			pcb->llegada_running=timenow();
			
			
			

			pthread_mutex_lock(&mutex_LISTA_EXEC);
			list_add(LISTA_EXEC, pcb);
			pthread_mutex_unlock(&mutex_LISTA_EXEC);
			log_info(logger_kernel, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <EXEC>",pcb->pid, global_estado_anterior);
	
			break;
		}
		case BLOCKED:
		{
			pthread_mutex_lock(&mutex_LISTA_BLOCKED);
			list_add(LISTA_BLOCKED, pcb);
			pthread_mutex_unlock(&mutex_LISTA_BLOCKED);

			//sem_post(&sem_cpu);

			log_info(logger_kernel, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <BLOCKED>",pcb->pid, global_estado_anterior);

			break;
		}
		// ? Todos los casos de salida de un proceso.
		case EXITED:{
			
			log_info(logger_kernel, "Finaliza el proceso <%d> - Motivo: <SUCCESS>", pcb->pid);
			
			paquete = serializar_pcb(pcb);
			enviar_paquete(conexion_memoria, FIN_PROCESO, paquete);
			liberar_todas_instancias(pcb);

			sem_post(&contador_multiprogramacion);
			pthread_mutex_lock(&mutex_LISTA_EXIT);
			list_add(LISTA_EXIT, pcb);
			pthread_mutex_unlock(&mutex_LISTA_EXIT);
			int socket_consola = pcb->fd_conexion;
			send(socket_consola, &estado_nuevo, sizeof(int),0);
			break;
		}
		case SEG_FAULT:{
			log_info(logger_kernel, "Finaliza el proceso <%d> - Motivo: <SEG_FAULT>", pcb->pid);
			
			liberar_todas_instancias(pcb);
			paquete = serializar_pcb(pcb);
			enviar_paquete(conexion_memoria, FIN_PROCESO, paquete);

			sem_post(&contador_multiprogramacion);
			pthread_mutex_lock(&mutex_LISTA_EXIT);
			list_add(LISTA_EXIT, pcb);
			pthread_mutex_unlock(&mutex_LISTA_EXIT);
			int socket_consola = pcb->fd_conexion;
			
			send(socket_consola, &estado_nuevo, sizeof(int),0);
			break;
		}
		case INVALID_RESOURCE:{
			log_info(logger_kernel, "Finaliza el proceso <%d> - Motivo: <INVALID_RESOURCE>", pcb->pid);
			
			paquete = serializar_pcb(pcb);
			enviar_paquete(conexion_memoria, FIN_PROCESO, paquete);
			liberar_todas_instancias(pcb);


			sem_post(&contador_multiprogramacion);
			pthread_mutex_lock(&mutex_LISTA_EXIT);
			list_add(LISTA_EXIT, pcb);
			pthread_mutex_unlock(&mutex_LISTA_EXIT);
			int socket_consola = pcb->fd_conexion;
			send(socket_consola, &estado_nuevo, sizeof(int),0);
			break;
		}
		case OUT_OF_MEMORY:{
			log_info(logger_kernel, "Finaliza el proceso <%d> - Motivo: <OUT_OF_MEMORY>", pcb->pid);
			
			paquete = serializar_pcb(pcb);
			enviar_paquete(conexion_memoria, FIN_PROCESO, paquete);
			liberar_todas_instancias(pcb);



			sem_post(&contador_multiprogramacion);
			pthread_mutex_lock(&mutex_LISTA_EXIT);
			list_add(LISTA_EXIT, pcb);
			pthread_mutex_unlock(&mutex_LISTA_EXIT);
			int socket_consola = pcb->fd_conexion;
			send(socket_consola, &estado_nuevo, sizeof(int),0);
			break;
		}
	}
	
	
}


void inicializar_listas_recursos(){ 
	int instancia_recurso;
	char* recurso;
	int tam = string_array_size(config_kernel.recursos);
	for (int i = 0; i < tam; i++)
	{
		// * Diccionario recursos
		instancia_recurso=atoi(config_kernel.instancias_recursos[i]);
		recurso = config_kernel.recursos[i];

		dictionary_put(diccionario_recursos, recurso, instancia_recurso);
	
		// * COLA DE RECURSOS BLOQUEADOS
		t_queue* cola_bloq= queue_create();
		dictionary_put(diccionario_recursos_bloqueados,recurso,cola_bloq);

	}

}


int timenow()
{
	time_t now = time(NULL);
	struct tm *local = localtime(&now);
	int hours, minutes, seconds; //

	hours = local->tm_hour;
	minutes = local->tm_min;
	seconds = local->tm_sec;

	int segundos_totales;
	segundos_totales = hours * 60 * 60 + minutes * 60 + seconds;
	return segundos_totales;
}

t_pcb *crear_pcb(char *instrucciones, int socket_consola)
{
	t_pcb *nuevoPCB = malloc(sizeof(t_pcb));

	nuevoPCB->instrucciones = string_new();
	nuevoPCB->instrucciones = string_duplicate(instrucciones);
	nuevoPCB->pc = 0;
	nuevoPCB->recurso_solicitado = string_new();

	nuevoPCB->primera_aparicion = true;


	nuevoPCB->tabla_archivo_x_proceso = list_create();
	nuevoPCB->tabla_segmentos = list_create();

	pthread_mutex_lock(&mutex_PID);
	nuevoPCB->pid = PID;
	PID++;
	pthread_mutex_unlock(&mutex_PID);

	nuevoPCB->estimacion_actual = config_kernel.estimacion_inicial;
	nuevoPCB->tabla_archivo_x_proceso = list_create(); 
	nuevoPCB->fd_conexion = socket_consola;
	nuevoPCB->rafaga_anterior_real = 0;

	nuevoPCB->tamanio_segmento = 0;
	nuevoPCB->id_segmento = 0;

	nuevoPCB->AX = string_new();
	nuevoPCB->BX = string_new();
	nuevoPCB->CX = string_new();
	nuevoPCB->DX = string_new();
	
	nuevoPCB->EAX = string_new();
	nuevoPCB->EBX = string_new();
	nuevoPCB->ECX = string_new();
	nuevoPCB->EDX = string_new();

	nuevoPCB->RAX = string_new();
	nuevoPCB->RBX = string_new();
	nuevoPCB->RCX = string_new();
	nuevoPCB->RDX = string_new();


	
	t_dictionary* diccionario = diccionario_recurso_en_cero();
	
	dictionary_put(diccionario_proceso_x_recurso ,string_itoa(nuevoPCB->pid), diccionario);

	return nuevoPCB;
}



void inicializar_kernel(char** argv)
{
	logger_kernel = iniciar_logger(PATH_LOG_KERNEL, "KERNEL");
	char* path_archivo = malloc((strlen(argv[1]) + 1) * sizeof(char));
    strcpy(path_archivo, argv[1]);
	config2 = iniciar_config(path_archivo);

	config_kernel.ip_kernel = config_get_string_value(config2, "IP_KERNEL");
	config_kernel.puerto_kernel = config_get_string_value(config2, "PUERTO_ESCUCHA");

	config_kernel.ip_memoria = config_get_string_value(config2, "IP_MEMORIA");
	config_kernel.puerto_memoria = config_get_string_value(config2, "PUERTO_MEMORIA");

	config_kernel.ip_file_system = config_get_string_value(config2, "IP_FILESYSTEM");
	config_kernel.puerto_file_system = config_get_string_value(config2, "PUERTO_FILESYSTEM");

	config_kernel.ip_cpu = config_get_string_value(config2, "IP_CPU");
	config_kernel.puerto_cpu = config_get_string_value(config2, "PUERTO_CPU");

	config_kernel.algoritmo = config_get_string_value(config2, "ALGORITMO_PLANIFICACION");
	config_kernel.estimacion_inicial = config_get_double_value(config2, "ESTIMACION_INICIAL");
	config_kernel.hrrn_alfa = config_get_double_value(config2, "HRRN_ALFA");
	config_kernel.grado_max_multiprogramacion = config_get_int_value(config2, "GRADO_MAX_MULTIPROGRAMACION");

	config_kernel.recursos = config_get_array_value(config2, "RECURSOS");
	config_kernel.instancias_recursos = config_get_array_value(config2, "INSTANCIAS_RECURSOS");

	diccionario_recursos=dictionary_create();
	diccionario_recursos_bloqueados=dictionary_create();
	tabla_global_archivos=dictionary_create();
	diccionario_proceso_x_recurso = dictionary_create();
	tabla_x_proceso = list_create();
	colas_archivos = dictionary_create();

	//* Inicializo listas
	LISTA_NEW = list_create();
	LISTA_READY = list_create();
	LISTA_EXEC = list_create();
	LISTA_BLOCKED = list_create();
	LISTA_EXIT = list_create();

	sem_init(&sem_planificador_largo_plazo, 0, 0);
	sem_init(&sem_planificador_corto_plazo, 0, 0);
	sem_init(&sem_compactacion, 0, 1);
	sem_init(&sem_cpu, 0, 1);
	sem_init(&contador_multiprogramacion, 0, config_kernel.grado_max_multiprogramacion);
}

void iniciar_planificador_largo_plazo()
{
	pthread_create(&hilo_largo_plazo, NULL, (void *)planificador_largo_plazo, NULL);
	//log_info(logger_kernel, "Inicio planificador largo plazo");
	pthread_detach(hilo_largo_plazo);
}

void iniciar_planificador_corto_plazo()
{
	pthread_create(&hilo_corto_plazo, NULL, (void *)planificador_corto_plazo, NULL);
	//log_info(logger_kernel, "Inicio planificador corto plazo");
	pthread_detach(hilo_corto_plazo);
}

void iniciar_receptor_mensajes_cpu()
{
	pthread_create(&hilo_mensajes_cpu, NULL, (void *)receptor_mensajes_cpu, NULL);
	//log_info(logger_kernel, "Inicio mensajes cpu");
	pthread_detach(hilo_mensajes_cpu);
}

t_dictionary* diccionario_recurso_en_cero(){
	t_dictionary* diccionario_recurso= dictionary_create();
	char* recurso;
	int tam = string_array_size(config_kernel.recursos);
	for (int i = 0; i < tam; i++)
	{
		int num = 0;
		recurso = config_kernel.recursos[i];
		dictionary_put(diccionario_recurso, recurso, num);
	
	}
	return diccionario_recurso;
}

char *loggear_lista_ready()
{
	char *lista_log= string_new();
	string_append(&lista_log, "[");
	int32_t pid;
	int i = 0;
	if (!list_is_empty(LISTA_READY))
	{
		while (i < list_size(LISTA_READY))
		{
			pthread_mutex_lock(&mutex_LISTA_READY);
			t_pcb *pcb = list_get(LISTA_READY, i);
			pthread_mutex_unlock(&mutex_LISTA_READY);
			
			pid = pcb->pid;
			char *pid_char = string_itoa(pid);

			string_append(&lista_log, pid_char);
			string_append(&lista_log, ",");
			i++;
		}
	}
	
	string_append(&lista_log, "]");
	return lista_log;
}

void liberar_todas_instancias(t_pcb *pcb)
{

	char *pid = string_itoa(pcb->pid);
	pthread_mutex_lock(&mutex_INSTANCIAS_RECURSOS);
	t_dictionary *diccionario_rec = dictionary_get(diccionario_proceso_x_recurso, pid);
	pthread_mutex_unlock(&mutex_INSTANCIAS_RECURSOS);

	t_list *lista_keys = list_create();
	lista_keys = dictionary_keys(diccionario_rec);
	// lista con todas las keys
	int instancias = 0;
	int instancias_globales;
	char *recurso = string_new();

	t_pcb *pcb_desblock;
	//for (i = 0; i < list_size(lista_keys); i++)
	t_list_iterator* iterador = list_iterator_create(lista_keys);
	while(list_iterator_has_next(iterador)) //el problema esta aca, cuando meto el elemento en aux
	{

		recurso = (char*)list_iterator_next(iterador);
		//recurso = list_get(lista_keys, i);
		// ? Agrego instancias globlaes
		pthread_mutex_lock(&mutex_INSTANCIAS_RECURSOS);
		instancias = dictionary_get(diccionario_rec, recurso);
		pthread_mutex_unlock(&mutex_INSTANCIAS_RECURSOS);		
		
		pthread_mutex_lock(&mutex_RECURSOS);
		instancias_globales = dictionary_get(diccionario_recursos, recurso);
		instancias_globales += instancias;
		dictionary_put(diccionario_recursos, recurso, instancias_globales);
		pthread_mutex_unlock(&mutex_RECURSOS);

		log_warning(logger_kernel, "PID <%s> :Instancias <%d> liberadas - Recurso <%s>", pid,instancias, recurso);
		
		pthread_mutex_lock(&mutex_COLAS_BLOCKEADOS);
			t_queue* cola_block = dictionary_get(diccionario_recursos_bloqueados, recurso);
		pthread_mutex_unlock(&mutex_COLAS_BLOCKEADOS);

		//log_info(logger_kernel, "llegue ac1");

		// ? Desbloqueo procesos
		while(instancias > 0 && !queue_is_empty(cola_block))
		{
				//log_info(logger_kernel, "llegue ac2");
				//log_info(logger_kernel, "cantidad bloqueados: %d,", queue_size(cola_block)); no llega aca al paracer
				pcb_desblock = queue_pop(cola_block);

				//log_info(logger_kernel, "llegue ac3");
				
				cambiar_estado(pcb_desblock, READY);
				//sem_post(&sem_cpu);
				char* pid_ready = string_itoa(pcb_desblock->pid);
				wait(pid_ready, recurso);

				instancias--;
		}
	}
}

void wait(char* pid, char* recurso)
{

	pthread_mutex_lock(&mutex_RECURSOS); // HAGO EL WAIT
	int instancias = dictionary_get(diccionario_recursos, recurso);
	int aux = instancias - 1;
	dictionary_put(diccionario_recursos, recurso, aux);
	pthread_mutex_unlock(&mutex_RECURSOS);

	log_warning(logger_kernel, "PID <%s> - Wait: <%s> - Instancias: <%d>",pid, recurso, aux);						

}

void signal_d(char* pid, char* recurso)
{

	pthread_mutex_lock(&mutex_RECURSOS);
	int instancias = dictionary_get(diccionario_recursos, recurso);
	int aux = instancias + 1;
	dictionary_put(diccionario_recursos, recurso, aux);
	pthread_mutex_unlock(&mutex_RECURSOS);

	log_warning(logger_kernel, "PID <%s> - Signal: <%s> - Instancias: <%d>", pid, recurso, aux);

	pthread_mutex_lock(&mutex_INSTANCIAS_RECURSOS);
	t_dictionary *diccionario = dictionary_get(diccionario_proceso_x_recurso, pid);
	int instanciasA = dictionary_get(diccionario, recurso);
	int instancias_asignadas = MAX(instanciasA - 1, 0);
	dictionary_put(diccionario, recurso, (instancias_asignadas));
	dictionary_put(diccionario_proceso_x_recurso, pid, diccionario);

	pthread_mutex_unlock(&mutex_INSTANCIAS_RECURSOS);
}

void asignar_instancias(char* pid, char* recurso){

	int instancias;
	pthread_mutex_lock(&mutex_INSTANCIAS_RECURSOS); // AGREGO A LA CUENTA DE INSTANCIAS DE ESE PROCESO
	t_dictionary *diccionario = dictionary_get(diccionario_proceso_x_recurso, pid);
	instancias = dictionary_get(diccionario, recurso);
	instancias += 1;
	dictionary_put(diccionario, recurso, instancias);
	dictionary_put(diccionario_proceso_x_recurso, pid, diccionario);
	pthread_mutex_unlock(&mutex_INSTANCIAS_RECURSOS);
}