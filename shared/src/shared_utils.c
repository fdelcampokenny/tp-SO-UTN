#include <shared_utils.h>

typedef struct { char *key; int val; } t_symstruct;

/*static t_symstruct lookuptable[] = {
    { "SET", I_SET }, 
	{ "EXIT", I_EXIT }
};*/

t_log* iniciar_logger(char* path, char* modulo)
{
	t_log* nuevo_logger;
	nuevo_logger = log_create(path, modulo, 1, LOG_LEVEL_INFO);

	if(nuevo_logger == NULL){
		printf("Logger nulo");
		exit(1);
	}

	return nuevo_logger;
}

t_config* iniciar_config(char* path)
{
	t_config* nuevo_config;
	nuevo_config = config_create(path);
	if(nuevo_config == NULL){
		printf("No se pudo crear config");
	}

	return nuevo_config;
}

//BITMAP


