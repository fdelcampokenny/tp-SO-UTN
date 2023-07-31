#include "cpu.h"


int main(int argc, char **argv){
   
    inicializar_cpu(argv);
    servidor_cpu = iniciar_servidor(config_cpu.ip_cpu, config_cpu.puerto_cpu, logger_cpu);
    conexion_memoria = iniciar_cliente(config_cpu.ip_memoria, config_cpu.puerto_memoria, logger_cpu);
    conexion_kernel = esperar_cliente(servidor_cpu, logger_cpu);
    
    inicializar_ciclo_cpu();

}

void inicializar_ciclo_cpu() {
	pthread_create(&hilo_ciclo_cpu, NULL, (void*)ciclo_cpu, NULL);
	pthread_join(hilo_ciclo_cpu, NULL);
}

void ciclo_cpu(){

	while(1){
		t_paquete* paquete = recibir_paquete(conexion_kernel);
		if (paquete == NULL) {
			continue;
		}
		t_pcb* pcb = deserializar_pcb(paquete->buffer);
		//eliminar_paquete(paquete);
        t_list* instrucciones = parsearInstrucciones(pcb->instrucciones);
        cicloInstruccion(pcb, instrucciones);
		list_destroy(instrucciones);
	}
}


t_list* parsearInstrucciones(char* lista_instrucciones){	
    t_list* listaIns = list_create();
    char** lineas = string_split(lista_instrucciones, "\n"); 
	int i = 0; 
    char** array;
    while (lineas[i] != NULL){

		array = string_split(lineas[i], " ");
    
		t_instruccion* nuevaIns = malloc(sizeof(t_instruccion));
        
		if (strcmp(array[0], "YIELD") == 0) {
            nuevaIns->identificador = YIELD;
			nuevaIns->parametro1 = string_duplicate("-1"); 
			nuevaIns->parametro2 = string_duplicate("-1");
            nuevaIns->parametro3 = string_duplicate("-1");
		} else if (strcmp(array[0], "SET") == 0) {
			nuevaIns->identificador = SET;
			nuevaIns->parametro1 = string_duplicate(array[1]); 
			nuevaIns->parametro2 = string_duplicate(array[2]);
            nuevaIns->parametro3 = string_duplicate("-1");
		}else if (strcmp(array[0], "EXIT") == 0) {   
			nuevaIns->identificador = EXIT;
			nuevaIns->parametro1 = string_duplicate("-1");
			nuevaIns->parametro2 = string_duplicate("-1");
            nuevaIns->parametro3 = string_duplicate("-1");
		}else if (strcmp(array[0], "I/O") == 0) {
            nuevaIns->identificador = I_O;
            nuevaIns->parametro1 = string_duplicate(array[1]);
            nuevaIns->parametro2 = string_duplicate("-1");
            nuevaIns->parametro3 = string_duplicate("-1");
        }else if (strcmp(array[0], "WAIT") == 0){
            nuevaIns->identificador = WAIT;
            nuevaIns->parametro1 = string_duplicate(array[1]);
            nuevaIns->parametro2 = string_duplicate("-1");
            nuevaIns->parametro3 = string_duplicate("-1");
        }else if (strcmp(array[0], "SIGNAL") == 0){
            nuevaIns->identificador = SIGNAL;
            nuevaIns->parametro1 = string_duplicate(array[1]);
            nuevaIns->parametro2 = string_duplicate("-1");
            nuevaIns->parametro3 = string_duplicate("-1");
        }else if (strcmp(array[0], "CREATE_SEGMENT") == 0){
            nuevaIns->identificador = CREATE_SEGMENT;
			nuevaIns->parametro1 = string_duplicate(array[1]);
			nuevaIns->parametro2 = string_duplicate(array[2]);
            nuevaIns->parametro3 = string_duplicate("-1");
        }else if (strcmp(array[0], "MOV_IN") == 0){
            nuevaIns->identificador = MOV_IN;
			nuevaIns->parametro1 = string_duplicate(array[1]); 
			nuevaIns->parametro2 = string_duplicate(array[2]);
            nuevaIns->parametro3 = string_duplicate("-1");
        }else if (strcmp(array[0], "MOV_OUT") == 0){
            nuevaIns->identificador = MOV_OUT;
			nuevaIns->parametro1 = string_duplicate(array[1]); 
			nuevaIns->parametro2 = string_duplicate(array[2]);
            nuevaIns->parametro3 = string_duplicate("-1");
        }else if (strcmp(array[0], "DELETE_SEGMENT") == 0){          
            nuevaIns->identificador = DELETE_SEGMENT;
            nuevaIns->parametro1 = string_duplicate(array[1]);
            nuevaIns->parametro2 = string_duplicate("-1");
            nuevaIns->parametro3 = string_duplicate("-1");
        }else if (strcmp(array[0], "F_OPEN") == 0){
            nuevaIns->identificador = F_OPEN;
            nuevaIns->parametro1 = string_duplicate(array[1]);
            nuevaIns->parametro2 = string_duplicate("-1");
            nuevaIns->parametro3 = string_duplicate("-1");
        }else if (strcmp(array[0], "F_TRUNCATE") == 0){
            nuevaIns->identificador = F_TRUNCATE;
            nuevaIns->parametro1 = string_duplicate(array[1]);
            nuevaIns->parametro2 = string_duplicate(array[2]);
            nuevaIns->parametro3 = string_duplicate("-1");
        }else if (strcmp(array[0], "F_CLOSE") == 0){
            nuevaIns->identificador = F_CLOSE;
            nuevaIns->parametro1 = string_duplicate(array[1]);
            nuevaIns->parametro2 = string_duplicate("-1");
            nuevaIns->parametro3 = string_duplicate("-1");
         }else if (strcmp(array[0], "F_SEEK") == 0){
            nuevaIns->identificador = F_SEEK;
            nuevaIns->parametro1 = string_duplicate(array[1]);
            nuevaIns->parametro2 = string_duplicate(array[2]);
            nuevaIns->parametro3 = string_duplicate("-1");
        }else if (strcmp(array[0], "F_WRITE") == 0){
            nuevaIns->identificador = F_WRITE;
            nuevaIns->parametro1 = string_duplicate(array[1]);
            nuevaIns->parametro2 = string_duplicate(array[2]);
            nuevaIns->parametro3 = string_duplicate(array[3]);
        }else if (strcmp(array[0], "F_READ") == 0){
            nuevaIns->identificador = F_READ;
            nuevaIns->parametro1 = string_duplicate(array[1]);
            nuevaIns->parametro2 = string_duplicate(array[2]);
            nuevaIns->parametro3 = string_duplicate(array[3]);
        }
	list_add(listaIns, nuevaIns);
    string_array_destroy(array);
	//free(array[0]);
	//free(array[1]);
    //free(array[2]);
    //free(array[3]);
	//free(array);
	//free(lineas[i]);
	i++;
    }
    
    //free(lineas);
    string_array_destroy(lineas);
    return listaIns;
}

