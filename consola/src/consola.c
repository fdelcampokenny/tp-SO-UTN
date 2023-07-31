#include "consola.h"

char *path_archivo;

int main(int argc, char **argv)
{
    inicializar_consola();

    procesar_entrada(argc, argv, logger_consola);

    char *lista_ins = armar_lista_instrucciones(path_archivo, logger_consola);

    free(path_archivo);

    int conexion_kernel = iniciar_cliente(config_consola.ip_kernel,
                                   config_consola.puerto_kernel,
                                   logger_consola);

    log_info(logger_consola, "%s", lista_ins);

    t_paquete *paquete = serializar_instrucciones(lista_ins);
    enviar_paquete(conexion_kernel, NUEVAS_INSTRUCCIONES, paquete);

    log_info(logger_consola, "se envio el paquete");

    // recibo la respuesta del servidor (delegar en otra funcion)
    //esto es un modelo
    void *valor = malloc(sizeof(void *));
    recv(conexion_kernel, valor, sizeof(int), 0);
    int valorcito = *((int *)valor);
    
    log_info(logger_consola, "Recibio: %d ", valorcito);
    free(valor);
}

void inicializar_consola()
{
    logger_consola = iniciar_logger(PATH_LOG_CONSOLA, "CONSOLA");
    config1 = iniciar_config(PATH_CONFIG_CONSOLA);

    config_consola.ip_kernel = config_get_string_value(config1, "IP_KERNEL");
    config_consola.puerto_kernel = config_get_string_value(config1, "PUERTO_KERNEL");
}

char *armar_lista_instrucciones(char *pathArchivo, t_log *logger)
{
    char *lista_ins = string_new();

    FILE *archivo = fopen(pathArchivo, "r");
    if (archivo == NULL)
    {
        log_error(logger, "No se pudo abrir el archivo.");
        exit(EXIT_FAILURE);
    }
    char linea[256];
    while (fgets(linea, sizeof(linea), archivo))
    {
        string_append(&lista_ins, linea);
    }

    lista_ins += '\0';

    fclose(archivo);
    return lista_ins;
}

void procesar_entrada(int argc,char** argv, t_log* logger){
    if (argc < 2)
    {
        log_error(logger, "Error no se proporcionaron argumentos ");
        exit(EXIT_FAILURE);
    }
    else
    {
        path_archivo = malloc((strlen(argv[1]) + 1) * sizeof(char));
        strcpy(path_archivo, argv[1]);
    }
    
}