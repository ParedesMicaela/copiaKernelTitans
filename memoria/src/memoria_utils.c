#include <memoria.h>

// INCIALIZAR ESPACIO DE MEMORIA PARA PROCESOS //
void inicializar_memoria(){
    espacio_memoria_usuario=malloc(config_valores_memoria.tam_memoria);
    pthread_mutex_init(&mutex_espacio_memoria_usuario,NULL);
}

// ATENDER CLIENTES//
int atender_clientes_memoria(int socket_servidor){

	int socket_cliente = esperar_cliente(socket_servidor); // se conecta primero cpu
	manejo_conexiones(&socket_cliente);
	return 0;
}

void manejo_conexiones(int socket_cliente){
	while(1){
	int codigo_operacion = recibir_operacion_nuevo(socket_cliente);
	switch(codigo_operacion){
	case HANDSHAKE: //conexion con cualquiera que le manda solo handshake
		log_info(memoria_logger,"Me llego el handshake :)\n");
		usleep(config_valores_memoria.retardo_memoria * 1000); //lo retardamos un poquito
		t_paquete* handshake=preparar_paquete_para_handshake();
		enviar_paquete(handshake,socket_cliente);
		log_info(memoria_logger,"Hanshake enviado :)\n");
		eliminar_paquete(handshake);
		break;
	default:
		break;
	}
}
}

void cargar_configuracion(char* path){

	 config = config_create(path); 

      if (config == NULL) {
          perror("Archivo de configuracion de kernel no encontrado \n");
          abort();
      }

	config_valores_memoria.ip_memoria=config_get_string_value(config,"IP_MEMORIA");
	config_valores_memoria.puerto_escucha_dispatch=config_get_string_value(config,"PUERTO_ESCUCHA_DISPATCH");
    config_valores_memoria.puerto_escucha_interrupt=config_get_string_value(config,"PUERTO_ESCUCHA_INTERRUPT");
}


// ACCESORIOS //
t_paquete* preparar_paquete_para_handshake(){
	t_paquete* paquete=crear_paquete();
	agregar_a_paquete(paquete,&config_valores_memoria.tam_segmento_0,sizeof(int));
	agregar_a_paquete(paquete,&config_valores_memoria.cant_segmentos,sizeof(int));
	return paquete;
}
