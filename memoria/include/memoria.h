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

// DEFINICIONES 
#define MAX_CHAR 60

//VARIABLES GLOBALES
extern t_log* memoria_logger;
extern t_config* config;
extern int socket_memoria;
extern int server_fd;

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

extern arch_config config_valores_memoria;

// FUNCIONES//
int atender_clientes_memoria(int);
void manejo_conexiones(void* );
void cargar_configuracion(char* );
void enviar_paquete_handshake(int );
void enviar_paquete_instrucciones(int , char* );

#endif
