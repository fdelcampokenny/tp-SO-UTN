#include "memoria.h"
#include <stdio.h>
#include <stdlib.h>
#define VERDE_F "\x1b[32m"

int main(int argc, char **argv) {
    id_global = 0;
    inicializar_memoria(argv);
    inicializar_segmentacion();

    servidor_memoria = iniciar_servidor(config_memoria.ip_memoria, config_memoria.puerto_memoria, logger_memoria);
    conexion_cpu = esperar_cliente(servidor_memoria, logger_memoria);
    conexion_fileSystem = esperar_cliente(servidor_memoria, logger_memoria);
    conexion_kernel = esperar_cliente(servidor_memoria, logger_memoria);

    inicializar_procesador_peticiones_kernel();
    inicializar_procesador_peticiones_cpu();
    inicializar_procesador_peticiones_fileSystem();
  
    pthread_join(hilo_procesador_kernel, NULL);
    pthread_join(hilo_procesador_cpu, NULL);
    pthread_join(hilo_procesador_fileSystem, NULL);
    imprimir_bitmap();
  
}

void inicializar_procesador_peticiones_kernel() {
	pthread_create(&hilo_procesador_kernel, NULL, (void*)procesador_peticiones_kernel, NULL);
    log_info(logger_memoria, "Inicio procesador peticiones kernel");
}    


void inicializar_procesador_peticiones_cpu() {
	pthread_create(&hilo_procesador_cpu, NULL, (void*)procesador_peticiones_cpu, NULL);
    log_info(logger_memoria, "Inicio procesador peticiones cpu");
    
}

void inicializar_procesador_peticiones_fileSystem() {
	pthread_create(&hilo_procesador_fileSystem, NULL, (void*)procesador_peticiones_fileSystem, NULL);
    log_info(logger_memoria, "Inicio procesador peticiones fileSystem");
    
}

void inicializar_memoria(char** argv){

    logger_memoria = iniciar_logger(PATH_LOG_MEMORIA, "MEMORIA");
    char* path_archivo = malloc((strlen(argv[1]) + 1) * sizeof(char));
    strcpy(path_archivo, argv[1]);
    config5 = iniciar_config(path_archivo);
    config_memoria.ip_memoria           = config_get_string_value(config5, "IP_MEMORIA");
    config_memoria.puerto_memoria       = config_get_string_value(config5, "PUERTO_ESCUCHA");
    config_memoria.tam_memoria          = config_get_int_value(config5, "TAM_MEMORIA");
    config_memoria.tam_segment_cero     = config_get_int_value(config5, "TAM_SEGMENTO_0");
    config_memoria.cantidad_segment     = config_get_int_value(config5, "CANT_SEGMENTOS");
    config_memoria.retardo_memoria      = config_get_double_value(config5, "RETARDO_MEMORIA");
    config_memoria.retardo_compactacion = config_get_double_value(config5, "RETARDO_COMPACTACION");
    config_memoria.algoritmo_asignacion = config_get_string_value(config5, "ALGORITMO_ASIGNACION");
}

void imprimir_bitmap() {
    for(int i = 0; i < config_memoria.tam_memoria; i++) {
       if(bitarray_test_bit(bitmap_memoria, i) == 0){
        printf("0");
       }else {
        printf("1");
       }
    }
    printf("\n");
}



void inicializar_segmentacion() {
    memoria = malloc(config_memoria.tam_memoria); //espacio contiguo (ram)
    tabla_segmentos_global = list_create(); //lista con todas las tablas de segmentos de todos los procesos en memoria
                             
    //bitmap segmentos
    char* datos = crear_bitmap_memoria(config_memoria.tam_memoria); 
    int tamanio = bitsToBytes(config_memoria.tam_memoria); //se hace este calculo para que en el bitmap cada bit represente un byte
    bitmap_memoria = bitarray_create_with_mode(datos, tamanio, MSB_FIRST); //el segundo parametro que recibe bitarray_create_with_mode
    inicializar_segmento_0();
}  

void inicializar_segmento_0() {
    segmento_0 = malloc(sizeof(t_segmento));
    segmento_0->id_segmento = 0;
    segmento_0->direccion_base = 0;
    segmento_0->tamanio = config_memoria.tam_segment_cero;
    actualizar_bitmap_a_uno(bitmap_memoria, segmento_0);
}//es cantidad de bits, si le pasaramos tam.memoria haria un
//bitmap de cada bit por byte de la memoria (una bocha)




int bitsToBytes(int bits){
	int bytes;
	if(bits < 8)
		bytes = 1; 
	else
	{
		double c = (double)bits;
		bytes = ceil(c/8.0);
	}
	
	return bytes;
}

