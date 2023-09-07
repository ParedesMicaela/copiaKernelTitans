#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <pthread.h>
#include "socket.h"
#include "logconfig.h"
#include "operaciones.h"

extern t_config *config;
extern t_log *filesystem_logger;
extern int socket_memoria; 
extern int server_fd;

//STRUCTS//
typedef struct  
 {
	char* ip_filesystem;
	char* puerto_filesystem;
    char* ip_memoria;
    char* puerto_memoria;
    char* puerto_escucha;
    char* path_fat;
    char* path_bloques;
    char* path_fcb;
    int cant_bloques_total;
    int cant_bloques_swap;
    int retardo_acceso_bloque;
    int retardo_acceso_fat;

} arch_config;

extern arch_config config_valores_filesystem;

void cargar_configuracion(char*);
void*conexion_inicial_memoria();
int atender_clientes_filesystem(int, int);

#endif