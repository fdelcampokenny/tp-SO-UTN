#include "fileSystem.h"
#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))
int main(int argc, char **argv)
{

	inicializar_fileSystem(argv);

	
	t_list* listaTest = dictionary_keys(diccionario_fcbs);
	t_list_iterator* iteradorTest = list_iterator_create(listaTest);
	while(list_iterator_has_next(iteradorTest)) {
		char* nombre_arch = list_iterator_next(iteradorTest);
		log_warning(logger_fileSystem, "Nombre archivo: %s", nombre_arch);
	}
	

	servidor_fileSystem = iniciar_servidor(config_fileSystem.ip_fileSystem, config_fileSystem.puerto_fileSystem, logger_fileSystem);
	conexion_memoria = iniciar_cliente(config_fileSystem.ip_memoria, config_fileSystem.puerto_memoria, logger_fileSystem);
	conexion_kernel = esperar_cliente(servidor_fileSystem, logger_fileSystem);
	
	levantar_superbloque();
	levantar_bitmap();
	crearBloques();
	inicializar_procesador_peticiones();

/* qsy capaz sirve para la entrega esto
	int tamanioBitmap = ceil((double)super_bloque.BLOCK_COUNT / 8);
	int file_descriptor = open(config_fileSystem.path_archivo_bitmap, O_CREAT | O_RDWR, 0664);
	void *string_bitmap = mmap(NULL, tamanioBitmap, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, 0);
	log_warning(logger_fileSystem, "puntero Bitmap main: %p", string_bitmap);

	t_bitarray *bitmap = bitarray_create_with_mode(string_bitmap, tamanioBitmap, LSB_FIRST);
	for (int i = 0; i < tamanioBitmap * 8; i++)
	{
		if (bitarray_test_bit(bitmap, i) == 0)
		{
			printf("0");
		}
		else
		{
			printf("1");
		}
	}
	printf("\n");
*/
//pthread_mutex_destroy(&mutex);
}