char* crear_bitmap_memoria(int bytes) {
    char* aux;
    aux = malloc(bytes);
    memset(aux, 0, bytes); //Seteo todos los bits en 0
    return aux;
}



void procesador_peticiones_kernel() { 
                                      
    while(1){

        t_paquete* paquete_kernel = recibir_paquete(conexion_kernel); 
        t_pcb* pcb = deserializar_pcb(paquete_kernel->buffer);

                
        switch(paquete_kernel->opcode){
            
            
            case INIT_PROCESO:{
            t_tabla_segmentos* tabla = inicializar_tabla_segmentos(pcb->pid);

            t_paquete* paquete = serializar_tabla_segmentos(tabla);
            enviar_paquete(conexion_kernel, TABLA_INICIALIZADA, paquete);
            log_info(logger_memoria, "Creacion del proceso PID: <%d>", pcb->pid);
            break;
            }
            
            
            case FIN_PROCESO: {
            log_info(logger_memoria, "Eliminacion del proceso PID: <%d>", pcb->pid);
            limpiar_tabla(pcb->pid);
            break;
            }

            case CREATE_SEGMENT:{//TODO: controlar que no nos pasemos de la cant maxima de segmentos dada en config                           
            
            int32_t base = create_segment(pcb->id_segmento, pcb->tamanio_segmento, pcb->pid);
            
            //* t_paquete* paquete_k =mensaje_only(base);
            
            
            if(base ==-1) {
                log_info(logger_memoria, "Pronta eliminacion del proceso PID: <%d>", pcb->pid);

                send(conexion_kernel, &base, sizeof(int32_t),0);               
                // * enviar_paquete(conexion_kernel, CREATE_SEGMENT, paquete_k);
                
                break;
            }else if(base == -2){
                log_info(logger_memoria, "PID: <%d> - Te voy a compactar", pcb->pid);
                // * enviar_paquete(conexion_kernel, CREATE_SEGMENT, paquete_k);
                send(conexion_kernel, &base, sizeof(int32_t),0);
                break;
            }else{
                log_info(logger_memoria, "PID: <%d> - Crear Segmento: <%d> - Base: <%d> - Tamaño: <%d>", pcb->pid, pcb->id_segmento, base, pcb->tamanio_segmento);  
                // * enviar_paquete(conexion_kernel, CREATE_SEGMENT, paquete_k);            
                send(conexion_kernel, &base, sizeof(int32_t),0);
                break;
            }
            break;
            }
            
      
            case DELETE_SEGMENT:{
            t_list* tabla_actualizada = delete_segment(pcb->id_segmento, pcb->pid);
            t_tabla_segmentos* tabla = malloc(sizeof(t_tabla_segmentos));
            tabla->pid = pcb->pid;
            tabla->segmentos = tabla_actualizada;

            t_paquete* paquete_tabla = serializar_tabla_segmentos(tabla);

            enviar_paquete(conexion_kernel, TABLA_ACTUALIZADA, paquete_tabla);
            break;
            }
            
            
            case COMPACTAR:{
            log_info(logger_memoria, "Solicitud de Compactacion");    
            compactar_CRACKED(); 
            log_info(logger_memoria, "termine de compactar");
            sleep(config_memoria.retardo_compactacion/1000); // TODO cambiar a lo del archivo

            t_paquete* paquete = serializar_tabla_global_segmentos(tabla_segmentos_global);
		    enviar_paquete(conexion_kernel, ACTUALIZAR_TABLA_SEG_GLOBAL, paquete); 
            log_info(logger_memoria, "envie la tabla");
            resultado_compactacion(); 
            //ejecuta los logs obligatorios de compactacion
            //char* buffer_compactacion = malloc(9);
            //memcpy(buffer_compactacion, get_value(128, 8), 8);
            //memcpy(buffer_compactacion + 8, "\0", 1);
            //log_warning(logger_memoria, "valor: %s", buffer_compactacion);
            break;
            }
            
        }
        eliminar_paquete(paquete_kernel);
        //free(pcb);
    }
}

