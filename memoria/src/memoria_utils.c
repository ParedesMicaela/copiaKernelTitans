#include "memoria.h"

int cantidad_paginas_proceso;
char* path_recibido = NULL;

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

	int socket_cliente = esperar_cliente(socket_servidor); 

	if(socket_cliente != -1){
		log_info(memoria_logger, "Se conecto un cliente \n");
		pthread_t hilo_cliente;
		pthread_create(&hilo_cliente, NULL, (void*) manejo_conexiones, &socket_cliente);
		pthread_detach(hilo_cliente);
		return 1;
	}else {
		log_error(memoria_logger, "Error al escuchar clientes... Finalizando servidor \n"); 
	}
	return 0;
}

void manejo_conexiones(void* socket_cliente)
{
	int cliente = *(int*)socket_cliente;
	int posicion_pedida = 0;
	int pid_proceso = 0;
	uint32_t valor_registro = 0;
	uint32_t direccion_fisica = 0;
	char* path_asignado = NULL;
	uint32_t numero_pagina;
	int socket_fs; //Ver como se matchea

	while(1){
	t_paquete* paquete = recibir_paquete(cliente);
    void* stream = paquete->buffer->stream;

	switch(paquete->codigo_operacion){		
	case HANDSHAKE:
		//Mandamos por Handshake el tam_pagina
		log_info(memoria_logger,"Me llego el handshake :)\n");
		int entero = sacar_entero_de_paquete(&stream);
		enviar_paquete_handshake(cliente);
		break;

	case MANDAR_INSTRUCCIONES:

		//la cpu nos manda el program counter y el pid del proceso que recibio para ejecutar
		posicion_pedida = sacar_entero_de_paquete(&stream);
		pid_proceso = sacar_entero_de_paquete(&stream);

		//aca vamos a buscar el proceso con el pid que recibimos y obtener el path_asignado
		path_asignado = buscar_path_proceso(pid_proceso); 

		//mandamos directamente el path del proceso porque ahi ya voy a tener las instrucciones leidas y cargadas
		usleep(config_valores_memoria.retardo_respuesta * 1000);  //Retardo consigna
		enviar_paquete_instrucciones(cliente, path_asignado, posicion_pedida);
		break;

	case CREACION_ESTRUCTURAS_MEMORIA:

		pid_proceso = sacar_entero_de_paquete(&stream);
		cantidad_paginas_proceso = sacar_entero_de_paquete(&stream);
		path_recibido = sacar_cadena_de_paquete(&stream);

		//De esta manera la memoria le pasa el path a la CPU
		config_valores_memoria.path_instrucciones = path_recibido;
		log_info(memoria_logger, "PATH recibido: %s", config_valores_memoria.path_instrucciones );

		crear_tablas_paginas_proceso(pid_proceso, cantidad_paginas_proceso, path_recibido);
		inicializar_swap_proceso(pid_proceso,cantidad_paginas_proceso, socket_fs);

        int ok_creacion = 1;
        send(cliente, &ok_creacion, sizeof(int), 0);
		log_info(memoria_logger,"Estructuras creadas en memoria kernel-kyunn\n");
		break;

	case FINALIZAR_EN_MEMORIA:
		int pid = sacar_entero_de_paquete(&stream);
		log_info(memoria_logger,"Recibi pedido de creacion de estructuras en memoria\n");
		//finalizar_en_memoria(pid, socket_fs);
	    int ok_finalizacion = 1;
        send(cliente, &ok_finalizacion, sizeof(int), 0);
		log_info(memoria_logger,"Estructuras eliminadas en memoria kernel-kyunn\n");
		break;

	case TRADUCIR_PAGINA_A_MARCO:
		numero_pagina = sacar_entero_sin_signo_de_paquete(&stream);
		pid_proceso = sacar_entero_de_paquete(&stream);
		enviar_respuesta_pedido_marco(cliente, numero_pagina, pid_proceso);
		break;

	case WRITE:
		pid_proceso = sacar_entero_de_paquete(&stream);
		direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
		valor_registro = sacar_entero_sin_signo_de_paquete(&stream);
		
		escribir(&valor_registro, direccion_fisica);

		break;

	case SOLUCIONAR_PAGE_FAULT:
		pid_proceso = sacar_entero_de_paquete(&stream);		// ya tengo definido en FINALIZAR_MEMORIA el int pid, sino el make se quejaba :(
		int pag_pf = sacar_entero_de_paquete(&stream);

		log_info(memoria_logger,"Recibi un pedido para solucionar page fault uwu con PID %d - Pagina %d", pid, pag_pf);

		/*El módulo deberá solicitar al módulo File System la página correspondiente y escribirla en la memoria principal.
		En caso de que la memoria principal se encuentre llena,
		se deberá seleccionar una página víctima utilizando el algoritmo de reemplazo.
		Si la víctima se encuentra modificada, se deberá previamente escribir en SWAP.
		*/
	default:
		break;
	}
	eliminar_paquete(paquete);
	}
}

