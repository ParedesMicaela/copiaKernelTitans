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
    int tam_bloque;
    int retardo_acceso_bloque;
    int retardo_acceso_fat;

} arch_config;

typedef struct 
{
    char* nombre_archivo; //Puede ser  char nombre_archivo[256]
    uint32_t tamanio_archivo; //Puede ser size_t
    uint32_t bloque_inicial;
} fcb;

extern fcb config_valores_fcb;

extern arch_config config_valores_filesystem;

void cargar_configuracion(char*);
void*conexion_inicial_memoria();
void atender_clientes_filesystem(void* ); //lo cambie a un void, era int(int, int)
void levantar_archivo_bloque(size_t tamanio_swap, size_t tamanio_fat);
void levantar_fat(size_t tamanio_fat);
void levantar_fcb(char* path);
int crear_archivo (char *nombre_archivo);

#endif
