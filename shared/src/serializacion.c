#include "serializacion.h"

void* serializar_paquete(t_paquete* paquete) {

    //               stream        +     opcode      +    size
    int bytes = paquete->buffer->size + sizeof(uint8_t) + sizeof(uint32_t);
    void* paqueteSerializado = malloc(bytes);
    int offset = 0; //Desplazamiento para rellenar 

    memcpy(paqueteSerializado + offset, &(paquete->opcode), sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(paqueteSerializado + offset, &(paquete->buffer->size), sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(paqueteSerializado + offset, paquete->buffer->stream, paquete->buffer->size);

    return paqueteSerializado;
}



void eliminar_paquete(t_paquete* paquete) {
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

void enviar_paquete(int socket_destino, uint8_t opcode, t_paquete* paquete)
{
    paquete->opcode = opcode;

    void* paqueteSerializado = serializar_paquete(paquete);

    int bytes = paquete->buffer->size + sizeof(uint8_t) + sizeof(uint32_t);

    send(socket_destino, paqueteSerializado, bytes, 0);

    free(paqueteSerializado);
    eliminar_paquete(paquete);
    
}

t_paquete* recibir_paquete(int socket_cliente)
{
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    recv(socket_cliente, &(paquete->opcode), sizeof(uint8_t), 0); // OPCODE

    recv(socket_cliente, &(paquete->buffer->size), sizeof(uint32_t), 0); // BUFFER SIZE

    paquete->buffer->stream = malloc(paquete->buffer->size); // asigno memoria segun el BUFFER SIZE
    recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0); // recibo el contenido del BUFFER

    return paquete;
}

//AGREGAR SERIALIZAR TABLA DE SEGMENTOS

t_paquete* serializar_tabla_segmentos(t_tabla_segmentos* tabla) {
    //cambie el tipo de base

    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    int tam_segmentos = list_size(tabla->segmentos);

    int bytes = tam_segmentos*2*sizeof(uint32_t) + tam_segmentos*sizeof(int32_t) + sizeof(uint32_t) + sizeof(int);
    paquete->buffer->size = bytes;

    void* stream = malloc(paquete->buffer->size);
    int offset = 0;

    memcpy(stream + offset, &tabla->pid, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(stream + offset, &tam_segmentos, sizeof(int));
    offset += sizeof(int);

    t_list_iterator* iterador = list_iterator_create(tabla->segmentos);

    while(list_iterator_has_next(iterador)){ //el problema esta aca, cuando meto el elemento en aux
        t_segmento* aux = (t_segmento*)list_iterator_next(iterador);

        memcpy(stream + offset, &aux->direccion_base, sizeof(int32_t));
        offset += sizeof(int32_t);
        memcpy(stream + offset, &aux->id_segmento, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(stream + offset, &aux->tamanio, sizeof(uint32_t));
        offset += sizeof(uint32_t);

    }
    list_iterator_destroy(iterador);
    paquete->buffer->stream = stream;

    return paquete;
}



t_tabla_segmentos* deserializar_tabla_segmentos(t_buffer* buffer) {
    t_tabla_segmentos* tabla = malloc(sizeof(t_tabla_segmentos));

    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer

    memcpy(&tabla->pid, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    int tam_segmentos;
    memcpy(&tam_segmentos, stream, sizeof(int));
    stream += sizeof(int);

    tabla->segmentos = list_create();

    for(int i =0; i<tam_segmentos; i++){ 
        t_segmento* aux = malloc(sizeof(t_segmento));

        memcpy(&aux->direccion_base, stream, sizeof(int32_t));
        stream += sizeof(int32_t);
        memcpy(&aux->id_segmento, stream, sizeof(uint32_t));
        stream += sizeof(uint32_t);
        memcpy(&aux->tamanio, stream, sizeof(uint32_t));
        stream += sizeof(uint32_t);

        list_add(tabla->segmentos, aux);
    }

    return tabla;
}

t_paquete* serializar_tabla_global_segmentos(t_list* tabla_global) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    int tamanio_paquete = 0;
    int offset = 0;

    t_list_iterator* iterador_tabla = list_iterator_create(tabla_global);
    while(list_iterator_has_next(iterador_tabla)) {
        t_tabla_segmentos* tabla = list_iterator_next(iterador_tabla);
        int bytes = calcular_bytes_tabla_segmentos(tabla);
        tamanio_paquete += bytes; 
    }

    list_iterator_destroy(iterador_tabla);

    paquete->buffer->size = tamanio_paquete;
    void* stream = malloc(paquete->buffer->size);

    t_list_iterator* iterador = list_iterator_create(tabla_global);
    while(list_iterator_has_next(iterador)) {
        t_tabla_segmentos* tabla = list_iterator_next(iterador);
        t_paquete* tabla_paquete = serializar_tabla_segmentos(tabla);

        memcpy(stream + offset, tabla_paquete->buffer->stream, tabla_paquete->buffer->size);
        offset += tabla_paquete->buffer->size;

        free(tabla_paquete->buffer->stream);
        free(tabla_paquete->buffer);
        free(tabla_paquete);
    }
    list_iterator_destroy(iterador);

    paquete->buffer->stream = stream;
    return paquete;
}

t_list* deserializar_tabla_global_segmentos(t_buffer* buffer) {
    t_list* tabla_global = list_create();

    void* stream = buffer->stream;
    int offset = 0;

    while (offset < buffer->size) {
        t_tabla_segmentos* tabla = malloc(sizeof(t_tabla_segmentos));

        memcpy(&tabla->pid, stream, sizeof(uint32_t));
        stream += sizeof(uint32_t);

        int tam_segmentos;
        memcpy(&tam_segmentos, stream, sizeof(int));
        stream += sizeof(int);

        tabla->segmentos = list_create();

        for (int i = 0; i < tam_segmentos; i++) {
            t_segmento* aux = malloc(sizeof(t_segmento));

            memcpy(&aux->direccion_base, stream, sizeof(int32_t));
            stream += sizeof(int32_t);
            memcpy(&aux->id_segmento, stream, sizeof(uint32_t));
            stream += sizeof(uint32_t);
            memcpy(&aux->tamanio, stream, sizeof(uint32_t));
            stream += sizeof(uint32_t);

            list_add(tabla->segmentos, aux);
        }

        list_add(tabla_global, tabla);

        offset += sizeof(uint32_t) + sizeof(int) + (tam_segmentos * sizeof(t_segmento));
    }

    return tabla_global;
}


int calcular_bytes_tabla_segmentos(t_tabla_segmentos* tabla) {
    int tam_segmentos = list_size(tabla->segmentos);
    int bytes = tam_segmentos * 2 * sizeof(uint32_t) + tam_segmentos * sizeof(int32_t) + sizeof(uint32_t) + sizeof(int);
    return bytes;
}

//Agregar una estructura pcb a un paquete para serializar y enviar
t_paquete* serializar_pcb(t_pcb* pcb) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    //-------------------
    int cantidad_archivos = list_size(pcb->tabla_archivo_x_proceso);
    int tam_archivo = 0;
    //int tam_nombre;
    
    t_list_iterator* iterador_archivo = list_iterator_create(pcb->tabla_archivo_x_proceso);

    while(list_iterator_has_next(iterador_archivo)) {
        t_archivo_x_proceso* archivo = list_iterator_next(iterador_archivo);
        tam_archivo += string_length(archivo->nombre_archivo) +1;// tam_nombre
        tam_archivo += sizeof(int); //tam del tam_nombre
        tam_archivo += sizeof(uint32_t); //tam puntero
    }

    list_iterator_destroy(iterador_archivo);
    

    //tam_archivo = sizeof(int) + (cantidad_archivos * (sizeof(uint32_t) + sizeof(int)));
    //-------------------


    int cant_segmentos = list_size(pcb->tabla_segmentos);
    int tam_tabla = cant_segmentos*2*sizeof(uint32_t) + cant_segmentos*sizeof(int32_t) + sizeof(int);
    t_list_iterator* iterador = list_iterator_create(pcb->tabla_segmentos);

    int tam_instrucciones = string_length(pcb->instrucciones) + 1;
    int tam_recurso_solicitado = string_length(pcb->recurso_solicitado) + 1;

    int bytes = 4*sizeof(uint32_t) + 5*sizeof(double) + 4*sizeof(int) + tam_instrucciones + tam_recurso_solicitado + 124 + tam_tabla + sizeof(int) + tam_archivo + sizeof(bool);///124 de los registros con sus /0
    paquete->buffer->size = bytes;

    void* stream = malloc(paquete->buffer->size);
    int offset = 0;

    ////////
    memcpy(stream + offset, &pcb->primera_aparicion, sizeof(bool));
    offset += sizeof(bool);
    ////////

    memcpy(stream + offset, &pcb->pid, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(stream + offset, &pcb->pc, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(stream + offset, &pcb->estimacion_actual, sizeof(double));
    offset += sizeof(double);
    memcpy(stream + offset, &pcb->llegada_ready, sizeof(double));
    offset += sizeof(double);
    memcpy(stream + offset, &pcb->llegada_running, sizeof(double));
    offset += sizeof(double);
    memcpy(stream + offset, &pcb->estado_actual, sizeof(int));
    offset += sizeof(int);
    memcpy(stream + offset, &pcb->fd_conexion, sizeof(int));
    offset += sizeof(int);
    memcpy(stream + offset, &pcb->rafaga_anterior_real, sizeof(double));
    offset += sizeof(double);
    memcpy(stream + offset, &pcb->tiempo_recurso, sizeof(double));
    offset += sizeof(double);

    memcpy(stream + offset, &pcb->id_segmento, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(stream + offset, &pcb->tamanio_segmento, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(stream + offset, pcb->AX, 5); //espero que sea 5 (teniendo en cuenta /0)
    offset += 5;
    memcpy(stream + offset, pcb->BX, 5); //espero que sea 5 (teniendo en cuenta /0)
    offset += 5;
    memcpy(stream + offset, pcb->CX, 5); //espero que sea 5 (teniendo en cuenta /0)
    offset += 5;
    memcpy(stream + offset, pcb->DX, 5); //espero que sea 5 (teniendo en cuenta /0)
    offset += 5;

    memcpy(stream + offset, pcb->EAX, 9); //espero que sea 5 (teniendo en cuenta /0)
    offset += 9;
    memcpy(stream + offset, pcb->EBX, 9); //espero que sea 5 (teniendo en cuenta /0)
    offset += 9;
    memcpy(stream + offset, pcb->ECX, 9); //espero que sea 5 (teniendo en cuenta /0)
    offset += 9;
    memcpy(stream + offset, pcb->EDX, 9); //espero que sea 5 (teniendo en cuenta /0)
    offset += 9;

    memcpy(stream + offset, pcb->RAX, 17); //espero que sea 5 (teniendo en cuenta /0)
    offset += 17;
    memcpy(stream + offset, pcb->RBX, 17); //espero que sea 5 (teniendo en cuenta /0)
    offset += 17;
    memcpy(stream + offset, pcb->RCX, 17); //espero que sea 5 (teniendo en cuenta /0)
    offset += 17;
    memcpy(stream + offset, pcb->RDX, 17); //espero que sea 5 (teniendo en cuenta /0)
    offset += 17;

    memcpy(stream + offset, &tam_instrucciones, sizeof(int));
    offset += sizeof(int);
    memcpy(stream + offset, pcb->instrucciones, tam_instrucciones);
    offset += tam_instrucciones;

    memcpy(stream + offset, &tam_recurso_solicitado, sizeof(int));
    offset += sizeof(int);
    memcpy(stream + offset, pcb->recurso_solicitado, tam_recurso_solicitado);
    offset += tam_recurso_solicitado;

    ////////////////////

    memcpy(stream + offset, &cant_segmentos, sizeof(int));
    offset += sizeof(int);

    while(list_iterator_has_next(iterador)){ //el problema esta aca, cuando meto el elemento en aux
        t_segmento* aux = (t_segmento*)list_iterator_next(iterador);

        memcpy(stream + offset, &aux->direccion_base, sizeof(int32_t));
        offset += sizeof(int32_t);
        memcpy(stream + offset, &aux->id_segmento, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(stream + offset, &aux->tamanio, sizeof(uint32_t));
        offset += sizeof(uint32_t);

    }
    list_iterator_destroy(iterador);

    memcpy(stream + offset, &cantidad_archivos, sizeof(int));
    offset += sizeof(int);

    // Iterar sobre la lista y copiar cada archivo al stream
    for (int i = 0; i < cantidad_archivos; i++) {
        t_archivo_x_proceso* archivo = list_get(pcb->tabla_archivo_x_proceso, i);

        // Copiar el puntero al stream
        memcpy(stream + offset, &(archivo->puntero), sizeof(uint32_t));
        offset += sizeof(uint32_t);

        // Copiar la longitud del nombre del archivo al stream
        int longitud_nombre = string_length(archivo->nombre_archivo) + 1;
        memcpy(stream + offset, &longitud_nombre, sizeof(int));
        offset += sizeof(int);

        // Copiar el nombre del archivo al stream
        memcpy(stream + offset, archivo->nombre_archivo, longitud_nombre);
        offset += longitud_nombre;
    }

    ////////////////////
    paquete->buffer->stream = stream;

    return paquete;
}

t_pcb* deserializar_pcb(t_buffer* buffer) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->tabla_segmentos = list_create();

    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer

    ///////
    memcpy(&pcb->primera_aparicion, stream, sizeof(bool));
    stream += sizeof(bool);
    ///////
    memcpy(&pcb->pid, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&pcb->pc, stream , sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&pcb->estimacion_actual, stream , sizeof(double));
    stream += sizeof(double);
    memcpy(&pcb->llegada_ready, stream , sizeof(double));
    stream += sizeof(double);
    memcpy(&pcb->llegada_running, stream , sizeof(double));
    stream += sizeof(double);
    memcpy(&pcb->estado_actual, stream , sizeof(int));
    stream += sizeof(int);
    memcpy(&pcb->fd_conexion, stream , sizeof(int));
    stream += sizeof(int);
    memcpy(&pcb->rafaga_anterior_real, stream , sizeof(double));
    stream += sizeof(double);
    memcpy(&pcb->tiempo_recurso, stream , sizeof(double));
    stream += sizeof(double);

    memcpy(&pcb->id_segmento, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&pcb->tamanio_segmento, stream , sizeof(uint32_t));
    stream += sizeof(uint32_t);

    
    pcb->AX = malloc(5);
    memcpy(pcb->AX, stream , 5);
    stream += 5;
    pcb->BX = malloc(5);
    memcpy(pcb->BX, stream , 5);
    stream += 5;
    pcb->CX = malloc(5);
    memcpy(pcb->CX, stream , 5);
    stream += 5;
    pcb->DX = malloc(5);
    memcpy(pcb->DX, stream , 5);
    stream += 5;

    pcb->EAX = malloc(9);
    memcpy(pcb->EAX, stream , 9);
    stream += 9;
    pcb->EBX = malloc(9);
    memcpy(pcb->EBX, stream , 9);
    stream += 9;
    pcb->ECX = malloc(9);
    memcpy(pcb->ECX, stream , 9);
    stream += 9;
    pcb->EDX = malloc(9);
    memcpy(pcb->EDX, stream , 9);
    stream += 9;

    pcb->RAX = malloc(17);
    memcpy(pcb->RAX, stream , 17);
    stream += 17;
    pcb->RBX = malloc(17);
    memcpy(pcb->RBX, stream , 17);
    stream += 17;
    pcb->RCX = malloc(17);
    memcpy(pcb->RCX, stream , 17);
    stream += 17;
    pcb->RDX = malloc(17);
    memcpy(pcb->RDX, stream , 17);
    stream += 17;


    int tam_instrucciones;

    memcpy(&tam_instrucciones, stream, sizeof(int)); 
    stream += sizeof(int);
    pcb->instrucciones = malloc(tam_instrucciones); 
    memcpy(pcb->instrucciones, stream, tam_instrucciones);
    stream += tam_instrucciones;

    int tam_recurso_solicitado;

    memcpy(&tam_recurso_solicitado, stream , sizeof(int));
    stream += sizeof(int);
    pcb->recurso_solicitado = malloc(tam_recurso_solicitado);
    memcpy(pcb->recurso_solicitado, stream, tam_recurso_solicitado);
    stream += tam_recurso_solicitado;

    /////////////

    
    int cant_segmentos;
    memcpy(&cant_segmentos, stream, sizeof(int));
    stream += sizeof(int);

    for(int i =0; i<cant_segmentos; i++){ //aca tambien esta el problema
        t_segmento* aux = malloc(sizeof(t_segmento));

        memcpy(&aux->direccion_base, stream, sizeof(int32_t));
        stream += sizeof(int32_t);
        memcpy(&aux->id_segmento, stream, sizeof(uint32_t));
        stream += sizeof(uint32_t);
        memcpy(&aux->tamanio, stream, sizeof(uint32_t));
        stream += sizeof(uint32_t);

        list_add(pcb->tabla_segmentos, aux);
    }


    //////////////////////////

  pcb->tabla_archivo_x_proceso = list_create();

    int tam_lista;
    int tam_nombre;

    memcpy(&tam_lista, stream, sizeof(int));
    stream += sizeof(int);

    for(int i =0; i<tam_lista; i++){ 
        t_archivo_x_proceso* archivo = malloc(sizeof(t_archivo_x_proceso));

        memcpy(&archivo->puntero, stream, sizeof(uint32_t));
        stream += sizeof(uint32_t);

        memcpy(&tam_nombre, stream, sizeof(int));
        stream += sizeof(int);

        archivo->nombre_archivo = malloc(tam_nombre);
        memcpy(archivo->nombre_archivo, stream, tam_nombre);
        stream += tam_nombre;

        list_add(pcb->tabla_archivo_x_proceso, archivo);

    }
    return pcb;
}


t_paquete* serializar_tabla_archivo_x_proceso(t_pcb* pcb) {

    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    int tam_lista = list_size(pcb->tabla_archivo_x_proceso);
    int tam_archivo;
    int tam_nombre;

    t_list_iterator* iterador_tabla = list_iterator_create(pcb->tabla_archivo_x_proceso);

    while(list_iterator_has_next(iterador_tabla)) {
        t_archivo_x_proceso* archivo = list_iterator_next(iterador_tabla);
        tam_archivo += string_length(archivo->nombre_archivo); //tam_nombre
        tam_archivo += sizeof(int); //tam del tam_nombre
        tam_archivo += sizeof(uint32_t); //tam puntero
    }

    list_iterator_destroy(iterador_tabla);

    int bytes = sizeof(int) + tam_archivo;
    paquete->buffer->size = bytes;

    void* stream = malloc(paquete->buffer->size);
    int offset = 0;

    memcpy(stream + offset, &tam_lista, sizeof(int));
    offset += sizeof(int);

    t_list_iterator* iterador = list_iterator_create(pcb->tabla_archivo_x_proceso);

    while(list_iterator_has_next(iterador)) {
        t_archivo_x_proceso* archivo = list_iterator_next(iterador);

        tam_nombre = string_length(archivo->nombre_archivo);

        memcpy(stream + offset, &archivo->puntero, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        memcpy(stream + offset, &tam_nombre, sizeof(int));
        offset += sizeof(int);

        memcpy(stream + offset, archivo->nombre_archivo, tam_nombre);
        offset += tam_nombre;

    }
    list_iterator_destroy(iterador);
    paquete->buffer->stream = stream;

    return paquete;
}


t_tabla_segmentos* deserializar_tabla_archivo_x_proceso(t_buffer* buffer) {

    void* stream = buffer->stream;

    t_list* tabla_archivo_x_proceso = list_create();

    int tam_lista;
    int tam_nombre;

    memcpy(&tam_lista, stream, sizeof(int));
    stream += sizeof(int);

    for(int i =0; i<tam_lista; i++){ 
        t_archivo_x_proceso* archivo = malloc(sizeof(t_archivo_x_proceso));

        memcpy(&archivo->puntero, stream, sizeof(uint32_t));
        stream += sizeof(uint32_t);

        memcpy(&tam_nombre, stream, sizeof(int));
        stream += sizeof(int);

        archivo->nombre_archivo = malloc(tam_nombre);
        memcpy(archivo->nombre_archivo, stream, tam_nombre);
        stream += tam_nombre;

        list_add(tabla_archivo_x_proceso, archivo);
    }

    return (t_tabla_segmentos*) tabla_archivo_x_proceso;
}



t_paquete* mensaje_only(int32_t contenido){
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer= malloc(sizeof(t_buffer));
    int size_bytes= 4;

    paquete->buffer->size=size_bytes;
    void* stream = malloc(size_bytes);
    memcpy(stream, &contenido, sizeof(int32_t));
    paquete->buffer->stream = stream;
    return paquete;

}
int32_t des_mensaje_only (t_buffer* buffer){
    void* stream = buffer->stream;
    int32_t contenido;

    memcpy(&contenido, stream, sizeof(int32_t));

    return contenido;
}

t_paquete* serializar_instrucciones(char* lista_ins){
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));
    
    int tam_lista = string_length(lista_ins) + 1;
    int offset = 0;

    int size_bytes = tam_lista + sizeof(int);
    paquete->buffer->size = size_bytes;
    void* stream = malloc(size_bytes);

    memcpy(stream, &tam_lista, sizeof(int));
    offset += sizeof(int);
    memcpy(stream + offset, lista_ins, tam_lista);
    
    paquete->buffer->stream = stream;
    
    return paquete;
}

char* deserializar_instrucciones(t_buffer* buffer){
    char* lista_ins;
    void* stream = buffer->stream;
    
    int tam_lista;

    memcpy(&tam_lista, stream, sizeof(int)); 
    stream += sizeof(int);
    lista_ins = malloc(tam_lista); 
    memcpy(lista_ins, stream, tam_lista);
    return lista_ins;
}


//DONE
t_paquete* serializar_escritura_memoria(char* valor_a_escribir, double direccion_fisica, uint32_t tamanio_a_escribir, uint32_t pid){
    
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    
    int size_bytes= tamanio_a_escribir + sizeof(double) + sizeof(uint32_t)*2;
    paquete->buffer->size=size_bytes;
    
    void* stream= malloc(size_bytes);

    int offset=0;

    memcpy(stream+offset,&pid,sizeof(uint32_t));
    offset+= sizeof(uint32_t);
    memcpy(stream+offset, &tamanio_a_escribir,sizeof(uint32_t));
    offset+= sizeof(uint32_t);
    memcpy(stream+offset,&direccion_fisica,sizeof(double));
    offset+=sizeof(double);
    memcpy(stream + offset, valor_a_escribir,tamanio_a_escribir);
    //offset+=tamanio_a_escribir;
   

    paquete->buffer->stream=stream;
    return paquete;

}

//DONE
t_escritura_memoria* deserializar_escritura_memoria(t_buffer* buffer){
    t_escritura_memoria* tricky= malloc(sizeof(t_escritura_memoria));     
    void* stream= buffer->stream;

    memcpy(&tricky->pid,stream,sizeof(uint32_t));
    stream+=sizeof(uint32_t);

    memcpy(&tricky->tamanio_a_escribir,stream,sizeof(uint32_t));
    stream+=sizeof(uint32_t);

    memcpy(&tricky->direccion_fisica, stream, sizeof(double));
    stream+=sizeof(double); 
    
    tricky->valor_a_escribir = malloc(tricky->tamanio_a_escribir);
    memcpy(tricky->valor_a_escribir, stream, tricky->tamanio_a_escribir);
   
    return tricky;

}

//DONE
t_paquete* serializar_lectura_memoria(double dirFisica, uint32_t tamanio, uint32_t pid){

    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    int size_bytes= sizeof(double)+ sizeof(uint32_t) + sizeof(uint32_t);
    paquete->buffer->size=size_bytes;
    
    void* stream= malloc(size_bytes);
    int offset=0;

    
    memcpy(stream+offset,&dirFisica, sizeof(double));
    offset+=sizeof(double);
    memcpy(stream+offset,&tamanio,sizeof(uint32_t));
    offset+=sizeof(uint32_t);
    memcpy(stream+offset,&pid,sizeof(uint32_t));

    paquete->buffer->stream=stream;
    return paquete;
}

//DONE
t_lectura_memoria* deserializar_lectura_memoria(t_buffer* buffer){
    
    t_lectura_memoria* tricky= malloc(sizeof(t_lectura_memoria));     
    void* stream= buffer->stream;

    memcpy(&tricky->direccion_fisica, stream,sizeof(double));
    stream+=sizeof(double);
    memcpy(&tricky->tamanio_registro, stream, sizeof(uint32_t));
    stream+=sizeof(uint32_t); 
    memcpy(&tricky->pid,stream,sizeof(uint32_t));
    return tricky;
}

t_paquete* serializar_mensaje_filesystem(uint32_t puntero, uint32_t tamanio, double direccion_fisica, char* nombre, uint32_t pid){

    t_paquete* paquete= malloc(sizeof(t_paquete));
    paquete->buffer= malloc(sizeof(t_buffer));

    
    uint32_t tamanio_char= strlen(nombre) +1;
    int size_bytes= sizeof(uint32_t)*4 + sizeof(double) + tamanio_char;

    paquete->buffer->size=size_bytes;
    void* stream= malloc(size_bytes);

    
    int offset=0;
    memcpy(stream + offset, &pid, sizeof(uint32_t));
    offset+=sizeof(uint32_t);
    memcpy(stream+offset, &puntero, sizeof(uint32_t));
    offset+=sizeof(uint32_t);
    memcpy(stream + offset, &tamanio, sizeof(uint32_t));
    offset+=sizeof(uint32_t);
    memcpy(stream + offset, &direccion_fisica, sizeof(double));
    offset+=sizeof(double);


    memcpy(stream + offset, &tamanio_char, sizeof(uint32_t));
    offset+=sizeof(uint32_t);
    memcpy(stream + offset, nombre, tamanio_char);
    //offset+=tamanio_char;
    
    paquete->buffer->stream= stream;

    return paquete; 
}

t_mensaje_filesystem* deserializar_mensaje_filesystem(t_buffer* buffer){

    t_mensaje_filesystem* mensaje_filesystem = malloc(sizeof(t_mensaje_filesystem));
    uint32_t tamanio_char;

    void* stream= buffer->stream;

    memcpy(&mensaje_filesystem->pid, stream, sizeof(uint32_t));
    stream+=sizeof(uint32_t);
    memcpy(&mensaje_filesystem->puntero, stream, sizeof(uint32_t));
    stream+=sizeof(uint32_t);
    memcpy(&mensaje_filesystem->tamanio, stream, sizeof(uint32_t));
    stream+=sizeof(uint32_t);
    memcpy(&mensaje_filesystem->direccion_fisica, stream, sizeof(double));
    stream+=sizeof(double);


    memcpy(&tamanio_char, stream, sizeof(uint32_t));
    stream+=sizeof(uint32_t);

    mensaje_filesystem->nombre_archivo = malloc(tamanio_char);
    memcpy(mensaje_filesystem->nombre_archivo, stream, tamanio_char);
    //stream+=tamanio_char;


    return mensaje_filesystem;
}

//! FUNCA
t_paquete* serializar_lista_archivos(t_list* lista_archivos) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    // Obtener la cantidad de elementos en la lista
    int cantidad_archivos = list_size(lista_archivos);

    // Calcular el tamaño total del paquete
    int bytes = sizeof(int) + (cantidad_archivos * (sizeof(uint32_t) + sizeof(int)));

    // Crear el stream de bytes
    void* stream = malloc(bytes);
    int offset = 0;

    // Copiar la cantidad de archivos al stream
    memcpy(stream + offset, &cantidad_archivos, sizeof(int));
    offset += sizeof(int);

    // Iterar sobre la lista y copiar cada archivo al stream
    for (int i = 0; i < cantidad_archivos; i++) {
        t_archivo_x_proceso* archivo = list_get(lista_archivos, i);

        // Copiar el puntero al stream
        memcpy(stream + offset, &(archivo->puntero), sizeof(uint32_t));
        offset += sizeof(uint32_t);

        // Copiar la longitud del nombre del archivo al stream
        int longitud_nombre = strlen(archivo->nombre_archivo) + 1;
        memcpy(stream + offset, &longitud_nombre, sizeof(int));
        offset += sizeof(int);

        // Copiar el nombre del archivo al stream
        memcpy(stream + offset, archivo->nombre_archivo, longitud_nombre);
        offset += longitud_nombre;
    }

    // Establecer el tamaño y el stream en el paquete
    paquete->buffer->size = bytes;
    paquete->buffer->stream = stream;

    return paquete;
}

t_list* deserializar_lista_archivos(t_buffer* buffer) {
    t_list* lista_archivos = list_create();

    // Obtener el stream de bytes del buffer
    void* stream = buffer->stream;

    // Leer la cantidad de archivos del stream
    int cantidad_archivos;
    memcpy(&cantidad_archivos, stream, sizeof(int));
    stream += sizeof(int);

    // Iterar sobre la cantidad de archivos y deserializar cada uno
    for (int i = 0; i < cantidad_archivos; i++) {
        t_archivo_x_proceso* archivo = malloc(sizeof(t_archivo_x_proceso));

        // Leer el puntero del archivo del stream
        memcpy(&(archivo->puntero), stream, sizeof(uint32_t));
        stream += sizeof(uint32_t);

        // Leer la longitud del nombre del archivo del stream
        int longitud_nombre;
        memcpy(&longitud_nombre, stream, sizeof(int));
        stream += sizeof(int);

        // Leer el nombre del archivo del stream
        archivo->nombre_archivo = malloc(longitud_nombre);
        memcpy(archivo->nombre_archivo, stream, longitud_nombre);
        stream += longitud_nombre;

        // Agregar el archivo a la lista
        list_add(lista_archivos, archivo);
    }

    return lista_archivos;
}