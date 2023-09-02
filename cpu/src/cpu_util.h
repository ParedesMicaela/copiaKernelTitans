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
#include <libshares/socket.h>
#include <libshares/logconfig.h>
#include <libshares/operaciones.h>
#include <math.h>
#include <pthread.h>

t_config* config;
t_handshake* configuracion_segmento;
t_log* cpu_logger ;
int socket_memoria;

//======================= Estructuras =======================
typedef struct  // archivo de configuracion cpu
{
   char* ip_cpu;
   int retardo_instruccion;
   char* ip_memoria;
   char* puerto_memoria;
   char* puerto_escucha;
   char* puerto_escucha_kernel;
   int tamanio_maximo_segmento;
} arch_config;

arch_config config_valores_cpu;

typedef struct{
	uint64_t low;
	uint64_t high;
} uint128_t;

//======================= Funciones =======================
void cargar_configuracion(char* path);
void*conexion_inicial_memoria();
void atender_cliente(int socket_cpu, int socket_cliente);
void finalizar_cpu();
#endif
