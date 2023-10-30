#include "memoria.h"

t_list* lista_procesos;


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

	//int socket_cliente = malloc(sizeof(int));
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
	int posicion_pedida = 0;
	int pid_proceso = 0;
	char* path_asignado = NULL;

	while(1){
	t_paquete* paquete = recibir_paquete(cliente);
    void* stream = paquete->buffer->stream;

	switch(paquete->codigo_operacion){		
	case HANDSHAKE:

		//va a recibir un handshake de la cpu y le va a tener que mandar como min el tam_pagina
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

    	t_proceso_en_memoria* proceso = malloc(sizeof(t_proceso_en_memoria));
		
		pid_proceso = sacar_entero_de_paquete(&stream);
		int tam_swap_pid = sacar_entero_de_paquete(&stream);

		//del kernel va a recibir un path que va a tener que leer para pasarle a la cpu la instruccion
		char* path_recibido = sacar_cadena_de_paquete(&stream);

		//aca cargamos los datos en nuestra estructura proceso
		proceso->pid = pid_proceso;
		proceso->tam_swap = tam_swap_pid;

		/*el path va a ser distinto al del archivo de config entonces lo tengo que modificar
	    no lo pongo en MANDAR_INSTRUCCIONES porque el kernel no me pide que le lea la instruccion, eso me lo pide la cpu
		el kernel me dice solamente el path que necesito leerle a la cpu*/
		config_valores_memoria.path_instrucciones = path_recibido;

		/*decidi leer el path que me manda el kernel al principio, y guardar las instrucciones dentro de mi
		estructura proceso para no tener que leer el archivo cada vez que tengo que mandar instrucciones. Si lo 
		hago solamente al principio y guardo las instrucciones leidas en el path_proceso del proceso, me ahorro
		tener que hacerlo todo el tiempo y ademas ya lo tengo todo leido y preparado para solamente devolver el
		indice (program_counter) que me pide*/
		char* instrucciones_leidas = leer_archivo_instrucciones(path_recibido);
		
		proceso->path_proceso = strdup(instrucciones_leidas);

		log_info(memoria_logger, "PATH recibido: %s", config_valores_memoria.path_instrucciones );

		//decidi hace una lista para buscar mas facil los datos de cada proceso
		list_add(lista_procesos, (void*)proceso);

		log_info(memoria_logger,"Recibi pedido de creacion de estructuras en memoria\n");
		//crear_tablas_paginas_proceso(pid_proceso, tam_swap_pid);
        int ok_creacion = 1;
        send(cliente, &ok_creacion, sizeof(int), 0);
		log_info(memoria_logger,"Estructuras creadas en memoria kernel-kyunn\n");
		//free(instrucciones_leidas);
		//free(path_recibido);
		break;

	case FINALIZAR_EN_MEMORIA:
		int pid = sacar_entero_de_paquete(&stream);
		log_info(memoria_logger,"Recibi pedido de creacion de estructuras en memoria\n");
		//funcion finalizar
        int ok_finalizacion = 1;
        send(cliente, &ok_finalizacion, sizeof(int), 0);
		log_info(memoria_logger,"Estructuras eliminadas en memoria kernel-kyunn\n");
		break;

	case SOLUCIONAR_PAGE_FAULT:
		pid = sacar_entero_de_paquete(&stream);		// ya tengo definido en FINALIZAR_MEMORIA el int pid, sino el make se quejaba :(
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

