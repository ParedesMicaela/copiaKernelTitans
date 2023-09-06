#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <pthread.h>
#include <socket.h>
#include <logconfig.h>
#include <operaciones.h>

//VARIABLES GLOBALES
extern t_log* memoria_logger;
extern t_config* config;
extern int socket_memoria;
extern int server_fd;

//ESTRUCTURAS
typedef struct  
{
	char* ip_memoria;
	char* puerto_escucha_dispatch;
    char* puerto_escucha_interrupt;
} arch_config;

extern arch_config config_valores_memoria;

// FUNCIONES//
int atender_clientes_memoria(int);
void manejo_conexiones(void*);
void cargar_configuracion(char* path);
t_paquete* preparar_paquete_para_handshake();

#endif
