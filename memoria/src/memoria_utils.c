#include "memoria.h"

// CONFIGURACION //
void cargar_configuracion(char* path){
	
	config = config_create(path); 

	      if (config == NULL) {
	          perror("Archivo de configuracion de filesystem no encontrado \n");
	          abort();
	      }
	config_valores_memoria.ip_memoria=config_get_string_value(config,"IP_MEMORIA");
	config_valores_memoria.puerto_escucha=config_get_string_value(config,"PUERTO_ESCUCHA");
	config_valores_memoria.ip_filesystem=config_get_string_value(config,"IP_FILESYSTEM");
	config_valores_memoria.puerto_filesystem=config_get_string_value(config,"PUERTO_FILESYSTEM");
	config_valores_memoria.tam_memoria=config_get_int_value(config,"TAM_MEMORIA");
	config_valores_memoria.tam_pagina=config_get_int_value(config,"TAM_PAGINA");
	config_valores_memoria.path_instrucciones=config_get_string_value(config,"PATH_INSTRUCCIONES");
	config_valores_memoria.retardo_respuesta=config_get_int_value(config,"RETARDO_RESPUESTA");
	config_valores_memoria.algoritmo_reemplazo=config_get_string_value(config,"ALGORITMO_REEMPLAZO");
}
// ATENDER CLIENTES CON HILOS//
int atender_clientes_memoria(int socket_servidor){

	int socket_cliente = esperar_cliente(socket_servidor); // se conecta primero cpu

	if(socket_cliente != -1){
		log_info(memoria_logger, "Se conecto un cliente \n");
		pthread_t hilo_cliente;
		pthread_create(&hilo_cliente, NULL, (void*) manejo_conexiones, &socket_cliente);
		pthread_detach(hilo_cliente);
		return 1;
	}else {
		log_error(memoria_logger, "Error al escuchar clientes... Finalizando servidor \n"); // log para fallo de comunicaciones
	}
	return 0;
}

void manejo_conexiones(void* socket_cliente)
{
	int cliente = *(int*)socket_cliente;
	while(1){
	t_paquete* paquete = recibir_paquete(cliente);
    void* stream = paquete->buffer->stream;

	switch(paquete->codigo_operacion){		
	case HANDSHAKE:

		//va a recibir un handshake de la cpu y le va a tener que mandar como min el tam_pagina
		log_info(memoria_logger,"Me llego el handshake :)\n");

		//lo retardamos
		usleep(config_valores_memoria.retardo_respuesta * 1000); 
		enviar_paquete_handshake(cliente);
		break;

	case RECIBIR_PATH:

		//del kernel va a recibir un path que va a tener que leer para pasarle a la cpu la instruccion
		char* path_recibido = sacar_cadena_de_paquete(&stream);

		/*el path va a ser distinto al del archivo de config entonces lo tengo que modificar
	    no lo pongo en MANDAR_INSTRUCCIONES porque el kernel no me pide que le lea la instruccion, eso me lo pide la cpu
		el kernel me dice solamente el path que necesito leerle a la cpu*/
		config_valores_memoria.path_instrucciones = path_recibido;

		log_info(memoria_logger, "PATH recibido: %s", path_recibido);

	case MANDAR_INSTRUCCIONES:
		//leemos el archivo de pseudo codigo del path de la config  y lo metemos en una cadena TODO JUNTO
		char* instrucciones_leidas = leer_archivo_instrucciones(config_valores_memoria.path_instrucciones);

		//necesito sacar del paquete la posicion de program_counter
		int posicion_pedida = sacar_entero_de_paquete(&stream);
		enviar_paquete_instrucciones(cliente, instrucciones_leidas,posicion_pedida);
		break;

	case CREACION_ESTRUCTURAS_MEMORIA:
	//esto no anda, hay que verlo
		/*int pid_proceso = sacar_entero_de_paquete(&stream);
		int tam_swap_pid = sacar_entero_de_paquete(&stream);

		log_info(memoria_logger,"Recibi pedido de creacion de estructuras en memoria\n");
		crear_tablas_paginas_proceso(pid_proceso, tam_swap_pid);
        int ok_creacion = 1; 
		enviar_datos(socket_cliente,&ok_creacion,sizeof(int));*/
	case FINALIZAR_EN_MEMORIA:

	default:
		break;
	}
}
}


