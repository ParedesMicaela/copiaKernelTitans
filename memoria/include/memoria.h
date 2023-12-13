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
extern t_list* procesos_en_memoria;
extern int socket_fs;
extern sem_t solucionado_pf;
extern sem_t swap_creado;
extern pthread_mutex_t mutex_path;
extern pthread_mutex_t mutex_instrucciones;
extern pthread_mutex_t mutex_lista_instrucciones;
extern sem_t swap_finalizado;
extern pthread_mutex_t mutex_tabla_de_paginas;
extern pthread_mutex_t contador_paginas;
extern t_list* tabla_de_paginas;
extern pthread_mutex_t mutex_tiempo;
extern pthread_mutex_t mutex_procesos;

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
	int id;
    int numero_de_pagina;
    int marco; 
    int bit_de_presencia; 
    int bit_modificado; 
    int posicion_swap; 
	int tiempo_uso;
	int tiempo_de_carga;
} t_pagina;

typedef struct 
{
	int pid;
	char* path_proceso;
	t_list* bloques_reservados;
	int cantidad_paginas; 
	t_list* paginas_asignadas;                
} t_proceso_en_memoria;


//======================================================= FUNCIONES =========================================================================================================
/// CONEXIONES y CONFIG ///
int atender_clientes_memoria(int);
void manejo_conexiones(void* );
void cargar_configuracion(char* );
void inicializar_semaforos();

/// @brief CPU + INSTRUCCIONES ///
void enviar_paquete_handshake(int );
void enviar_paquete_instrucciones(int , char* , int );
char* leer_archivo_instrucciones(char* );
char* buscar_path_proceso(int );
void enviar_respuesta_pedido_marco(int socket_cpu, uint32_t num_pagina, int pid);

/// @brief ESPACIO USUARIO ///
void creacion_espacio_usuario();
void liberar_espacio_usuario() ;
void escribir(uint32_t* valor, uint32_t direccion_fisica, int socket_cpu);
uint32_t leer(uint32_t direccion_fisica);
void escribir_en_memoria(void* contenido, size_t tamanio_contenido, uint32_t direccion_fisica);
void* leer_en_memoria(size_t tamanio_contenido, uint32_t direccion_fisica);
void enviar_valor_de_lectura(uint32_t valor, int socket_cpu);

/// @brief  TABLAS DE PAGINAS ///
int buscar_marco(int num_pagina, int pid);
void inicializar_la_tabla_de_paginas(int cantidad_paginas_proceso, int pid);
void inicializar_swap_proceso(int pid_proceso, int cantidad_paginas_proceso);
void crear_tablas_paginas_proceso(int pid, int cantidad_paginas_proceso, char* path_recibido);
void finalizar_en_memoria(int pid);
void escribir_en_memoria_principal(int nro_pagina, int posicion_swap, int pid);
void enviar_pedido_pagina_para_escritura(int pid, int pag_pf);
t_proceso_en_memoria* buscar_proceso_en_memoria(int pid);
t_pagina* buscar_pagina(int num_pagina);
int mas_vieja(t_pagina* una_pag, t_pagina* otra_pag);
int obtener_tiempo();
int obtener_tiempo_carga();

/// @brief SWAP ///
void escribir_en_swap(t_pagina* pagina, int pid);
void swap_in_tabla_proceso (int nro_pagina, int pid);
void recibir_pagina_swap (int pid);
void enviar_paquete_confirmacion_escritura();
void bloques_para_escribir(int tam_bloque, void* contenido, uint32_t puntero_archivo, char* nombre_archivo);

#endif