void procesador_peticiones_cpu()
{
    t_escritura_memoria *escritura = malloc(sizeof(t_escritura_memoria));
    t_lectura_memoria *lectura = malloc(sizeof(t_lectura_memoria));

    while (1)
    {
        t_paquete *paquete_cpu = recibir_paquete(conexion_cpu);
        if (paquete_cpu->opcode == MOV_OUT)
        {
            escritura = deserializar_escritura_memoria(paquete_cpu->buffer);
        }
        else if (paquete_cpu->opcode == MOV_IN)
        {
            lectura = deserializar_lectura_memoria(paquete_cpu->buffer);
        }
        switch (paquete_cpu->opcode)
        {

        case MOV_IN:
        { // ? LEO de memoria y se lo envio a CPU para que escriba en registro
            
            //double direccion_fisica = lectura->direccion_fisica;
            uint32_t tamanio = lectura->tamanio_registro;


            char *valor = malloc(lectura->tamanio_registro);
            char *log_valor = malloc(lectura->tamanio_registro + 1);
            memcpy(valor, get_value(lectura->direccion_fisica, lectura->tamanio_registro), lectura->tamanio_registro);
            memcpy(log_valor, valor, tamanio);
            memcpy(log_valor + tamanio, "\0", 1);
            log_info(logger_memoria, "PID: <%d> - Acción: <LEER> - Dirección física: <%f> - Tamaño: <%d> - Origen: <CPU>", lectura->pid, lectura->direccion_fisica, lectura->tamanio_registro);
            log_warning(logger_memoria, "El contenido leido de la Direccion Fisica es : %s", log_valor);
            sleep(config_memoria.retardo_memoria / 1000);
            send(conexion_cpu, valor, tamanio, 0);
            free(log_valor);
            free(valor);
            break;
            
        }

        case MOV_OUT:
        { // ? Leo de registro y lo ESCRIBO en memoria
            //log_info(logger_memoria, "Dato a escribir: %s", escritura->valor_a_escribir);
            //log_info(logger_memoria, "Tamanio dato a escribir %d", escritura->tamanio_a_escribir);
            mov_out(escritura->valor_a_escribir, escritura->direccion_fisica, escritura->tamanio_a_escribir);
            log_info(logger_memoria," PID: <%d> - Acción: <ESCRIBIR> - Dirección física: <%f> - Tamaño: <%d> - Origen: <CPU>", escritura->pid, escritura->direccion_fisica, escritura->tamanio_a_escribir);
            break;
    }

    
    //free(lectura);
    //free(escritura);
        }
    }
}

void procesador_peticiones_fileSystem() { 
    //no hay que hacer malloc, declaras un t_paquete y lo igualas a recibir paquete, lo mismo con la estructura que deserializas
    t_lectura_memoria* lectura= malloc(sizeof(t_lectura_memoria));
    t_escritura_memoria* escritura= malloc(sizeof(t_escritura_memoria));
    while(1){
        t_paquete* paquete_fileSystem = recibir_paquete(conexion_fileSystem);    
        //sem_post(&sem_peticiones_cpu);
    if(paquete_fileSystem->opcode== F_READ){ //fread lee un archivo y ESCRIBE en memoria
     escritura = deserializar_escritura_memoria(paquete_fileSystem->buffer); //se volvia a definir escritura
    }
    else if( paquete_fileSystem->opcode== F_WRITE){//fwrite LEE de memoria y escribe en archivo
     lectura = deserializar_lectura_memoria(paquete_fileSystem->buffer); //se volvia a definir lectura
    }
        switch(paquete_fileSystem->opcode){
            case F_WRITE:{
            
            double direccion_fisica= lectura->direccion_fisica;
            uint32_t tamanio= lectura->tamanio_registro;
            log_info(logger_memoria, "direccion fisica donde escribir: %f", escritura->direccion_fisica);

            
            char* contenido = malloc(tamanio);
            memcpy(contenido,get_value(direccion_fisica,tamanio), tamanio);
            char* string_buffer = malloc(tamanio + 1);
            memcpy(string_buffer, contenido, tamanio);
            memcpy(string_buffer + tamanio, "\0", 1);
            log_warning(logger_memoria, "Obtuve el contenido: %s", string_buffer);
            free(string_buffer);
            sleep(config_memoria.retardo_memoria/1000);
            
            paquete_fileSystem = serializar_escritura_memoria(contenido,0,tamanio,0);
            log_info(logger_memoria, "serialice paquete para mandar a fs");

            enviar_paquete(conexion_fileSystem, OK, paquete_fileSystem);
            free(contenido);// ! WARD descomentar post test
            
            log_info(logger_memoria, "PID: <%d> - Acción: <LEER> - Dirección física: <%f> - Tamaño: <%d> - Origen: <FS>", lectura->pid, direccion_fisica, tamanio);
            break;
            }

            case F_READ:{
            char* info = escritura->valor_a_escribir;
            double direccion_fisica= escritura->direccion_fisica;
            uint32_t tamanio_registro = escritura->tamanio_a_escribir;
	    char* string_buff = malloc(tamanio_registro + 1);
		memcpy(string_buff, info, tamanio_registro);
		memcpy(string_buff + tamanio_regsitro, "\0", 1);
		log_warning(logger_memoria, "valor a escribir: %s", string_buff);	
		free(string_buff);
            mov_out(info, direccion_fisica, tamanio_registro);
            log_info(logger_memoria, "PID: <%d> - Acción: <ESCRIBIR> - Dirección física: <%f> - Tamaño: <%d> - Origen: <FS>", escritura->pid, direccion_fisica, tamanio_registro);
            sleep(config_memoria.retardo_memoria/1000);
            enviar_paquete(conexion_fileSystem, OK, paquete_fileSystem);
            
            break;
            }
            
        }        
    }

    
    //free(paquete_cpu);
    //free(lectura);
    //free(escritura);
}


