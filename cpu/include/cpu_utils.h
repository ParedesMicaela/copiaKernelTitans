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
extern int tam_pagina;

//======================= Estructuras =======================
typedef struct  
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
	char nombre_recurso [50];
	int instancias_recurso;
}t_recursos_asignados;

typedef struct
{
    int pid;
    int program_counter;
    int prioridad;
    int pag_pf;
 	t_registros_cpu registros_cpu;
   t_recursos_asignados* recursos_asignados;  
}t_contexto_ejecucion;

//======================= Funciones =======================
void cargar_configuracion(char*);
void realizar_handshake(int);
void ciclo_de_instruccion(int, int, t_contexto_ejecucion*);
void*conexion_inicial_memoria();
void atender_dispatch(int, int );
void atender_interrupt(void* );
void finalizar_cpu();
void mov_in(uint32_t registro, uint32_t direccion_logica);
void mov_out(uint32_t direccion_logica, uint32_t registro);

#endif
