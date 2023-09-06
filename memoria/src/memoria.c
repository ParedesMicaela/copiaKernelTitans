#include <memoria.h>

t_log* memoria_logger;
t_config* config;
int socket_memoria;
int server_fd_dispatch;
int server_fd_interrupt;

int main(void) {
	 memoria_logger = log_create("/home/utnso/tp-2023-2c-KernelTitans//memoria/memoria.log", "memoria.log", 1, LOG_LEVEL_INFO);

    ///CARGAR LA CONFIGURACION
    cargar_configuracion("/home/utnso/tp-2023-2c-KernelTitans/memoria/Default/memoria.config");
    log_info(memoria_logger,"Configuracion de memoria cargada correctamente\n");

    log_info(memoria_logger,"Inicializando memoria\n");
    //inicializar_memoria();

    int server_fd_dispatch = iniciar_servidor(config_valores_memoria.ip_memoria,config_valores_memoria.puerto_escucha_dispatch);
	int server_fd_interrupt = iniciar_servidor(config_valores_memoria.ip_memoria,config_valores_memoria.puerto_escucha_interrupt);

    log_info(memoria_logger,"Servidor creado\n");
    log_info(memoria_logger, "Memoria lista para recibir al modulo cliente \n");

    	while(atender_clientes_memoria(server_fd));
	return 0;
}