int32_t create_segment(int id, int tamanio, int pid) {

    t_segmento* nuevo_segmento = malloc(sizeof(t_segmento));
    t_segmento* hueco = buscar_segmento(tamanio);
    

    //con este if contemplo out of memory
    if(hueco->direccion_base > 0){
        nuevo_segmento->id_segmento = id;                                
        nuevo_segmento->direccion_base = hueco->direccion_base;      
        nuevo_segmento->tamanio = tamanio;  
        t_tabla_segmentos* tabla = malloc(sizeof(t_tabla_segmentos));
        tabla = buscar_tabla(pid);
        agregar_ordenado_por_id(tabla->segmentos, nuevo_segmento);
        actualizar_bitmap_a_uno(bitmap_memoria, nuevo_segmento);
        //t_segmento* segmento0 = list_get(tabla->segmentos,0);
        //t_segmento* segmento1 = list_get(tabla->segmentos, 1);
        //free(hueco);
    }
    
    return hueco->direccion_base;
}

void agregar_ordenado_por_id(t_list* segmentos, t_segmento* nuevo_segmento){

    bool id_menor(void* un_segmento, void* otro_segmento) {
        return (((t_segmento*)un_segmento)->id_segmento) < (((t_segmento*)otro_segmento)->id_segmento);
    }

    list_add_sorted(segmentos, nuevo_segmento, id_menor);
}

t_list* delete_segment(int id_segmento, int pid){
    //obtener tabla, encontrar id por id_segmento del parametro y eliminarlo
    t_tabla_segmentos* tabla = malloc(sizeof(t_tabla_segmentos));
    tabla = buscar_tabla(pid);
    t_segmento* segmento = malloc(sizeof(t_segmento));
    segmento = buscar_segmento_por_id(tabla->segmentos, id_segmento);
    log_info(logger_memoria, "PID: <%d> - Eliminar Segmento: <%d> - Base: <%d> - TAMAÑO: <%d>", pid, id_segmento, segmento->direccion_base, segmento->tamanio); 
    limpiar_segmentos_en_memoria(tabla->segmentos, segmento);
    //free(tabla);
    return tabla->segmentos;
    }

t_tabla_segmentos* buscar_tabla(int pid){

    bool tiene_mismo_id(void* una_tabla) {
        return (((t_tabla_segmentos*)una_tabla)->pid == pid);
    }

    t_tabla_segmentos* tabla = list_find(tabla_segmentos_global, tiene_mismo_id);
    if(tabla == NULL) {
        log_error(logger_memoria, "No encontre la tabla con el pid: %d", pid);
        return NULL;
    }
    return tabla;
}

t_segmento* buscar_segmento_por_id(t_list* segmentos, int id_segmento) {

    bool tiene_mismo_id_seg(void* un_segmento) {
        return (((t_segmento*)un_segmento)->id_segmento == id_segmento);
    }

    t_segmento* un_segmento = list_find(segmentos, tiene_mismo_id_seg);
    if(un_segmento == NULL) {
        log_error(logger_memoria, "No encontre el segmento con id_segmento: %d", id_segmento);
        return NULL;
    }
    return un_segmento;
}


t_list* limpiar_segmentos_en_memoria(t_list* segmentos, t_segmento* segmento) {
    if(segmento == NULL) {
        printf("Error, no encontre el segmento \n");
        return 0;
    }else {
    actualizar_bitmap_a_cero(bitmap_memoria, segmento);
    limpiar_tabla_segmentos(segmentos, segmento);
    return segmentos;
    }
}

 void limpiar_tabla_segmentos(t_list* segmentos, t_segmento* segmento) {
        list_remove_element(segmentos, segmento);
 }

