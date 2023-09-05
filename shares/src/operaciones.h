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

typedef struct
{
    uint32_t stream_size;
    void *stream;
    int size;
} t_buffer;


typedef enum
{
	PAQUETE,
	HANDSHAKE,
	PCB,
	FINALIZACION,
	DESALOJO
} op_code;

typedef struct
{
    op_code codigo_operacion;
    t_buffer *buffer;
}t_paquete;



typedef struct{
	uint32_t nro_segmento;
	uint32_t desplazamiento;
} t_handshake;

typedef struct{
	char* puerto;
	char* ip;
}conexion_t;

// OPERACION //
int recibir_operacion(int);

// PAQUETES //
t_paquete* crear_paquete(op_code codigo);
void agregar_caracter_a_paquete(t_paquete* paquete,char caracter);
void agregar_entero_a_paquete(t_paquete* paquete,int numero);
void agregar_cadena_a_paquete(t_paquete* paquete, char* palabra);
void agregar_array_cadenas_a_paquete(t_paquete* paquete, char** palabras);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void* serializar_paquete(t_paquete* paquete, int bytes);
t_paquete* recibir_paquete(int conexion);
char* sacar_cadena_de_paquete(void** stream);
int sacar_entero_de_paquete(void** stream);
char** sacar_array_cadenas_de_paquete(void** stream);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

// MENSAJE //
void enviar_mensaje(char*, int);
void recibir_mensaje(int ,t_log*);

// HANDSHAKE //

//CONEXION //


// BUFFER //


#endif
