#ifndef CPU_UTILS_H
#define CPU_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include "shared/include/socket.h"
#include "shared/include/logconfig.h"
#include "shared/include/operaciones.h"
#include <math.h>
#include <pthread.h>

//Variables
t_config* config;
t_log* cpu_logger ;
int socket_memoria;
extern t_handshake* configuracion_segmento;
extern arch_config config_valores_cpu;

//======================= Estructuras =======================
typedef struct  // archivo de configuracion cpu
{
   char* ip_cpu;
   char* ip_memoria;
   char* puerto_memoria;
   char* puerto_escucha_dispatch;
   char* puerto_escucha_interrupt;
} arch_config;

typedef struct
{
    int pid;
    int program_counter;
    int prioridad;
    char** registros;
}t_contexto_ejecucion;

typedef struct{
	uint64_t low;
	uint64_t high;
} uint128_t;

//======================= Funciones =======================
void cargar_configuracion(char* path);
void*conexion_inicial_memoria();
void atender_dispatch(int socket_cliente_dispatch, int socket_cliente_memoria);
void atender_interrupt(void* cliente);
void finalizar_cpu();
#endif
