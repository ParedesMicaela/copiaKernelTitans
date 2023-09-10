#include "filesystem.h"

//Variables Globales//
t_log* filesystem_logger;
t_config* config;
int server_fd;
int* cliente_fd;
int socket_memoria;
arch_config config_valores_filesystem;

int main(void)
{
	 filesystem_logger = log_create("/home/utnso/tp-2023-2c-KernelTitans/filesystem/cfg/filesystem.log", "filesystem.log", 1, LOG_LEVEL_INFO);

	 cargar_configuracion("/home/utnso/tp-2023-2c-KernelTitans/filesystem/cfg/filesystem.config");

	 log_info(filesystem_logger, "Archivo de configuracion cargada \n");


    // COMUNICACIÓN MEMORIA //
	socket_memoria = crear_conexion(config_valores_filesystem.ip_memoria, config_valores_filesystem.puerto_memoria);

    // LEVANTAR ARCHIVOS //

    /// CREA LA CONEXION CON KERNEL Y MEMORIA ///
    int server_fd = iniciar_servidor(config_valores_filesystem.ip_filesystem,config_valores_filesystem.puerto_escucha);
    log_info(filesystem_logger, "Filesystem listo para recibir al modulo cliente \n");

    while(1) 
    {
        int* cliente_fd = malloc(sizeof(int));
        *cliente_fd = esperar_cliente(server_fd);
        log_info(filesystem_logger,"Se conecto un cliente \n");
        pthread_t multihilo;
	    pthread_create(&multihilo,NULL,(void*) atender_clientes_filesystem, cliente_fd);
	    pthread_detach(multihilo);
    }    

    return EXIT_SUCCESS;
}






