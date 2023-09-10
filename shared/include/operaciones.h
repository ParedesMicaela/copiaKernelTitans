#ifndef OPERACIONES_H_
#define OPERACIONES_H_

#include <stdio.h>
#include <stdlib.h>
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


// ESTRUCTURAS //
typedef enum
{
	PAQUETE,
	HANDSHAKE,
	MANDAR_INSTRUCCIONES,
	INSTRUCCIONES,
	PCB,
	FINALIZACION,
	DESALOJO
} op_code;

typedef enum {
	SET,
	SUM,
	SUB,
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

// OPERACION //
int recibir_operacion(int);

int enviar_datos(int , void *, uint32_t );

t_paquete *crear_paquete_con_codigo_de_operacion(uint8_t codigo);
void crear_buffer(t_paquete *paquete);
void *recibir_stream(int *size, int socket_cliente);


// PAQUETES //
t_paquete* crear_paquete(op_code );
void agregar_caracter_a_paquete(t_paquete* ,char );
void agregar_entero_a_paquete(t_paquete* ,int );
void agregar_cadena_a_paquete(t_paquete* , char* );
void agregar_array_cadenas_a_paquete(t_paquete* , char** );
void agregar_a_paquete(t_paquete* , void* , int );
void* serializar_paquete(t_paquete* , int );
t_paquete* recibir_paquete(int );
char* sacar_cadena_de_paquete(void** );
int sacar_entero_de_paquete(void** );
char** sacar_array_cadenas_de_paquete(void** );
void enviar_paquete(t_paquete* , int );
void eliminar_paquete(t_paquete* );

//SERIALIZACION
void* serializar_paquete(t_paquete* , int );

// MENSAJE //
void enviar_mensaje(char*, int);
void recibir_mensaje(int ,t_log*);

// BUFFER //
void* recibir_buffer(int*, int);
void crear_buffer(t_paquete*);
void agregar_a_buffer(t_buffer*, void*, int);
t_buffer* inicializar_buffer_con_parametros(uint32_t, void*);

//CONEXION //


// BUFFER //


#endif
