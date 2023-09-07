#include "memoria.h"

static char* leer_archivo_instrucciones(char* path_instrucciones);
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

void manejo_conexiones(int socket_cliente){
	while(1){
	int codigo_operacion = recibir_operacion(socket_cliente);
	switch(codigo_operacion){
	case HANDSHAKE: //HANDSHAKE con CPU
		log_info(memoria_logger,"Me llego el handshake :)\n");
		usleep(config_valores_memoria.retardo_respuesta * 1000); //lo retardamos un poquito
		enviar_paquete_handshake(socket_cliente);
		break;
	case MANDAR_INSTRUCCIONES:
		//leemos el archivo de pseudo codigo del path de la config
		char* instrucciones = leer_archivo_instrucciones(config_valores_memoria.path_instrucciones);
		enviar_paquete_instrucciones(socket_cliente, instrucciones);
		break;
	default:
		break;
	}
}
}

// HANDSHAKE A CPU //
void enviar_paquete_handshake(int socket_cliente) {

	t_paquete* handshake=crear_paquete(HANDSHAKE);
	agregar_entero_a_paquete(handshake,config_valores_memoria.tam_pagina);

	enviar_paquete(handshake,socket_cliente);
	log_info(memoria_logger,"Handshake enviado :)\n");
	eliminar_paquete(handshake);
}

// INSTRUCCIONES A CPU //
void enviar_paquete_instrucciones(int socket_cpu, char* instrucciones)
{
    char** lista_instrucciones = string_split(instrucciones, "\n");

    t_paquete* paquete = crear_paquete(INSTRUCCIONES); 

    agregar_array_cadenas_a_paquete(paquete, lista_instrucciones);

    enviar_paquete(paquete, socket_cpu);
	log_info(memoria_logger,"Instrucciones enviadas :)\n");

	eliminar_paquete(paquete);
}

static char* leer_archivo_instrucciones(char* path_instrucciones) {

    FILE* instr_f = fopen(path_instrucciones, "r");
    char* una_cadena    = string_new();
    char* cadena_completa   = string_new();

    if (instr_f == NULL) {
        perror("no se pudo abrir el archivo de instrucciones");
        exit(-1);
    }

    while (!feof(instr_f)) {
        fgets(una_cadena, MAX_CHAR, instr_f);
        string_append(&cadena_completa, una_cadena);
    }
    
    free(una_cadena);
    fclose(instr_f);

    return cadena_completa;
}
