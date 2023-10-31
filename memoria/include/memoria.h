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
extern t_list* lista_procesos;
//ESTRUCTURAS
typedef struct  
{
	char* ip_memoria;
	char* puerto_escucha;
	char* ip_filesystem;
	char* puerto_filesystem;
	int tam_memoria;
	int tam_pagina;
	char* path_instrucciones;	
	int retardo_respuesta;
	char* algoritmo_reemplazo;
} arch_config;

extern arch_config config_valores_memoria;

typedef struct {
    int numero_de_pagina;
    int marco; //Revisar si es unit32_t
    int bit_de_presencia; //Puede ser un bool
    int bit_modificado; //Puede ser un bool
    int posicion_swap; //Revisar si es unit32_t
} entrada_t_pagina;

typedef struct {
    entrada_t_pagina* entradas;
    int tamanio;                 
} t_pagina;

typedef struct 
{
	int pid;
	int tam_swap;
	t_list* paginas_en_memoria;
	char* path_proceso;
} t_proceso_en_memoria;


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

/// @brief  TABLAS DE PAGINAS ///
int buscar_marco(int pid, int num_pagina);
void inicializar_la_tabla_de_paginas();

#endif