char* get_value(double direccion_fisica, uint32_t tamanio_registro) {
    char* valor = malloc(tamanio_registro);
    char* puntero_memoria = memoria + (int)direccion_fisica; //me paro en memoria donde se encuentre lo que quiero leer
    memcpy(valor, puntero_memoria, tamanio_registro);
    return valor;
}

//MOV_OUT: Escribe un valor, dada una direccion fisica y su tamanio 
void mov_out(char* valor, double direccion_fisica, uint32_t tamanio_a_escribir) { 
    //char* puntero_memoria = memoria + (char)direccion_fisica;
    char* puntero_memoria = memoria + (size_t)direccion_fisica;   
    memcpy(puntero_memoria, valor, tamanio_a_escribir); 

}






void write_value(void* ram_auxiliar, char* contenido, double direccion_fisica, uint32_t tamanio_a_escribir)
{
    char* posicion =  ram_auxiliar + (char)direccion_fisica;
    memcpy(posicion,contenido, tamanio_a_escribir);
}


void write_value_segmento(void* ram_auxiliar, char* contenido, t_segmento* segmento_final) // COMENTADA
{   
    char* posicion =  ram_auxiliar + (size_t)segmento_final->direccion_base;
    memcpy(posicion,contenido, segmento_final->tamanio);
}


void compactar_CRACKED(){ //TODO: Probar con varios procesos, en lugar de 1.
    
    void* ram_auxiliar= malloc(config_memoria.tam_memoria); //2048 x ejemplo 
    int tamanio_tabla_global = list_size(tabla_segmentos_global);
    resetear_bitmap();
    inicializar_segmento_0();      


    for(int i = 0; i < tamanio_tabla_global; i++) { // RECORRO TABLA DE PROCESOS 
            memcpy(ram_auxiliar, memoria, config_memoria.tam_segment_cero);
            t_tabla_segmentos* tabla_x_proceso = list_get(tabla_segmentos_global, i); // TABLA DE SEGMENTOS PROCESO 1
            t_list* tabla_segmentos = tabla_x_proceso->segmentos;
        
            for(int a=1; a < list_size(tabla_segmentos); a++){ // Arrancamos en A=1 saltear segemento 0
            t_segmento* segmento = list_get(tabla_segmentos, a);
            char* contenido = get_value((int)segmento->direccion_base, segmento->tamanio);
                        
            t_segmento* segmento_ram_auxiliar = buscar_segmento((int)segmento->tamanio);
            segmento_ram_auxiliar->tamanio = segmento->tamanio;
            write_value_segmento(ram_auxiliar,contenido, segmento_ram_auxiliar);
            //free(contenido);
            
            segmento->direccion_base = segmento_ram_auxiliar->direccion_base;
            actualizar_bitmap_a_uno(bitmap_memoria, segmento);      
                }
        }
    log_info(logger_memoria, "Bitmap final:");
    imprimir_bitmap();
    memoria = ram_auxiliar;
    //write_value(memoria, get_value(0, config_memoria.tam_segment_cero), 0, config_memoria.tam_segment_cero);
    //free(ram_auxiliar);
}

void liberar_tabla(int pid) {
    t_tabla_segmentos* tabla_segmentos = buscar_tabla(pid);
    t_list* lista_segmentos = tabla_segmentos->segmentos;
    int i = 1;
    while(list_size(lista_segmentos) > 1) { //seteo el bitmap de cada segmento a 0
    //t_segmento* segmento = list_get(lista_segmentos, i); //no se usa (posiblemente eliminar)
    delete_segment(i, pid);
    i++;
    }
    //free(tabla_segmentos);
  }

void  limpiar_tabla(int pid) {
    t_tabla_segmentos* tabla_a_eliminar = buscar_tabla(pid);
    t_list* segmentos = tabla_a_eliminar->segmentos;
    
    list_remove_element(segmentos, segmento_0);
    
    t_list_iterator* iterador = list_iterator_create(tabla_a_eliminar->segmentos);

    while(list_iterator_has_next(iterador)){ 
        t_segmento* aux = (t_segmento*)list_iterator_next(iterador);
        actualizar_bitmap_a_cero(bitmap_memoria, aux);
        
    }
    list_iterator_destroy(iterador);

    list_remove_element(tabla_segmentos_global, tabla_a_eliminar);

}


      

