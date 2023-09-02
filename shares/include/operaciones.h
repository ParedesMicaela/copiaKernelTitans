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

} op_code;

typedef struct
{
    uint32_t stream_size;
    void *stream;
    int size;
} t_buffer;

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

// SACAR DE PAQUETE //


// MENSAJE //
void enviar_mensaje(char*, int);
void recibir_mensaje(int ,t_log*);

// HANDSHAKE //

//CONEXION //


// BUFFER //


// PAQUETES //
t_list* recibir_paquete_como_lista(int);


// MEMORIA //




#endif
