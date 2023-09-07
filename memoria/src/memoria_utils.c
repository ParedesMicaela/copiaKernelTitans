#include "memoria.h"

/*void* mutex_espacio_memoria_usuario;

// INCIALIZAR ESPACIO DE MEMORIA PARA PROCESOS //
void inicializar_memoria(){
    espacio_memoria_usuario=malloc(config_valores_memoria.tam_memoria);
    pthread_mutex_init(&mutex_espacio_memoria_usuario,NULL);
}*/

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
// ATENDER CLIENTES//
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

void manejo_conexiones(int socket_cliente){
	while(1){
	int codigo_operacion = recibir_operacion(socket_cliente);
	switch(codigo_operacion){
	case HANDSHAKE: //conexion con cualquiera que le manda solo handshake
		log_info(memoria_logger,"Me llego el handshake :)\n");
		usleep(config_valores_memoria.retardo_respuesta * 1000); //lo retardamos un poquito
		t_paquete* handshake=preparar_paquete_para_handshake(HANDSHAKE);
		enviar_paquete(handshake,socket_cliente);
		log_info(memoria_logger,"Hanshake enviado :)\n");
		eliminar_paquete(handshake);
		break;
	default:
		break;
	}
}
}

// HANDSHAKE A CPU //
t_paquete* preparar_paquete_para_handshake(){
	t_paquete* paquete=crear_paquete(HANDSHAKE);
	agregar_entero_a_paquete(paquete,config_valores_memoria.tam_pagina);
	return paquete;
}