void cicloInstruccion(t_pcb *pcb, t_list *instrucciones)
{ // esto es para ejecutar una serie de instrucciones

    t_instruccion *instruccionActual;
    t_paquete *paquete;
    t_paquete *paquete_fs;

    bool enviar_mensaje_filesystem = false;
    bool devolver_pcb = false;

    log_info(logger_cpu, "PID: %d  PC: %d",pcb->pid, pcb->pc);
    instruccionActual = list_get(instrucciones, pcb->pc);
    pcb->pc += 1;
    log_info(logger_cpu, "Identificador proxima instruccion: <%d>", instruccionActual->identificador);
    switch (instruccionActual->identificador)
    {

    case SET:
    { 
    sleep(config_get_int_value(config3, "RETARDO_INSTRUCCION") / 1000); //TODO: descomentar
    agregar_a_registro(instruccionActual->parametro1, instruccionActual->parametro2, pcb);
    log_warning(logger_cpu, "Registro: %s, valor: %s", instruccionActual->parametro1, instruccionActual->parametro2);
    log_info(logger_cpu, "Instruccion SET ejecutada");
    break;
    } // DONE

    case YIELD:
    {
    log_info(logger_cpu, "Instruccion YIELD ejecutada");
    devolver_pcb = true;
    break;
    } // DONE

    case EXIT:
    {
    log_info(logger_cpu, "Instruccion EXIT ejecutada");
    devolver_pcb = true;
    break;
    } // DONE

    case I_O:
    {
    pcb->tiempo_recurso = atof(instruccionActual->parametro1);
    log_info(logger_cpu, "Instruccion I/O ejecutada");
    devolver_pcb = true;
    break;
    // DONE
    }

    case WAIT:
    {
    pcb->recurso_solicitado = string_duplicate(instruccionActual->parametro1);
    log_info(logger_cpu, "Recurso: %s", pcb->recurso_solicitado);
    log_info(logger_cpu, "Instruccion WAIT ejecutada");
    devolver_pcb = true;
    break; // DONE
    }

    case SIGNAL:
    {
    pcb->recurso_solicitado = string_duplicate(instruccionActual->parametro1);
    log_info(logger_cpu, "Recurso: %s", pcb->recurso_solicitado);
    log_info(logger_cpu, "Instruccion SIGNAL ejecutada");
    devolver_pcb = true;
    break; // DONE
    }

    case CREATE_SEGMENT:
    {
    pcb->id_segmento = atoi(instruccionActual->parametro1); // hay que castear de uint32 a int?
    pcb->tamanio_segmento = atoi(instruccionActual->parametro2);
    log_info(logger_cpu, "Crear segmento de ID: <%d> con tamanio:  <%d>", pcb->id_segmento, pcb->tamanio_segmento);
    log_info(logger_cpu, "Instruccion CREATE_SEGMENT ejecutada");
    devolver_pcb = true;
    break;
    }// DONE

    case DELETE_SEGMENT:
    {
    pcb->id_segmento = atoi(instruccionActual->parametro1);
    log_info(logger_cpu, "Elimino segmento <%d>", pcb->id_segmento);
    log_info(logger_cpu, "Instruccion DELETE_SEGMENT ejecutada");
    devolver_pcb = true;
    break;
    } // DONE

    case MOV_OUT:
    {
    char *registro = instruccionActual->parametro2;
    char *register_value = obtener_valor_registro(registro, pcb);
    uint32_t tamanio_registro = calcular_tamanio_registro(registro);

    char* log_register = malloc(tamanio_registro + 1);
    memcpy(log_register, register_value, tamanio_registro);
    memcpy(register_value + tamanio_registro, "\0", 1);
    //log_warning(logger_cpu, "memoria, escribite esto: %s", register_value);

    double dirFisica = traducir_direccion_logica(instruccionActual->parametro1, pcb, tamanio_registro);

    if(dirFisica ==-1){
        instruccionActual->identificador=SEG_FAULT;
        devolver_pcb=true;
    break;
    }

    t_paquete *paquete = serializar_escritura_memoria(register_value, dirFisica, tamanio_registro, pcb->pid);
    enviar_paquete(conexion_memoria, MOV_OUT, paquete);
    log_info(logger_cpu, "Instruccion MOV_OUT ejecutada");
    //devolver_pcb=false;
    free(register_value);

    break;
    } // DONE
    

    case MOV_IN:
    {
    char *registro = instruccionActual->parametro1;
    char *dirlogica = instruccionActual->parametro2;

    uint32_t tamanio_registro = calcular_tamanio_registro(registro);
    double dirFisica = traducir_direccion_logica(dirlogica, pcb, tamanio_registro);
    
    if(dirFisica ==-1){
        instruccionActual->identificador=SEG_FAULT;
        devolver_pcb=true;
    break;
    }


    t_paquete *paquete = serializar_lectura_memoria(dirFisica, tamanio_registro, pcb->pid);
    enviar_paquete(conexion_memoria, MOV_IN, paquete);

    
    //RECIBIR UN VALOR DE MEMORIA
    char *valor = malloc(tamanio_registro);
    char* string_buffer = malloc(tamanio_registro+1);
    //log_info(logger_cpu, "paquete enviado, esperando rta...");

    recv(conexion_memoria, valor, tamanio_registro, 0);
    memcpy(string_buffer, valor, tamanio_registro);
    memcpy(string_buffer + tamanio_registro, "\0", 1);
    
    //log_info(logger_cpu, "Recibi el valor <%s> para escribirlo en <%s>", string_buffer, registro);
    //agregar_a_registro(registro, valor, pcb);
    
    free(valor);
    free(string_buffer);
    log_info(logger_cpu, "Instruccion MOV_IN ejecutada");
    //devolver_pcb=false;

    break;
    } // DONE
   
    case F_OPEN:
    {
    char *nombre_archivo = instruccionActual->parametro1;
    paquete_fs = serializar_mensaje_filesystem(0, 0, 0, nombre_archivo, pcb->pid);
    
    devolver_pcb = true;
    enviar_mensaje_filesystem = true;

    log_info(logger_cpu, "Instruccion F_OPEN ejecutada");
    break;
    } // DONE
    case F_CLOSE:
    {
    char *nombre_archivo = instruccionActual->parametro1;
    paquete_fs = serializar_mensaje_filesystem(0, 0, 0, nombre_archivo, pcb->pid);
    devolver_pcb = true;
    enviar_mensaje_filesystem = true;

    log_info(logger_cpu, "Instruccion F_CLOSE ejecutada");

    break;
    } // DONE
    case F_SEEK:
    {
    char *nombre_archivo = instruccionActual->parametro1;
    int aux= atoi(instruccionActual->parametro2);
    uint32_t puntero = (uint32_t)aux;

    paquete_fs = serializar_mensaje_filesystem(puntero, 0, 0, nombre_archivo, pcb->pid);

    enviar_mensaje_filesystem = true;
    devolver_pcb = true;
    log_info(logger_cpu, "Instruccion F_SEEK ejecutada");
    break;
    } // DONE
    case F_READ:
    {
    log_info(logger_cpu, "Entro al F_READ CPU");
    char *nombre_archivo = instruccionActual->parametro1;
    int aux = atoi(instruccionActual->parametro3);
    uint32_t tamanio = (uint32_t)aux;
    double direccion_fisica = traducir_direccion_logica(instruccionActual->parametro2, pcb, tamanio);

    paquete_fs = serializar_mensaje_filesystem(0, tamanio, direccion_fisica, nombre_archivo, pcb->pid);
    devolver_pcb = true;
    enviar_mensaje_filesystem = true;
    log_info(logger_cpu, "Instruccion F_READ ejecutada");
    break;
    }// DONE
    case F_WRITE:
    {
    char *nombre_archivo = instruccionActual->parametro1;
    int aux = atoi(instruccionActual->parametro3);
    uint32_t tamanio = (uint32_t)aux;
    double direccion_fisica = traducir_direccion_logica(instruccionActual->parametro2, pcb, tamanio);

    paquete_fs = serializar_mensaje_filesystem(0, tamanio, direccion_fisica, nombre_archivo, pcb->pid);
    devolver_pcb = true;
    enviar_mensaje_filesystem = true;

    log_info(logger_cpu, "Instruccion F_WRITE ejecutada");

    break;
    }//DONE
    case F_TRUNCATE:
    {
    char *nombre_archivo = instruccionActual->parametro1;
    int aux = atoi(instruccionActual->parametro2);
    uint32_t tamanio = (uint32_t)aux;
    paquete_fs = serializar_mensaje_filesystem(0, tamanio, 0, nombre_archivo, pcb->pid);
    devolver_pcb = true;
    enviar_mensaje_filesystem = true;

    log_info(logger_cpu, "Instruccion F_TRUNCATE ejecutada");

    break;
    }// DONE

    default:
    {
    log_warning(logger_cpu, "El ENUM: <%d> No matchea con ninguno de los case",instruccionActual->identificador);
    break;
    }
    }

    
    // CASOS DONDE HAY QUE DEVOLVER PCB O MENSAJE_FILESYSTEM

    if (!devolver_pcb)
    {
    cicloInstruccion(pcb, instrucciones);
    }
    else if (devolver_pcb)
    {
    paquete = serializar_pcb(pcb);
    enviar_paquete(conexion_kernel, instruccionActual->identificador, paquete);
    }
    if (enviar_mensaje_filesystem)
    {
    enviar_paquete(conexion_kernel, instruccionActual->identificador, paquete_fs);
    }
}

