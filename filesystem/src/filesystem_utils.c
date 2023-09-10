#include "filesystem.h"
//..................................CONFIGURACIONES.....................................................................

void cargar_configuracion(char* path) {

       config = config_create(path); //Leo el archivo de configuracion

      if (config == NULL) {
          perror("Archivo de configuracion de filesystem no encontrado \n");
          abort();
      }

      config_valores_filesystem.ip_filesystem = config_get_string_value(config, "IP_FILESYSTEM");
      config_valores_filesystem.puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");
      config_valores_filesystem.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
      config_valores_filesystem.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
      config_valores_filesystem.puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
      config_valores_filesystem.path_fat = config_get_string_value(config, "PATH_FAT");
      config_valores_filesystem.path_bloques = config_get_string_value(config, "PATH_BLOQUES");
      config_valores_filesystem.path_fcb = config_get_string_value(config, "PATH_FCB");
      config_valores_filesystem.cant_bloques_total = config_get_int_value(config, "CANT_BLOQUES_TOTAL");
      config_valores_filesystem.cant_bloques_swap = config_get_int_value(config, "CANT_BLOQUES_SWAP");
      config_valores_filesystem.retardo_acceso_bloque = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");
      config_valores_filesystem.retardo_acceso_fat = config_get_int_value(config, "RETARDO_ACCESO_FAT");
}

// ATENDER CLIENTES //

void atender_clientes_filesystem(void* conexion) {
    int cliente_fd = *(int*)conexion;

    int cod_op = recibir_operacion(cliente_fd);
    log_info(filesystem_logger,"codigo op: %d", cod_op);
    switch (cod_op)
        {
        case -1:
            log_error(filesystem_logger, "Fallo la comunicacion. Abortando \n");
        default:
            log_warning(filesystem_logger, "Operacion desconocida \n");
        break;
        }
}

/*
/// COMUNICACIÓN A MEMORIA ///
void* conexion_inicial_memoria(){

	int codigo_memoria;

	while(1){
		codigo_memoria=recibir_operacion(socket_memoria);
		switch(codigo_memoria){
			case PAQUETE:
				log_info(filesystem_logger,"Recibi configuracion por handshake \n");
				return NULL;
			break;
			case -1:
				log_error(filesystem_logger, "Fallo la comunicacion. Abortando \n");
				return (void*)(EXIT_FAILURE);
			break;
			default:
				 log_warning(filesystem_logger, "Operacion desconocida \n");
			break;
		}
	}
	 return (void*)(EXIT_SUCCESS);
}
*/