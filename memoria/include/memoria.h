#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <CUnit/CUnit.h>
#include <sys/types.h>
#include <commons/log.h>
#include <pthread.h>
#include "socket.h"
#include "logconfig.h"
#include "operaciones.h"
#include <commons/bitarray.h>

// DEFINICIONES 
//#define MAX_CHAR 100

//VARIABLES GLOBALES
extern t_log* memoria_logger;
extern t_config* config;
extern int socket_memoria;
extern int server_fd;
extern void* espacio_usuario;

//ESTRUCTURAS
typedef struct  
{
	char* ip_memoria;
	char* puerto_escucha;
	char* ip_filesystem;
	char* puerto_filesystem;
	int tam_pagina;
	int tam_memoria;
	char* path_instrucciones;	
	int retardo_respuesta;
	char* algoritmo_reemplazo;
} arch_config;

typedef struct 
{
	int pid;
	int tam_swap;
	//t_list* paginas_en_memoria;
	char* path_proceso;
} t_proceso_en_memoria;

extern t_list* lista_procesos;
extern arch_config config_valores_memoria;

// FUNCIONES//
int atender_clientes_memoria(int);
void manejo_conexiones(void* );
//void manejo_conexiones(int );
void cargar_configuracion(char* );
void enviar_paquete_handshake(int );
void enviar_paquete_instrucciones(int , char* , int );
void crear_tablas_paginas_proceso(int, int );
void liberar_espacio_usuario() ;
char* leer_archivo_instrucciones(char* );
char* buscar_path_proceso(int );


#endif