t_list* copiar_contenido_segmentos(t_list* segmentos_ocupados) {
    t_list* contenidos_pegados = list_create();
    t_list* lista_aux = list_create();

   int cant_segmentos_ocupados = list_size(segmentos_ocupados);

   //recorro todas las tablas de segmentos
   for(int i = 0; i< cant_segmentos_ocupados; i++) {
    //obtengo una tabla
    t_list* tabla_segmentos = list_get(segmentos_ocupados, i);

    //ejecuto copiar_segmento en una tabla
    lista_aux = copiar_segmento(tabla_segmentos);

    //agrego esa lista de char* a la tabla a retornar
    list_add_all(contenidos_pegados, lista_aux);

    //limpio la lista auxiliar para la prox iteracion
    list_clean(lista_aux);
   }                    
   return contenidos_pegados;                                     
}

//En realidad tengo que copiar el char* que tiene en memoria CADA segmento de cada taba de segmentos de cada proceso (tabla global)
//tengo que acceder a cada segmento
//con cada segmento, hacer malloc del tamaño de su char* (segmento->tamanio)
//hacer memcpy de ese char*


t_list* copiar_segmento(t_list* segmentos) {
    int cantidad_segmentos = list_size(segmentos);

    //creo lista de contenidos de una tabla char*|char*|char*
    t_list* lista_contenidos_tabla = list_create();

    for(int i = 0; i< cantidad_segmentos; i++) {
    t_segmento* segmento = list_get(segmentos, i);
    void* contenido = malloc(segmento->tamanio);
    memcpy(contenido, memoria + segmento->direccion_base, segmento->tamanio);
    list_add(lista_contenidos_tabla, contenido);
    }
   //ahora lista_contenidos_tabla tiene 
   //char*|char*|char* de cada segmento de dicha tabla
    
    return lista_contenidos_tabla;
}



/*
void resetear_bitmap(t_bitarray* bitmap, int base, int tamanio) {
    for(int i=0; i< tamanio-base; i++) { 
        bitarray_clean_bit(bitmap_memoria, i);
    }
}
*/

void resetear_bitmap() {
    for(int i = config_memoria.tam_segment_cero; i <= config_memoria.tam_memoria; i++) {
        bitarray_clean_bit(bitmap_memoria, i);
    }
}

void actualizar_compactacion(t_list* seg_no_compactados, t_list* seg_compactados) {
    for(int i=0; i<list_size(seg_no_compactados); i++) {
        actualizar_cada_segmento(list_get(seg_no_compactados, i), list_get(seg_compactados, i));
    }
}

void actualizar_cada_segmento(t_segmento* segmento_viejo, t_segmento* segmento_nuevo) {
    segmento_viejo->id_segmento = segmento_nuevo->id_segmento;
    segmento_viejo->direccion_base = segmento_nuevo->direccion_base;
    segmento_viejo->tamanio = segmento_nuevo->tamanio;
    
}



t_segmento* buscar_segmento(int tamanio) {
    t_list* huecos;
    t_segmento* hueco = malloc(sizeof(t_segmento));
    t_list* huecos_candidatos;
    huecos = buscar_huecos(); //TODO: si no hay memoria dispo, retornar error "OUT OF MEMORY" 
    //t_segmento* hueco1 = list_get(huecos, 0);
    //log_info(logger_memoria, "base hueco 1: %d. tamanio hueco 1: %d, id: %d", hueco1->direccion_base, hueco1->tamanio, hueco1->id_segmento);
    int mismo_id(void* un_segmento) {
        return (((t_segmento*)un_segmento)->id_segmento == hueco->id_segmento);
    }
    
    huecos_candidatos = pueden_guardar(tamanio, huecos); //lista de huecos a quienes le entran lo que quiero guardar
    if(list_is_empty(huecos_candidatos)) { 
        if(sirve_compactar(huecos, tamanio)) {
            hueco->direccion_base = (int32_t)(-2);
        }else {
             hueco->direccion_base = (int32_t)(-1);
        }
    }else if(list_size(huecos_candidatos) == 1) {
        hueco = list_get(huecos_candidatos, 0); //obtengo el unico segmento candidato disponible
        list_remove_by_condition(huecos, (void*)mismo_id);
    }else{
        hueco = elegir_segun_criterio(huecos_candidatos, tamanio);
        list_remove_by_condition(huecos, (void*)mismo_id);
    }


    list_destroy(huecos);
    list_destroy(huecos_candidatos);
    //El hueco ha sido asignado para guardar algo, ya no esta libre (ya no es hueco). Lo elimino de la lista de huecos
    

    return hueco;
}

bool sirve_compactar(t_list* huecos_candidatos, int tamanio) {
    int tamanio_total = 0;
    t_list_iterator* iterador_huecos = list_iterator_create(huecos_candidatos);
    while(list_iterator_has_next(iterador_huecos)) {
        t_segmento* hueco = list_iterator_next(iterador_huecos);
        //log_warning(logger_memoria, "base hueco: %d - tamanio hueco: %d",hueco->direccion_base, hueco->tamanio);
        tamanio_total += hueco->tamanio;
    }
    return tamanio_total >= tamanio;
}

