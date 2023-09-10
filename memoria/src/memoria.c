#include "memoria.h"

//VARIABLES GLOBALES
t_log* memoria_logger;
t_config* config;
int socket_memoria;
int server_fd;
arch_config config_valores_memoria;

int main(void) {
	 memoria_logger = log_create("/home/utnso/tp-2023-2c-KernelTitans/memoria/cfg/memoria.log", "memoria.log", 1, LOG_LEVEL_INFO);

    ///CARGAR LA CONFIGURACION
    cargar_configuracion("/home/utnso/tp-2023-2c-KernelTitans/memoria/cfg/memoria.config");
    log_info(memoria_logger,"Configuracion de memoria cargada correctamente\n");

    log_info(memoria_logger,"Inicializando memoria\n");

    int server_memoria = iniciar_servidor(config_valores_memoria.ip_memoria,config_valores_memoria.puerto_escucha);

    log_info(memoria_logger,"Servidor creado\n");
    log_info(memoria_logger, "Memoria lista para recibir al modulo cliente \n");

    	while(1) 
        {
            atender_clientes_memoria(server_memoria);
        }
	return 0;
}


