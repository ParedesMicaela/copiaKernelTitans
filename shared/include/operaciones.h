#ifndef OPERACIONES_H_
#define OPERACIONES_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // para los uint32_t de los registros de CPU
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/config.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include "pthread.h"
#include "semaphore.h"
#include "inttypes.h"


//==================================================== Estructuras =========================================================================================================
typedef enum
{
	PAQUETE,
	HANDSHAKE,
	RECIBIR_PATH,
	MANDAR_INSTRUCCIONES,
	CREACION_ESTRUCTURAS_MEMORIA,
	INSTRUCCIONES,
	PCB,
	FINALIZACION,
	FINALIZAR_EN_MEMORIA,
	DESALOJO
} op_code;

typedef enum{
	NEW,
	READY,
	EXEC,
	BLOCKED,
	EXIT
} estado;

typedef enum{
	FIFO,
	PRIORIDADES,
	RR
} algoritmo;

typedef enum {
	SET,
	SUM,
	SUB,
	WAIT,
	SIGNAL,
	INSTRUCCION_EXIT
} codigo_instrucciones;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
    op_code codigo_operacion;
    t_buffer *buffer;
}t_paquete;

typedef struct{
	char* puerto;
	char* ip;
}conexion_t;

//======================================================= PCB ==============================================================================================================
typedef struct registros_cpu
{
	uint32_t AX;
	uint32_t BX;
	uint32_t CX;
	uint32_t DX;
} t_registros_cpu;

typedef struct
{
	int pid;
	int program_counter;
 	t_registros_cpu registros_cpu;
	int prioridad;
	//en el estado vamos a ir viendo en que parte del ciclo de instruccion esta
	estado estado_pcb;
	int quantum;
	char** recursos_asignados;
	char* recurso_pedido; /*el proceso me va a pedir un recurso a la vez, entonces puedo hacer esta variable
	que empiece en NULL y despues si me pide un recurso, lo pongo aca y luego lo vacio nuevamente para que pueda pedir mas.
	Capaz nos sirve para el deadlock despues*/
	
	//t_dictionary *archivosAbiertos;
	//acá le vamos agregando todo lo que vayamos necesitando en el pcb
	//pthread_mutex_t *mutex;
	//aca NO vamos a poner las cosas con las que se relaciona el proceso en memoria (tam paginas por ejemplo)
	//vamos a ponerlo en memoria pero despues
}t_pcb; //declaro el pcb

//======================================================= Operaciones ======================================================================================================
int recibir_operacion(int);
int enviar_datos(int , void *, uint32_t );
int recibir_datos(int , void *, uint32_t );
t_paquete *crear_paquete_con_codigo_de_operacion(uint8_t codigo);
void crear_buffer(t_paquete *paquete);
void *recibir_stream(int *size, int socket_cliente);


//======================================================= Paquetes =========================================================================================================
t_paquete* crear_paquete(op_code );
void agregar_caracter_a_paquete(t_paquete* ,char );
void agregar_entero_a_paquete(t_paquete* ,int );
void agregar_entero_sin_signo_a_paquete(t_paquete* , uint32_t);
void agregar_cadena_a_paquete(t_paquete* , char* );
void agregar_array_cadenas_a_paquete(t_paquete* , char** );
void agregar_a_paquete(t_paquete* , void* , int );
void* serializar_paquete(t_paquete* , int );
t_paquete* recibir_paquete(int );
char* sacar_cadena_de_paquete(void** );
int sacar_entero_de_paquete(void** );
uint32_t sacar_entero_sin_signo_de_paquete(void** );
char** sacar_array_cadenas_de_paquete(void** );
void enviar_paquete(t_paquete* , int );
void eliminar_paquete(t_paquete* );

//SERIALIZACION
void* serializar_paquete(t_paquete* , int );

//======================================================= Mensaje ==========================================================================================================
void enviar_mensaje(char*, int);
void recibir_mensaje(int ,t_log*);

/*======================================================= Buffer ======================================================
void* recibir_buffer(int*, int);
void crear_buffer(t_paquete*);
void agregar_a_buffer(t_buffer*, void*, int);
t_buffer* inicializar_buffer_con_parametros(uint32_t, void*);
*/


#endif
