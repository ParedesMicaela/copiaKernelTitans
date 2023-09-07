#include "filesystem.h"

int main(void)
{
	 filesystem_logger = log_create("/home/utnso/tp-2023-1c-Desayuno_Pesado/filesystem/cfg/filesystem.log", "filesystem.log", 1, LOG_LEVEL_INFO);

	 cargar_configuracion("/home/utnso/tp-2023-1c-Desayuno_Pesado/filesystem/cfg/filesystem.config");

	 log_info(filesystem_logger, "Archivo de configuracion cargada \n");


    // COMUNICACIÓN MEMORIA //
	socket_memoria = crear_conexion(config_valores_filesystem.ip_memoria, config_valores_filesystem.puerto_memoria);

    // LEVANTAR ARCHIVOS //

    /// CREA LA CONEXION CON EL KERNEL ///

    int server_fd = iniciar_servidor(config_valores_filesystem.ip_filesystem,config_valores_filesystem.puerto_escucha);
    log_info(filesystem_logger, "Filesystem listo para recibir al modulo cliente \n");
    int cliente_fd = esperar_cliente(server_fd);
    log_info(filesystem_logger,"Se conecto un cliente \n");
/*
    while (1)
        {
        int cod_op = recibir_operacion_nuevo(cliente_fd);
        switch (cod_op)
            {
            case -1:
                log_error(filesystem_logger, "Fallo la comunicacion. Abortando \n");
            default:
                log_warning(filesystem_logger, "Operacion desconocida \n");
                break;
            }
        }
        //while(atender_clientes_kernel(server_fd));
  */      
    return EXIT_SUCCESS;
}






