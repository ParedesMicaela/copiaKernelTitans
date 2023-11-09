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
extern int socket_filesystem;
extern int server_fd;
extern void* espacio_usuario;
extern t_list* procesos_en_memoria;
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
    int cantidad_entradas;                 
} t_pagina;

typedef struct 
{
	int pid;
	t_pagina* paginas_en_memoria;
	char* path_proceso;
	t_list* bloques_reservados;
} t_proceso_en_memoria;


//======================================================= FUNCIONES =========================================================================================================
/// CONEXIONES y CONFIG ///
int atender_clientes_memoria(int);
void manejo_conexiones(void* );
void cargar_configuracion(char* );

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
void enviar_valor_de_lectura(uint32_t valor, int socket_cpu);

/// @brief  TABLAS DE PAGINAS ///
int buscar_marco(int pid, int num_pagina);
void inicializar_la_tabla_de_paginas();
void inicializar_swap_proceso(int pid_proceso, int cantidad_paginas_proceso, int socket_fs);
void crear_tablas_paginas_proceso(int pid, int cantidad_paginas_proceso, char* path_recibido);
void finalizar_en_memoria(int pid, int socket_fs);
void escribir_en_memoria_principal(int numero_de_pagina, int marco, int posicion_swap, int pid);
void enviar_pedido_pagina_para_escritura(int pid, int pag_pf);
void recibir_pagina_para_escritura();
t_proceso_en_memoria* buscar_proceso_en_memoria(int pid);


#endif
