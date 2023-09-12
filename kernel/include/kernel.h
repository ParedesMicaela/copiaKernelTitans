#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>  
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/log.h>
#include "socket.h"
#include "logconfig.h"
#include "operaciones.h"
#include "dictionary_int.h"

//================================================== Variables =====================================================================
extern t_log* kernel_logger;
extern t_config* config;
extern int server_fd;
extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;
extern int socket_memoria;
extern int socket_filesystem;

//==============================================================================================================================

typedef struct 
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

//============================================= Inicializacion =====================================================================
void cargar_configuracion(char* );
int atender_clientes_kernel(int );
void manejar_conexion(int );
void iniciar_proceso (char* , int , int );
void inicializar_diccionarios();
void inicializar_colas();
void inicializar_planificador();

//============================================= Planificador =================================================================================================================
void planificador_largo_plazo();
void planificador_corto_plazo();
void enviar_path_a_memoria(char* );
void mostrar_lista_pcb(t_list* );
void meter_en_cola(t_pcb* , estado );

////========================================= Relacion con Memoria ===========================================================================================================
void enviar_path_a_memoria(char* );

//================================================== PCB =====================================================================================================================
t_pcb* crear_pcb(int, int); //como 2do parametro había un uint32_t que tiraba error ya que time_swap(en el .c) estaba como int
void enviar_pcb_a_cpu(t_pcb* );

//================================================ Destruir ==================================================================================================================
void finalizar_kernel();

#endif
