#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/time.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <libshares/socket.h>
#include <libshares/logconfig.h>
#include <libshares/operaciones.h>

// VARIABLES DE INICIALIZACIÓN //
extern t_log* kernel_logger;
extern t_config* config;
extern int server_fd;
extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;
extern int socket_memoria;
extern int socket_filesystem;

// VARIABLES DE PLANIFICACION //

// VARIABLES COLAS //


// VARIABLES DICCIONARIOS //


// CONFIGURACIÓN //
typedef struct  // archivo de configuracion kernel
 {
    char* ip_memoria;
    char* puerto_memoria;
    char* ip_filesystem;
    char* puerto_filesystem;
    char* ip_cpu;
    char* puerto_cpu_dispatch;
    char* puerto_cpu_interrupt;
	char* ip_kernel;
    char* puerto_escucha;
    char* algoritmo_planificacion;
    int quantum;
    char** recursos;
    int grado_multiprogramacion_ini;
	char** instancias_recursos; 
} arch_config_kernel;

extern arch_config_kernel config_valores_kernel;

// FUNCIONES INICIALIZACIÓN //
void cargar_configuracion(char* path);
int atender_clientes_kernel(int );
void manejar_conexion(int );

// FUNCIONES PLANIFICACIÓN //


// MANEJO RECURSOS //


//FUNCIONES DESTRUIR//


#endif
