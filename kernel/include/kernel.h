#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
extern t_log *kernel_logger;
extern t_config *config;

extern int server_fd;
extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;
extern int socket_memoria;
extern int socket_filesystem;

extern sem_t mutex_pid;
extern sem_t hay_proceso_nuevo;
extern sem_t grado_multiprogramacion;

extern uint32_t AX;
extern uint32_t BX;
extern uint32_t CX;
extern uint32_t DX;

extern pthread_mutex_t mutex_new;
extern pthread_mutex_t mutex_ready;
extern pthread_mutex_t mutex_exec;
extern pthread_mutex_t mutex_exit;
extern pthread_mutex_t mutex_blocked;
extern pthread_mutex_t mutex_recursos;
extern pthread_mutex_t mutex_colas;
extern t_dictionary_int *diccionario_colas;
extern pthread_mutex_t mutex_corriendo;
extern pthread_cond_t cond_corriendo;
extern pthread_mutex_t cola_locks;



extern t_list *cola_NEW;
extern t_list *cola_READY;
extern t_list *cola_BLOCKED;
extern t_list *cola_EXEC;
extern t_list *cola_EXIT;

//cola de locks (peticiones)
extern t_list *cola_locks_escritura; 
extern t_list *cola_locks_lectura;
//extern t_list *cola_locks_bloqueados;
extern t_list* tabla_global_archivos_abiertos;

extern t_list *lista_recursos;
extern int *instancias_del_recurso;
extern int *instancias_maximas_del_recurso;
extern char **recursos;
extern char **nombres_recursos;


extern int corriendo;
extern bool hay_deadlock;
extern bool existe_en_tabla;
//==============================================================================================================================

typedef struct
{
    char *ip_memoria;
    char *puerto_memoria;
    char *ip_filesystem;
    char *puerto_filesystem;
    char *ip_cpu;
    char *puerto_cpu_dispatch;
    char *puerto_cpu_interrupt;
    char *ip_kernel;
    char *puerto_escucha;
    char *algoritmo_planificacion;
    int quantum;
    char **recursos;
    int grado_multiprogramacion_ini;
    char **instancias_recursos;
} arch_config_kernel;

extern arch_config_kernel config_valores_kernel;


//============================================= Inicializacion =====================================================================
void cargar_configuracion(char *);
void manejar_conexion(int);
void iniciar_proceso(char *, int, int);
void inicializar_diccionarios();
void inicializar_colas();
void inicializar_planificador();
void inicializar_semaforos();
void crear_colas_bloqueo();

//============================================= Planificador =================================================================================================================
void inicializar_planificador();
void planificador_largo_plazo();
void planificador_corto_plazo();
void enviar_path_a_memoria(char *);
void mostrar_lista_pcb(t_list *, char *);
void meter_en_cola(t_pcb *pcb, estado, t_list *);
t_pcb *obtener_siguiente_ready();
void proceso_en_execute(t_pcb *);
void proceso_en_ready();
void proceso_en_sleep(t_pcb *);
void proceso_en_exit(t_pcb *);
void proceso_en_page_fault(t_pcb* );
void obtener_siguiente_blocked(t_pcb*);
t_pcb* obtener_bloqueado_por_recurso(t_list* );
t_pcb *obtener_siguiente_FIFO();
algoritmo obtener_algoritmo();
t_pcb *obtener_siguiente_RR();
t_pcb *obtener_siguiente_PRIORIDADES();

// t_pcb* obtener_siguiente_RR();
t_pcb *obtener_siguiente_new();

////======================================== Relacion con Memoria ===========================================================================================================
void enviar_pcb_a_memoria(t_pcb *, int, op_code);
op_code esperar_respuesta_memoria(int);

void atender_page_fault(t_pcb *);
//================================================== PCB =====================================================================================================================
t_pcb *crear_pcb(int, int, char*); 
void enviar_pcb_a_cpu(t_pcb *);
char *recibir_contexto(t_pcb *);

//================================================ Recursos =====================================================================================================================
int indice_recurso (char* );
void asignacion_recursos(t_pcb* );
char* recibir_peticion_recurso(t_pcb* );
void liberacion_recursos(t_pcb* );
bool proceso_reteniendo_recurso(t_pcb* ,char* );
void deteccion_deadlock (t_pcb*, char* );
void mensaje_deadlock_detectado(t_pcb* , char* );
void liberar_todos_recurso(t_pcb* );

//================================================ Destruir ==================================================================================================================
void eliminar_pcb(t_pcb *);
void eliminar_registros_pcb(t_registros_cpu);
void eliminar_recursos_asignados(t_pcb* );
void eliminar_archivos_abiertos(t_dictionary *);
void eliminar_mutex(pthread_mutex_t *);

//================================================ Consola ==================================================================================================================
void inicializar_consola_interactiva();
void parse_iniciar_proceso(char *linea);
void parse_finalizar_proceso(char *linea);
void parse_detener_planificacion (char* linea);
void parse_iniciar_planificacion (char* linea);
void parse_multiprogramacion(char *linea);
void parse_proceso_estado (char* linea);
void consola_parsear_instruccion(char *leer_linea);
void iniciar_proceso(char *path, int tam_proceso_swap, int prioridad);
void consola_finalizar_proceso(int pid);
void consola_detener_planificacion();
void consola_iniciar_planificacion();
void consola_modificar_multiprogramacion(int);
void consola_proceso_estado();
void detener_planificacion ();

////======================================== File System ===========================================================================================================
void atender_peticiones_al_fs(t_pcb* proceso);
bool es_una_operacion_con_archivos(char* motivo_bloqueo);
t_archivo* buscar_en_tabla_de_archivos_abiertos(char* nombre_a_buscar);
t_archivo_proceso* buscar_en_tabla_de_archivos_proceso(t_pcb* proceso, char* nombre_a_buscar);
void fopen_kernel_filesystem();
void fclose_kernel_filesystem();
void fseek_kernel_filesystem();
void fread_kernel_filesystem();
void fwrite_kernel_filesystem();
void ftruncate_kernel_filesystem();
void iniciar_tabla_archivos_abiertos();
void enviar_solicitud_fs(char* nombre_archivo, op_code operacion, int tamanio, uint32_t posicion, uint32_t direccion_fisica);
void agregar_archivo_tgaa(char* nombre_archivo, int tamanio, uint32_t direccion);
void asignar_archivo_al_proceso(t_archivo* archivo,t_pcb* proceso, char* modo_apertura);
void bloquear_proceso_por_archivo(char* nombre_archivo, t_pcb* proceso, char* modo_apertura);
#endif
