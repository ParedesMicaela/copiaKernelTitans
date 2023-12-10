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
extern bool hay_page_fault;

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
   uint32_t puntero;
   int pag_pf;
   uint32_t direccion_fisica_proceso;
 	t_registros_cpu registros_cpu;
   t_recursos_asignados* recursos_asignados;  
}t_contexto_ejecucion;

//======================= Funciones AUXILIARES =======================
void cargar_configuracion(char*);
void realizar_handshake();
void ciclo_de_instruccion(int, t_contexto_ejecucion*);
void*conexion_inicial_memoria();
void atender_dispatch(int);
void atender_interrupt(void* );
void finalizar_cpu();

//======================= MEMORIA + REGISTROS=======================
int buscar_registro(char *registros);
uint32_t traducir_de_logica_a_fisica(uint32_t direccion_logica, t_contexto_ejecucion* contexto_ejecucion);
void mov_in(char* registro, uint32_t direccion_logica, t_contexto_ejecucion* contexto_ejecucion);
void mov_out(uint32_t direccion_logica, char* registro, t_contexto_ejecucion* contexto_ejecucion);
void setear_registro(char *registro, int valor);

#endif