void inicializar_cpu(char** argv){

	logger_cpu = iniciar_logger(PATH_LOG_CPU, "CPU");
    
    char* path_archivo = malloc((strlen(argv[1]) + 1) * sizeof(char));
    strcpy(path_archivo, argv[1]);
    config3 = iniciar_config(path_archivo);

	config_cpu.ip_cpu = config_get_string_value(config3, "IP_CPU");
	config_cpu.puerto_cpu = config_get_string_value(config3, "PUERTO_ESCUCHA");

	config_cpu.ip_memoria = config_get_string_value(config3, "IP_MEMORIA");
	config_cpu.puerto_memoria = config_get_string_value(config3, "PUERTO_MEMORIA");

    config_cpu.tam_max_segmento = config_get_int_value(config3, "TAM_MAX_SEGMENTO");

	
}

double traducir_direccion_logica(char* direccionLogica, t_pcb* pcb, uint32_t tamanio_valor) {

    uint32_t direccion_logica = atoi(direccionLogica);
    uint32_t num_segmento = floor(direccion_logica / config_cpu.tam_max_segmento);
    uint32_t desplazamiento_segmento = direccion_logica % config_cpu.tam_max_segmento;

    bool _es_segmento(void* un_segmento) {//PROBABLEMENTE ROMPE, FUCK LAMBDAS
        return (((t_segmento*)un_segmento)->id_segmento == num_segmento);
    }

    //t_list* tabla_segmentos = pcb->tabla_segmentos;
    t_segmento* segmento = malloc(sizeof(t_segmento));
    segmento = list_find(pcb->tabla_segmentos, _es_segmento);//ROMPE

    uint32_t base = segmento->direccion_base;
    uint32_t limite = segmento->tamanio;
    uint32_t direccion_fisica = base + desplazamiento_segmento; 
    //free(segmento);
    //log_warning(logger_cpu, "num segmento: %d - desplazamiento segmento: %d", num_segmento, desplazamiento_segmento);
    
    
    if(desplazamiento_segmento + tamanio_valor > limite) { 
        log_info(logger_cpu, "Error Segmentation Fault: PID: <%d> - Error SEF_FAULT - Segmento: <%d> - Offset: <%d> - Tamaño: <%d>",pcb->pid, segmento->id_segmento, desplazamiento_segmento, limite);
        
        return -1;
    }else{
        
        return direccion_fisica; 

    }
}

