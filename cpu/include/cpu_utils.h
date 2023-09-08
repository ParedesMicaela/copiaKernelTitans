#ifndef CPU_UTILS_H
#define CPU_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <CUnit/CUnit.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include "socket.h"
#include "logconfig.h"
#include "operaciones.h"
#include <math.h>
#include <pthread.h>

//Variables
extern t_config* config;
extern t_log* cpu_logger ;
extern int socket_cliente_memoria;
extern int socket_servidor_dispatch;
extern int socket_servidor_interrupt;
extern int socket_cliente_dispatch;
extern int socket_cliente_interrupt;

//======================= Estructuras =======================
typedef struct  // archivo de configuracion cpu
{
   char* ip_cpu;
   char* ip_memoria;
   char* puerto_memoria;
   char* puerto_escucha_dispatch;
   char* puerto_escucha_interrupt;
} arch_config;

extern arch_config config_valores_cpu;
typedef struct
{
    int pid;
    int program_counter;
    int prioridad;
    char** registros;
}t_contexto_ejecucion;

//======================= Funciones =======================
void cargar_configuracion(char*);
void realizar_handshake(int);
void ciclo_de_instruccion(int, int, t_contexto_ejecucion*);
void*conexion_inicial_memoria();
void atender_dispatch(int, int );
void atender_interrupt(void* );
void finalizar_cpu();


void pedir_handshake(int );

#endif