t_list* buscar_huecos() {
    t_list* segmentos_huecos = list_create();
    int base = 0;
    while(base < config_memoria.tam_memoria) {
        t_segmento* un_segmento = buscar_hueco(&base);   
        list_add(segmentos_huecos, un_segmento);
    }
    return segmentos_huecos;
}

t_segmento* buscar_hueco(int* base) {
    t_segmento* segmento_hueco = malloc(sizeof(t_segmento));          
    int tamanio = 0;
    if(bitarray_test_bit(bitmap_memoria, (*base)) == 1) {
        int offset = contar_bytes_ocupados_desde(bitmap_memoria, (*base));  
        (*base) += offset;
        //log_info(logger_memoria, "Cantidad de 1s: %d", offset);
    }
    
    //log_info(logger_memoria, "base: %d", (*base));
    //ahora base esta en el primer bit de seg libre
    tamanio = contar_bytes_libres_desde(bitmap_memoria, (*base));
    //log_info(logger_memoria, "tamanio: %d", tamanio);
    segmento_hueco->id_segmento = generar_id_segmento(); 
    segmento_hueco->direccion_base = (*base);             
    segmento_hueco->tamanio = tamanio; 

    //muevo la base un lugar para la proxima busqueda de huecos
    (*base) += tamanio;
    
    
    return segmento_hueco;
}

int contar_bytes_ocupados_desde(t_bitarray* bitmap, int a) {
    int contador = 0;

    while((bitarray_test_bit(bitmap, a)== 1) && (a < config_memoria.tam_memoria)) { 
        contador++;
        a++;
    }                      //11111\0\1011
    return contador;
}
// 1111000001
// 0123456789
// ----012345

int contar_bytes_libres_desde(t_bitarray* bitmap, int a) {
    int contador = 0;

    while((bitarray_test_bit(bitmap, a)== 0) && (a < config_memoria.tam_memoria)) {     
        contador++;
        a++;
    }
    return contador;
}

t_list* pueden_guardar(int tamanio_a_guardar, t_list* huecos){
    
    t_list* aux;
    
    bool pueden_guardar_en_hueco(void* hueco){ 
        return (((t_segmento*)hueco)->tamanio >= tamanio_a_guardar);
    }
    aux = list_filter(huecos, pueden_guardar_en_hueco);

    return aux; //retorna lista de huecos en los que entra el contenido a guardar
}

t_segmento* guardar_contenido(void* contenido, int tamanio) { //MOV_OUT??
    t_segmento* un_segmento = malloc(sizeof(t_segmento));
    //busco hueco TODO:cambiar nombre a buscar_hueco
    t_segmento* aux;
    aux = buscar_segmento(tamanio);
    
    if(aux->tamanio != tamanio) {
        log_error(logger_memoria, "encontre un hueco con tamanio erroneo");
    }
    guardar_en_memoria(contenido, aux, tamanio);

    un_segmento->id_segmento = aux->id_segmento;
    un_segmento->direccion_base = aux->direccion_base;
    un_segmento->tamanio = aux->tamanio;

    return un_segmento;
}

void guardar_en_memoria(void* contenido, t_segmento* segmento, int tamanio) {
    actualizar_bitmap_a_uno(bitmap_memoria, segmento);               
    ocupar_memoria(contenido, segmento->direccion_base, tamanio);
}

void actualizar_bitmap_a_uno(t_bitarray* bitmap, t_segmento* segmento) { //OJO: MOV_OUT??
    int base = segmento->direccion_base;
    for(int i = 0; i < segmento->tamanio; i++){
    
        bitarray_set_bit(bitmap, base);
        base++;
    }
}

void actualizar_bitmap_a_cero(t_bitarray* bitmap, t_segmento* segmento) {
    int base = segmento->direccion_base;
    for(int i = 0; i < segmento->tamanio; i++){
        bitarray_clean_bit(bitmap, base);
        base++;
}
}


void ocupar_memoria(void* contenido,int base, int tamanio) { //OJO: MOV_OUT
    memcpy(memoria+base, contenido, tamanio);
}