void agregar_a_registro(char* registro, char* valor, t_pcb* pcb){
    
    if (registro[1] == 'X') // primer caso registro 4 bytes
    {
        if (registro[0]=='A'){
            strcpy(pcb->AX, valor);
        }
        if (registro[0]=='B'){
            strcpy(pcb->BX, valor);
        }
        if (registro[0]=='C'){
            strcpy(pcb->CX, valor);
        }
        if (registro[0]=='D'){
            strcpy(pcb->DX, valor);
        }
    }
    if (registro[0]== 'E'){ //registro de 8 bytes
        if (registro[1]== 'A'){
            strcpy(pcb->EAX, valor);
        }
        if (registro[1]== 'B'){
            strcpy(pcb->EBX, valor);
        }
        if (registro[1]== 'C'){
            strcpy(pcb->ECX, valor);
        }
        if (registro[1]== 'D'){
            strcpy(pcb->EDX, valor);
        }
    }else{
        if (registro[1]== 'A'){
            strcpy(pcb->RAX, valor);
        }
        if (registro[1]== 'B'){
            strcpy(pcb->RBX, valor);
        }
        if (registro[1]== 'C'){
            strcpy(pcb->RCX, valor);
        }
        if (registro[1]== 'D'){
            strcpy(pcb->RDX, valor);
        }
    }
}