void procesador_peticiones()
{
	while (1)
	{
		t_paquete *paquete_kernel = recibir_paquete(conexion_kernel);
		t_mensaje_filesystem *mensaje_kernel = deserializar_mensaje_filesystem(paquete_kernel->buffer);

		//GUARDAMOS TODO EN VARIABLES
		char *nombre_archivo = mensaje_kernel->nombre_archivo;
		uint32_t puntero = mensaje_kernel->puntero;
		uint32_t tamanio = mensaje_kernel->tamanio;
		double direccion_fisica = mensaje_kernel->direccion_fisica;
		uint32_t pid= mensaje_kernel->pid;
		switch (paquete_kernel->opcode)
		{

		case F_OPEN:
		{
			t_paquete* paquete = serializar_mensaje_filesystem(0, 0, 0, "0",pid);

			if (dictionary_has_key(diccionario_fcbs, nombre_archivo))
			{ // EXISTE

				enviar_paquete(conexion_kernel, OK, paquete);
				log_info(logger_fileSystem, "Abrir Archivo: <%s>", nombre_archivo);
				break;
			}
			else
			{ // NO EXISTE

				enviar_paquete(conexion_kernel, ARCHIVO_INEXISTENTE, paquete);
				log_info(logger_fileSystem, "No existe el archivo <%s>", nombre_archivo);
				break;
			}
			break;
		} // DONE
		case F_CREATE:
		{
			//log_warning(logger_fileSystem, "entro al f_create de fs, nombre archivo: %s",nombre_archivo);
			crear_fcb(nombre_archivo);
			
			log_info(logger_fileSystem, "Crear Archivo: <%s>", nombre_archivo);
			break;
			// DONE
		}
		case F_READ:
		{	
			log_info(logger_fileSystem, "entre a F_READ");
			char* contenido = malloc(tamanio);
			contenido = f_read(nombre_archivo, puntero, tamanio);
			char* string_buffer = malloc(tamanio + 1);
			memcpy(string_buffer, contenido, tamanio);
			memcpy(string_buffer + tamanio, "\0", 1);
			log_warning(logger_fileSystem, "valor leido: %s", string_buffer);
			free(string_buffer);


			log_info(logger_fileSystem, "Leer archivo: <%s> - Puntero: <%d> - Memoria: <%f> - Tamaño: <%d>", nombre_archivo, puntero, direccion_fisica, tamanio);
			t_paquete* paquete = serializar_escritura_memoria(contenido, direccion_fisica, tamanio, pid);
			enviar_paquete(conexion_memoria, F_READ, paquete);
			t_paquete* paquete2 = recibir_paquete(conexion_memoria);
			if(paquete->opcode == OK) {
				log_info(logger_fileSystem, "F_READ en memoria finalizado, envio OK a kernel");
				//enviar_paquete(conexion_kernel, OK, paquete);
				u_int32_t valor = 34;
				send(conexion_kernel, &valor, sizeof(u_int32_t),0);
			}
			free(contenido);
			break;
			// DONE
		}
		case F_TRUNCATE:
		{
			int auxTamanio= (int)tamanio;
			
			log_info(logger_fileSystem, "Truncar Archivo: <%s> - Tamaño: <%d>", nombre_archivo, tamanio);
			f_truncate(nombre_archivo, auxTamanio);
			//t_paquete* paquete= mensaje_only(0);
			//enviar_paquete(conexion_kernel, OK, paquete);
			u_int32_t valor = 34;
			send(conexion_kernel, &valor, sizeof(u_int32_t),0);
			break;
			// DONE
		}

		case F_WRITE:
		{
			log_warning(logger_fileSystem, "Entro a F_WRITE SALVADOR");
			t_paquete* paquete = serializar_lectura_memoria(direccion_fisica, tamanio, pid);
			//pthread_mutex_lock(&mutex);
			enviar_paquete(conexion_memoria, F_WRITE, paquete);
			paquete = recibir_paquete(conexion_memoria);
			//pthread_mutex_unlock(&mutex);
			t_escritura_memoria *mensaje_memoria = malloc(sizeof(t_escritura_memoria));
			mensaje_memoria = deserializar_escritura_memoria(paquete->buffer);

			//log_info(logger_fileSystem, "pid: %d, tamanio: %d", mensaje_memoria->pid, mensaje_memoria->tamanio_a_escribir);
			char *contenido_a_escribir = malloc(tamanio);
			memcpy(contenido_a_escribir, mensaje_memoria->valor_a_escribir, tamanio);
			
			
			
			char* string_buff = malloc(tamanio + 1);
			memcpy(string_buff, contenido_a_escribir, tamanio);
			memcpy(string_buff + tamanio, "\0", 1);
			
			log_warning(logger_fileSystem, "valor a escribir: %s", string_buff);
			
			f_write(puntero, tamanio, nombre_archivo, contenido_a_escribir);
			free(string_buff);
			free(contenido_a_escribir);
			
			log_info(logger_fileSystem, "Escribir archivo: <%s> - Puntero: <%d> - Memoria: <%f> - Tamaño: <%d>", nombre_archivo, puntero, direccion_fisica, tamanio);
			
			//t_paquete* paqueteOnly= mensaje_only(0);
			//enviar_paquete(conexion_kernel, OK, paqueteOnly);
			u_int32_t valor = 34;
			send(conexion_kernel, &valor, sizeof(u_int32_t),0);
			break;
			// DONE
		}

		}
	eliminar_paquete(paquete_kernel);
	free(mensaje_kernel);
	}
}

	char *f_read(char *nombre_archivo, uint32_t puntero, int tamanio_a_leer)
	{

		char *ruta_fcb = malloc(strlen(ruta_directorio_fcb) + strlen(nombre_archivo) + 1);
		strcpy(ruta_fcb, ruta_directorio_fcb);
		strcat(ruta_fcb, nombre_archivo);

		// obtener fcb a partir del nombre_archivo (commons config)
		t_config *config_fcb = dictionary_get(diccionario_fcbs, nombre_archivo);

		// int file_descriptor = open(ruta, O_CREAT | O_RDWR, 0664);
		int fd_fcb = open(ruta_fcb, O_RDONLY);

		// Obtener tamanio archivo (dato del fcb) y guardarlo en variable
		int tamanio_archivo = config_get_int_value(config_fcb, "TAMANIO_ARCHIVO");

		// Usar tamanio archivo para mapearlo
		char *archivo_fcb = mmap(NULL, tamanio_archivo, PROT_READ, MAP_SHARED, fd_fcb, 0);

		// abrir archivo bloques
		int fd_bloques = open(config_fileSystem.path_archivo_bloques, O_RDWR);
		
		int tamanio_bloques = super_bloque.BLOCK_COUNT * super_bloque.BLOCK_SIZE;
		char *archivo_bloques = mmap(NULL, tamanio_bloques, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bloques, 0);
		int puntero_indirecto = config_get_int_value(config_fcb, "PUNTERO_INDIRECTO");
		int puntero_directo = config_get_int_value(config_fcb, "PUNTERO_DIRECTO");
		char *string_buffer = malloc(tamanio_a_leer);
		int byte_inicio_relativo = puntero % super_bloque.BLOCK_SIZE;
		int bytes_posibles = super_bloque.BLOCK_SIZE - byte_inicio_relativo;
		char *puntero_bloque_obtenido = NULL;
		char *puntero_lectura;
		int byte_bloque_indirecto = puntero_indirecto * super_bloque.BLOCK_SIZE;
		int bloque_inicio_lectura = ceil((double)(puntero / super_bloque.BLOCK_SIZE));
		int puntero_indirecto_arranque = fmax(bloque_inicio_lectura - 1, 0) - 1; // cantidad de punteros a saltear
		int bytes_desplazados_indirecto = (puntero_indirecto_arranque * 4);		 // cant de bytes desplazados desde el bloque indirecto
		int bloque_archivo_log = puntero_indirecto_arranque + 2;
		uint32_t *buffer = malloc(4);											 // buffer destinado a alojar punteros uint32_t
		int offset_info = 0;
		int offset = 0;
		int aux;
		if (puntero < super_bloque.BLOCK_SIZE)
		{
			//TODO: log acceso a bloque directo
			log_info(logger_fileSystem, "Acceso Bloque - Archivo <%s> - Bloque Archivo: <0> - Bloque File System <%d>", nombre_archivo, puntero_directo);
			char *puntero_en_directo = archivo_bloques + puntero_directo + (char)byte_inicio_relativo;
			aux = MIN(bytes_posibles, tamanio_a_leer); // aux es el tamanio a escribir por cada iteracion/bloque accedido
			memcpy(string_buffer + offset_info, puntero_en_directo, aux);
			offset_info += aux;
			tamanio_a_leer -= aux;
		}
		if (puntero > super_bloque.BLOCK_SIZE)
		{
			//TODO: log acceso a bloque indirecto
			log_info(logger_fileSystem, "Acceso Bloque - Archivo <%s> - Bloque Archivo: <1> - Bloque File System <%d>", nombre_archivo, puntero_indirecto);
			puntero_bloque_obtenido = archivo_bloques + byte_bloque_indirecto + (char)bytes_desplazados_indirecto;
			memcpy(buffer, puntero_bloque_obtenido, sizeof(uint32_t));

			log_info(logger_fileSystem, "Acceso Bloque - Archivo <%s> - Bloque Archivo: <%d> - Bloque File System <%d>", nombre_archivo, bloque_archivo_log, *buffer);

			puntero_lectura = archivo_bloques + (*buffer * super_bloque.BLOCK_SIZE) + byte_inicio_relativo;
			aux = MIN(bytes_posibles, tamanio_a_leer);
			memcpy(string_buffer + offset_info, puntero_lectura, aux);
			tamanio_a_leer -= aux;
			offset_info += aux;
		}

		while (tamanio_a_leer != 0)
		{
			//TODO: log acceso a bloques obtenidos por punteros del bloque indirecto
			bloque_archivo_log++;
			bytes_desplazados_indirecto += 4;
			puntero_bloque_obtenido = archivo_bloques + byte_bloque_indirecto + (char)bytes_desplazados_indirecto;
			memcpy(buffer, puntero_bloque_obtenido, sizeof(uint32_t));
			log_info(logger_fileSystem, "Acceso Bloque - Archivo <%s> - Bloque Archivo: <%d> - Bloque File System <%d>", nombre_archivo, bloque_archivo_log, *buffer);
			offset += 4;
			int byte_puntero_bloque_obtenido = *buffer * super_bloque.BLOCK_SIZE;
			puntero_lectura = archivo_bloques + (char)byte_puntero_bloque_obtenido;
			aux = MIN(super_bloque.BLOCK_SIZE, tamanio_a_leer);
			memcpy(string_buffer + offset_info, puntero_lectura, aux);
			tamanio_a_leer -= aux;
			offset_info += aux;
		}

		munmap(archivo_fcb, tamanio_archivo);
		munmap(archivo_bloques, super_bloque.BLOCK_COUNT * super_bloque.BLOCK_SIZE);
		free(ruta_fcb);
		return string_buffer;
	}

	void f_write(uint32_t puntero, int tamanio_a_escribir, char *nombre_archivo, char *informacion)
	{
		// abrir fcb y sacar data necesaria
		char *ruta_fcb = malloc(strlen(ruta_directorio_fcb) + strlen(nombre_archivo) + 1);
		strcpy(ruta_fcb, ruta_directorio_fcb);
		strcat(ruta_fcb, nombre_archivo);

		// obtener fcb a partir del nombre_archivo (commons config)
		t_config *config_fcb = dictionary_get(diccionario_fcbs, nombre_archivo);

		int fd_bloques = open(config_fileSystem.path_archivo_bloques, O_RDWR);
		// mapearlo
		int tamanio_bloques = super_bloque.BLOCK_COUNT * super_bloque.BLOCK_SIZE;
		char *archivo_bloques = mmap(NULL, tamanio_bloques, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bloques, 0);
		// ir al bloque usando puntero parametro
		//log_warning(logger_fileSystem, "puntero arch BLOQUES: %p", archivo_bloques);
		escribir_archivo(archivo_bloques, tamanio_a_escribir, informacion, puntero, config_fcb);
		//mem_hexdump(archivo_bloques, super_bloque.BLOCK_COUNT*super_bloque.BLOCK_SIZE);
		msync(&fd_bloques, tamanio_bloques, MS_SYNC);
		munmap(archivo_bloques, tamanio_bloques);
		close(fd_bloques);
	}

	void escribir_archivo(char *archivo_bloques, uint32_t tamanio_a_escribir, char *informacion, int byte_del_archivo, t_config *config_fcb)
	{
		// VARIABLES GLOBALES DE LA FUNCION
		int puntero_directo = config_get_int_value(config_fcb, "PUNTERO_DIRECTO");
		int puntero_indirecto = config_get_int_value(config_fcb, "PUNTERO_INDIRECTO");
		char* nombre_archivo = config_get_string_value(config_fcb, "NOMBRE_ARCHIVO");
		int bloque_inicio_escritura = ceil((double)(byte_del_archivo / super_bloque.BLOCK_SIZE));
		//int byte_inicio_bloque = bloque_inicio_escritura * super_bloque.BLOCK_SIZE;
		int byte_inicio_relativo = byte_del_archivo % super_bloque.BLOCK_SIZE; // forma mas performante de calcular el byte relativo
		int bytes_posibles = super_bloque.BLOCK_SIZE - byte_inicio_relativo;   // bytes que puedo escribir en el primer bloque
		char *puntero_escritura;
		int puntero_indirecto_arranque = MAX(bloque_inicio_escritura - 1, 0) - 1; // cantidad de punteros a saltear
		
		//!log_info(logger_fileSystem, "bloque inicio escritura: %d", bloque_inicio_escritura);
		//!log_info(logger_fileSystem, "byte_inicio_realtivo: %d", byte_inicio_relativo);
		//!log_info(logger_fileSystem, "en el primero bloque puedo escribir <%d> bytes", bytes_posibles);


		int offset = 0;															 // para recorrer de a 4 en el bloque indirecto
		int offset_info = 0;													 // para escribir el char* informacion de a porciones
		int byte_bloque_indirecto = puntero_indirecto * super_bloque.BLOCK_SIZE; // byte en el archivo de bloques
		char *puntero_bloque_obtenido = NULL;
		int bloque_archivo_log = puntero_indirecto_arranque + 2;
		uint32_t *buffer = malloc(4);

		int aux;
		int bytes_desplazados_indirecto = (puntero_indirecto_arranque * 4); // cant de bytes desplazados desde el bloque indirecto
		// este primer buffer tiene que ser 2
		if (byte_del_archivo < super_bloque.BLOCK_SIZE) // ARRANCO EN EL DIRECTO
		{
			//TODO: log acceso bloque directo
			log_info(logger_fileSystem, "Acceso Bloque - Archivo <%s> - Bloque Archivo: <0> - Bloque File System <%d>", nombre_archivo, puntero_directo);
			char *puntero_en_directo = archivo_bloques + puntero_directo + (char)byte_inicio_relativo;
			aux = MIN(bytes_posibles, tamanio_a_escribir);
			memcpy(puntero_en_directo, informacion + offset_info, aux);
			offset_info += aux;
			tamanio_a_escribir -= aux;
		}
		if (byte_del_archivo > super_bloque.BLOCK_SIZE) // ARRANCO EN LOS INDIRECTOS
		{
			//TODO: log acceso bloque indirecto
			log_info(logger_fileSystem, "Acceso Bloque - Archivo <%s> - Bloque Archivo: <1> - Bloque File System <%d>", nombre_archivo, puntero_indirecto);
			puntero_bloque_obtenido = archivo_bloques + byte_bloque_indirecto + (char)bytes_desplazados_indirecto;
			memcpy(buffer, puntero_bloque_obtenido, sizeof(uint32_t));

			log_info(logger_fileSystem, "Acceso Bloque - Archivo <%s> - Bloque Archivo: <%d> - Bloque File System <%d>", nombre_archivo, bloque_archivo_log, *buffer);
			puntero_escritura = archivo_bloques + (*buffer * super_bloque.BLOCK_SIZE) + byte_inicio_relativo;

			aux = MIN(bytes_posibles, tamanio_a_escribir);
			memcpy(puntero_escritura, informacion + offset_info, aux);
			tamanio_a_escribir -= aux;
			offset_info += aux;
		}

		while (tamanio_a_escribir != 0)
		{ // Aca es donde recorro los siguientes uint32s
		//TODO: log acceso bloques de dato obtenidos desde el bloque indirecto
			bloque_archivo_log++;
			bytes_desplazados_indirecto += 4;
			puntero_bloque_obtenido = archivo_bloques + byte_bloque_indirecto + (char)bytes_desplazados_indirecto;
			memcpy(buffer, puntero_bloque_obtenido, sizeof(uint32_t));
			log_info(logger_fileSystem, "Acceso Bloque - Archivo <%s> - Bloque Archivo: <%d> - Bloque File System <%d>", nombre_archivo, bloque_archivo_log, *buffer);
			offset += 4;
			int byte_puntero_bloque_obtenido = *buffer * super_bloque.BLOCK_SIZE;
			//puntero_escritura = archivo_bloques + (char)byte_puntero_bloque_obtenido;
			puntero_escritura = archivo_bloques + (*buffer * super_bloque.BLOCK_SIZE) + byte_inicio_relativo;
			aux = MIN(super_bloque.BLOCK_SIZE, tamanio_a_escribir);
			memcpy(puntero_escritura, informacion + offset_info, aux);
			
			tamanio_a_escribir -= aux;
			offset_info += aux; // la prox iteracion escribo el resto de info que me falta escribir
		}
	}

	void f_truncate(char *nombre_archivo, int bytes)
	{
		t_config *archivo_fcb = dictionary_get(diccionario_fcbs, nombre_archivo);

		// MAPEO BITMAP
		int tamanioBitmap = ceil(super_bloque.BLOCK_COUNT / 8);
		int file_descriptor = open(config_fileSystem.path_archivo_bitmap, O_RDWR, 0664);
		void *string_bitmap = mmap(NULL, tamanioBitmap, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, 0);
		t_bitarray *bitmap = bitarray_create_with_mode(string_bitmap, tamanioBitmap, LSB_FIRST);
		// MAPEO FILESYSTEM
		//int tamanioBitmapPosta = bitarray_get_max_bit(bitmap);
		int file_descriptor_ab = open(config_fileSystem.path_archivo_bloques, O_RDWR, 0664);
		void *archivo_bloque = mmap(NULL, (super_bloque.BLOCK_SIZE * super_bloque.BLOCK_COUNT), PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor_ab, 0);

		t_list *bloques_libres = buscar_bloques_libres(bitmap); // FIX: paso por parametro el tamanioBitmap y el bitmap
		
		
		int bloques_final = ceil((double)bytes / (double)super_bloque.BLOCK_SIZE);

		int tamanio_archivo = config_get_int_value(archivo_fcb, "TAMANIO_ARCHIVO");
		int cantidad_bloques_archivo = ceil(tamanio_archivo / super_bloque.BLOCK_SIZE);

		uint32_t puntero_indirecto = config_get_int_value(archivo_fcb, "PUNTERO_INDIRECTO");
		uint32_t puntero_directo = config_get_int_value(archivo_fcb, "PUNTERO_DIRECTO");

		// mapeo archivo de bloques

		// si la cantidad necesaria es distinta que la que tengo, agrego os saco bloques
		if (bloques_final != cantidad_bloques_archivo)
		{
			if (tamanio_archivo < bytes)
			{ // AGRANDAR
				int bloques_a_agregar = bloques_final - cantidad_bloques_archivo;
				int i = 0;
				int offset = 0;
				while (bloques_a_agregar != 0)
				{
					if (tamanio_archivo == 0)
					{
						//TODO: Acceso a bloque
						int *posicion_bloque = list_get(bloques_libres, i);

						bitarray_set_bit(bitmap, *posicion_bloque); // ACA

						char *posicion_bloque_c = string_itoa(*posicion_bloque); 

						log_info(logger_fileSystem, "Acceso Bloque - Archivo <%s> - Bloque Archivo: <0> - Bloque File System <%s>", nombre_archivo, posicion_bloque_c);
						
						config_set_value(archivo_fcb, "PUNTERO_DIRECTO", posicion_bloque_c);
						config_save(archivo_fcb);
						bloques_a_agregar--;
						i++;
						tamanio_archivo += super_bloque.BLOCK_SIZE;
					}
					else
					{

						if (tamanio_archivo == super_bloque.BLOCK_SIZE)
						{
							//TODO: log acceso a bloque
							int *posicion_bloque = list_get(bloques_libres, i);
							bitarray_set_bit(bitmap, *posicion_bloque); // ACA
							char *posicion_bloque_c = string_itoa(*posicion_bloque);

							log_info(logger_fileSystem, "Acceso Bloque - Archivo <%s> - Bloque Archivo: <1> - Bloque File System <%s>", nombre_archivo, posicion_bloque_c);
							
							config_set_value(archivo_fcb, "PUNTERO_INDIRECTO", posicion_bloque_c);
							config_save(archivo_fcb);
							bitarray_set_bit(bitmap, i); // ACA

							i++;
						}
						//TODO: log accesos a bloques
						int *posicion_bloque_datos = list_get(bloques_libres, i);

						bitarray_set_bit(bitmap, *posicion_bloque_datos); // ACA
						
						int posicion_bloque_indirecto = config_get_int_value(archivo_fcb, "PUNTERO_INDIRECTO");
						int byte_bloque_indirecto = posicion_bloque_indirecto * super_bloque.BLOCK_SIZE; // byte de inicio del bloque obtenido por el bitmap
						int punteros_en_bloque = fmax(cantidad_bloques_archivo - 1, 0);
						int punteros_desplazar = punteros_en_bloque * 4;
						int byte_ultimo_puntero = (posicion_bloque_indirecto * super_bloque.BLOCK_SIZE) + punteros_desplazar; // 4 porque los punteros uint32

						uint32_t casteo = (uint32_t)(*posicion_bloque_datos);
						uint32_t *buffer = malloc(4);
						memcpy(buffer, &casteo, 4);

						char *puntero_a_escribir = archivo_bloque + (char)byte_ultimo_puntero + offset;

						memcpy(puntero_a_escribir, buffer, sizeof(uint32_t));
						msync(archivo_bloque, 4, MS_SYNC);
						offset += 4;
						i++;
						bloques_a_agregar--;
						tamanio_archivo += super_bloque.BLOCK_SIZE;
					}
				}

				char *string_tamanio_archivo = string_itoa(tamanio_archivo);
				config_set_value(archivo_fcb, "TAMANIO_ARCHIVO", string_tamanio_archivo);

				config_save(archivo_fcb);
				msync(string_bitmap, tamanioBitmap, MS_SYNC);
				//mem_hexdump(string_bitmap, tamanioBitmap);
				munmap(string_bitmap, tamanioBitmap);
			}
			else
			{ // ACHICAR
				//tamanioBitmapPosta = bitarray_get_max_bit(bitmap);

				int bloques_totales_a_eliminar = cantidad_bloques_archivo - bloques_final;
				int bloques_a_eliminar_indirecto = 0;

				if ((cantidad_bloques_archivo - 1) >= bloques_totales_a_eliminar)
				{
					bloques_a_eliminar_indirecto = bloques_totales_a_eliminar;
				}
				else
				{
					bloques_a_eliminar_indirecto = bloques_totales_a_eliminar - 1;
				}
				int borrar_desde_aca = (cantidad_bloques_archivo - 1) - bloques_a_eliminar_indirecto;
				char *direccion_indirecto = archivo_bloque + (char)(puntero_indirecto * super_bloque.BLOCK_COUNT) + (borrar_desde_aca * 4);

				int offset = 0;

				while (bloques_totales_a_eliminar != 0)
				{
					if (bloques_a_eliminar_indirecto != 0)
					{
						//TODO: log accesos a bloques
						log_info(logger_fileSystem, "Acceso Bloque - Archivo <%s> - Bloque Archivo: <1> - Bloque File System <%d>", nombre_archivo, puntero_indirecto);
						//uint32_t puntero = (uint32_t)(*direccion_indirecto);
						uint32_t *buffer = malloc(4); // me voy copiando en un buffer los punteros uint32_t del bloque de indirectos
						int posicion_bloque_indirecto = config_get_int_value(archivo_fcb, "PUNTERO_INDIRECTO");
						int punteros_en_bloque = fmax(cantidad_bloques_archivo - 1, 0);
						int punteros_desplazar = (punteros_en_bloque * 4) - 4; // si borras ese -4, rompe
						int byte_ultimo_puntero = (posicion_bloque_indirecto * super_bloque.BLOCK_SIZE) + punteros_desplazar;
						char *posicion = archivo_bloque + byte_ultimo_puntero;

						memcpy(buffer, posicion - offset, sizeof(uint32_t));

						bitarray_clean_bit(bitmap, *buffer); // ACA
						msync(string_bitmap, tamanioBitmap, MS_SYNC);
						offset += 4;

						bloques_a_eliminar_indirecto--;
						bloques_totales_a_eliminar--;
						memcpy(buffer, posicion + offset, sizeof(uint32_t));
						if (*buffer == posicion_bloque_indirecto)
						{

							bitarray_clean_bit(bitmap, posicion_bloque_indirecto); // ACA
							msync(string_bitmap, tamanioBitmap, MS_SYNC);
						}
						free(buffer);
					}
					else
					{
						log_info(logger_fileSystem, "Acceso Bloque - Archivo <%s> - Bloque Archivo: <0> - Bloque File System <%d>", nombre_archivo, puntero_directo);
						bitarray_clean_bit(bitmap, puntero_directo); // ACA
						config_set_value(archivo_fcb, "PUNTERO_DIRECTO", "0");
						msync(string_bitmap, tamanioBitmap, MS_SYNC);
						bitarray_clean_bit(bitmap, puntero_indirecto); // ACA
						log_info(logger_fileSystem, "Acceso Bloque - Archivo <%s> - Bloque Archivo: <1> - Bloque File System <%d>", nombre_archivo, puntero_indirecto);
						config_set_value(archivo_fcb, "PUNTERO_INDIRECTO", "0");
						msync(string_bitmap, tamanioBitmap, MS_SYNC);
						bloques_totales_a_eliminar--;
					}
				}

				char *string_tamanio_archivo = string_itoa(bytes);

				config_set_value(archivo_fcb, "TAMANIO_ARCHIVO", string_tamanio_archivo);
				config_save(archivo_fcb);
				msync(string_bitmap, tamanioBitmap, MS_SYNC);
				//tamanioBitmapPosta = bitarray_get_max_bit(bitmap);

				munmap(string_bitmap, tamanioBitmap);
			}
		}
		list_destroy(bloques_libres);
	}

	t_list *buscar_bloques_libres(t_bitarray * bitmap)
	{
		t_list *bloques_libres = list_create();
		int *numero_bloque_libre;
		for (int i = 0; i < super_bloque.BLOCK_COUNT; i++)
		{
			if (bitarray_test_bit(bitmap, i) == 0)
			{
				numero_bloque_libre = malloc(sizeof(int));
				*numero_bloque_libre = i;
				list_add(bloques_libres, numero_bloque_libre);
				log_info(logger_fileSystem, "Acceso a Bitmap - Bloque: <%d> - Estado: <0>", i);//TODO: descomentar para la entrega
			}else {
				log_info(logger_fileSystem, "Acceso a Bitmap - Bloque: <%d> - Estado: <1>", i);//TODO: descomentar para la entrega
			}
			
		}
		// free(numero_bloque_libre);
		return bloques_libres; // lista con la pos de cada bit libre
	}

	void crearBloques()
	{
		// O_CREAT: crear si no existe. O_RDWR: abre el archivo tanto para lectura como para escritura.
		// 0664 codigo de permisos para que tanto el owner como el grupo puedan leer y/o escribir
		if(access(config_fileSystem.path_archivo_bloques, F_OK) != -1)
		{
			return;
		}
		int file_descriptor = open(config_fileSystem.path_archivo_bloques, O_CREAT | O_RDWR, 0664); // revisar si referencia bien al path de config

		int tamanio_bloques = super_bloque.BLOCK_COUNT * super_bloque.BLOCK_SIZE;
		ftruncate(file_descriptor, tamanio_bloques);

		// mapeamos el archivo de bloques a memoria
		// el primer parametro es NULL para que el SO elija unaS direccion disponible adecuada
		// PROT_X son los permisos, el ultimo parametro es el offset (en este caso mapeamos desde el principio del archivo/entero)
		void *bloques = mmap(NULL, tamanio_bloques, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, 0);

		void *array_bloques = malloc(tamanio_bloques);
		memcpy(bloques, array_bloques, tamanio_bloques);
		msync(bloques, tamanio_bloques, MS_SYNC);
		//mem_hexdump(bloques, tamanio_bloques);
		munmap(bloques, tamanio_bloques);
		free(array_bloques);
		close(file_descriptor);
	}

	void levantar_bitmap()
	{
		int tamanioBitmap = ceil((double)super_bloque.BLOCK_COUNT / 8);

		if(access(config_fileSystem.path_archivo_bitmap, F_OK) != -1)
		{
			return;
		}

		int file_descriptor = open(config_fileSystem.path_archivo_bitmap, O_CREAT | O_RDWR, 0664);
		if (file_descriptor == -1)
		{
			printf("Error al abrir archivo de bitmap\n");
			
			return;
		}
		ftruncate(file_descriptor, tamanioBitmap);

		void *string_bitmap = mmap(NULL, tamanioBitmap, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, 0);
		// MAPEO A memoria

		if (string_bitmap == MAP_FAILED)
		{
			printf("Error al asignar memoria para el bitmap\n");
			close(file_descriptor);
			return;
		}

		t_bitarray *bitmap = bitarray_create_with_mode(string_bitmap, tamanioBitmap, LSB_FIRST);
		if (bitmap == NULL)
		{
			printf("Error al crear el bitarray\n");
			munmap(string_bitmap, tamanioBitmap);
			close(file_descriptor);
			return;
		}
		int tamanioBitmapPosta = bitarray_get_max_bit(bitmap);
		//log_info(logger_fileSystem, "Tamanio bitmap: %d", tamanioBitmapPosta);

		for (int i = 0; i < tamanioBitmap; i++)
		{
			bitarray_clean_bit(bitmap, i);
		}

		msync(string_bitmap, tamanioBitmap, MS_SYNC);

		close(file_descriptor);
	}

	void levantar_superbloque()
	{

		int file_descriptor = open(config_fileSystem.path_archivo_superbloques, O_CREAT | O_RDWR, 0664);

		// con cada valor del config relleno t_superbloque
		config_superBloque = iniciar_config(config_fileSystem.path_archivo_superbloques);
		super_bloque.BLOCK_SIZE = config_get_int_value(config_superBloque, "BLOCK_SIZE");
		super_bloque.BLOCK_COUNT = config_get_int_value(config_superBloque, "BLOCK_COUNT");
		close(file_descriptor);
	}
	void inicializar_procesador_peticiones()
	{
		pthread_create(&manejo_peticiones, NULL, (void *)procesador_peticiones, NULL);
		//log_info(logger_fileSystem, "Me conecté con filesystem :)");
		pthread_join(manejo_peticiones, NULL);
	}

	void inicializar_fileSystem(char** argv)
	{

		logger_fileSystem = iniciar_logger(PATH_LOG_FILESYSTEM, "FILESYSTEM");
		char* path_archivo = malloc((strlen(argv[1]) + 1) * sizeof(char));
    	strcpy(path_archivo, argv[1]);
		config4 = iniciar_config(path_archivo);

		config_fileSystem.ip_fileSystem = config_get_string_value(config4, "IP_FILESYSTEM");
		config_fileSystem.puerto_fileSystem = config_get_string_value(config4, "PUERTO_ESCUCHA");
		config_fileSystem.ip_memoria = config_get_string_value(config4, "IP_MEMORIA");
		config_fileSystem.puerto_memoria = config_get_string_value(config4, "PUERTO_MEMORIA");
		config_fileSystem.path_fcb = config_get_string_value(config4, "PATH_FCB");
		config_fileSystem.path_archivo_bloques = config_get_string_value(config4, "PATH_BLOQUES");
		config_fileSystem.path_archivo_bitmap = config_get_string_value(config4, "PATH_BITMAP");
		config_fileSystem.path_archivo_superbloques = config_get_string_value(config4, "PATH_SUPERBLOQUE");

		levantar_directorio_fcbs();
		//pthread_mutex_init(&mutex, NULL);
	}

	void levantar_directorio_fcbs() {
		//acceder al primer fcb
		//hacer config_get_value del nombre y el puntero al config
		//hacer dictionary put de estos dos valores
		diccionario_fcbs = dictionary_create();
		//hacer opendir del directorio fcbs
		DIR* dir;
		struct dirent* entry;
		dir = opendir("/home/utnso/tp-2023-1c-Grupo-/fileSystem/fcbs"); // dir = opendir("./fcbs");
		//por cada archivo fcb del directorio:
		while((entry = readdir(dir)) != NULL) {
			if(entry->d_type == DT_REG) {					
			char *file_path = string_from_format("%s/%s", "/home/utnso/tp-2023-1c-Grupo-/fileSystem/fcbs" , entry->d_name);
			t_config* config = config_create(file_path);
			free(file_path);

			char* nombre_archivo = config_get_string_value(config, "NOMBRE_ARCHIVO");
			dictionary_put(diccionario_fcbs, nombre_archivo, config);
			}
		}
		closedir(dir);
	}

	void crear_fcb(char *nombre_archivo)
	{

	 	char* ruta = string_from_format("%s/%s", config_fileSystem.path_fcb, nombre_archivo);
		//log_warning(logger_fileSystem, "ruta: %s", ruta);
		int file_descriptor = open(ruta, O_CREAT | O_RDWR, 0664);
		t_config *config_fcb = config_create(ruta);
		config_set_value(config_fcb, "NOMBRE_ARCHIVO", nombre_archivo);
		config_set_value(config_fcb, "TAMANIO_ARCHIVO", "0");
		config_set_value(config_fcb, "PUNTERO_DIRECTO", "0");
		config_set_value(config_fcb, "PUNTERO_INDIRECTO", "0");
		config_save(config_fcb);

		dictionary_put(diccionario_fcbs, nombre_archivo, config_fcb);
		free(ruta);
	}