t_segmento* elegir_segun_criterio(t_list* segmentos_candidatos, int tamanio){
    t_segmento* segmento;

    if(strcmp(config_memoria.algoritmo_asignacion, "FIRST") == 0) {
        segmento = list_get(segmentos_candidatos, 0);
    }else if(strcmp(config_memoria.algoritmo_asignacion, "BEST") == 0) {
        segmento = segmento_best_fit(segmentos_candidatos, tamanio);
    }else if(strcmp(config_memoria.algoritmo_asignacion, "WORST") == 0) {
        segmento = segmento_worst_fit(segmentos_candidatos, tamanio);
    }

    return segmento;
}

int generar_id_segmento() { 
    int id = id_global;
    id_global++;
    return id;
}


t_segmento* segmento_best_fit(t_list* candidatos, int tamanio){
    t_segmento* segmento;

    bool igual_tamanio(void* segmento){
        return ((t_segmento*)segmento)->tamanio == tamanio;
    }

    t_segmento* segmento_igual_tamanio = list_find(candidatos, igual_tamanio);
    

    if(segmento_igual_tamanio != NULL){
        segmento = segmento_igual_tamanio; 
    } else {
        segmento = (t_segmento*)list_get_minimum(candidatos, segmento_menor_tamanio);
    }

    return segmento; 
    
}

t_segmento* segmento_worst_fit(t_list* candidatos, int tamanio) {
    t_segmento* segmento;
    segmento = (t_segmento*)list_get_maximum(candidatos, segmento_mayor_tamanio);
    return segmento;
}

void* segmento_menor_tamanio(void* segmento, void* otro) {
    if(((t_segmento*)segmento)->tamanio < ((t_segmento*)otro)->tamanio) {
        return segmento;
    }else {
        return otro;
    }
}

void* segmento_mayor_tamanio(void* segmento, void* otro) {
    if(((t_segmento*)segmento)->tamanio > ((t_segmento*)otro)->tamanio) {
        return segmento;
    }else {
        return otro;
    }
}
/*
void unificar_huecos() { //OJO: esto para delete ("huecos libres aledaños, etc")
    t_list* huecos = buscar_segmentos_libres();
    int cant_huecos = list_size(huecos);
    for(int i = 0; i < cant_huecos; i++) {
        t_segmento* hueco = list_get(huecos, i);
        t_segmento* hueco_siguiente = list_get(huecos, i+1);
        if(hueco->tamanio == hueco_siguiente->direccion_base-1) {
            hueco->tamanio = hueco_siguiente->tamanio;
            list_remove_and_destroy_by_condition(huecos, i+1, (void*) free);
            i--;
            cant_huecos--;
        }
    }
}
*/

//OJO: Ver que tenga + de 1 segmento, ya que puede tener solo el segmento_0 y practicamente no esta ocupado en tal caso
//TODO: Solo agregar a la global, aquellas tablas que tengan 2 o mas segmentos
t_tabla_segmentos* inicializar_tabla_segmentos(int pid){
    
    t_tabla_segmentos* nueva_tabla = malloc(sizeof(t_tabla_segmentos));
    nueva_tabla->pid = pid;
    nueva_tabla->segmentos = tabla_segmentos_inicial();
    list_add(tabla_segmentos_global, nueva_tabla);
    //bitarray_set_bit(bitmap_memoria, 0); posible solucion
    //bitarray_set_bit(bitmap_memoria, 1);
    return nueva_tabla;
}

t_list* tabla_segmentos_inicial(){
    t_list* tabla_inicial = list_create();
    //create_segment(0, segmento_0->tamanio, pid);
    list_add(tabla_inicial, segmento_0);
    return tabla_inicial;
}   

void resultado_compactacion() {
    t_list_iterator* iterador_tabla_global = list_iterator_create(tabla_segmentos_global);
    while(list_iterator_has_next(iterador_tabla_global)){
        t_tabla_segmentos* tabla_segmento = list_iterator_next(iterador_tabla_global);
        int pid = tabla_segmento->pid;
        t_list_iterator* iterador_tabla_segmentos = list_iterator_create(tabla_segmento->segmentos);
        while(list_iterator_has_next(iterador_tabla_segmentos)){
            t_segmento* segmento = (t_segmento*)list_iterator_next(iterador_tabla_segmentos);
            log_info(logger_memoria, "PID: <%d> - Segmento: <%d> - Base: <%d> - Tamaño <%d>", pid, segmento->id_segmento, segmento->direccion_base, segmento->tamanio);          
            
            /* 
            char* buffer_compactacion = malloc(17);
            memcpy(buffer_compactacion, get_value(segmento->direccion_base, 16), 16);
            memcpy(buffer_compactacion + 16, "\0", 1);
            log_warning(logger_memoria, "contenido: %s", buffer_compactacion);
            */
        }

    }

}