char* obtener_valor_registro(char* registro, t_pcb* pcb){

    char* valor_registro = string_new();
    
    if (registro[1] == 'X') // primer caso registro 4 bytes
    {
        if (registro[0]=='A'){
            memcpy(valor_registro, pcb->AX,5);
        }
        if (registro[0]=='B'){
            memcpy(valor_registro, pcb->BX,5);
        }
        if (registro[0]=='C'){
            memcpy(valor_registro, pcb->CX,5);
        }
        if (registro[0]=='D'){
            memcpy(valor_registro, pcb->DX,5);
        }
    }
    if (registro[0]== 'E'){ //registro de 8 bytes
        if (registro[1]== 'A'){
            memcpy(valor_registro, pcb->EAX,9);
        }
        if (registro[1]== 'B'){
            memcpy(valor_registro, pcb->EBX,9);
        }
        if (registro[1]== 'C'){
            memcpy(valor_registro, pcb->ECX,9);
        }
        if (registro[1]== 'D'){
            memcpy(valor_registro, pcb->EDX,9);
        }
    
    }else{
        if (registro[1]== 'A'){
            memcpy(valor_registro, pcb->RAX,17);
        }
        if (registro[1]== 'B'){
            memcpy(valor_registro, pcb->RBX,17);
        }
        if (registro[1]== 'C'){
            memcpy(valor_registro, pcb->RCX,17);
        }
        if (registro[1]== 'D'){
            memcpy(valor_registro, pcb->RDX,17);
        }
    }

    return valor_registro;
}


uint32_t calcular_tamanio_registro(char* registro){
char* registros_4="AX BX CX DX";
char* registros_8= "EAX EBX ECX EDX";



if(string_contains(registros_4, registro)){
    return (uint32_t)4;
    
}
else if(string_contains(registros_8, registro)){
    return (uint32_t)8;
    
} else {
    
    return (uint32_t)16;
}
};
