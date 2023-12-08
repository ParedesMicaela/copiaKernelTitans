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

extern int tamanio_recursos;

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
	FINALIZACION_PROCESO,
	FINALIZAR_EN_MEMORIA,
	PAGE_FAULT,
	SOLUCIONAR_PAGE_FAULT_MEMORIA,
	SOLUCIONAR_PAGE_FAULT_FILESYSTEM,
	TRADUCIR_PAGINA_A_MARCO,
	INICIALIZAR_SWAP,
	ESCRIBIR_EN_MEMORIA,
	LEER_EN_MEMORIA,
	LISTA_BLOQUES_RESERVADOS,
	PAGINA_PARA_ESCRITURA,
	ESCRIBIR_PAGINA_SWAP,
	PEDIR_PAGINA_PARA_ESCRITURA,
	NUMERO_MARCO,
	READ,
	VALOR_READ,
	WRITE,
	DESALOJO,
	ARCHIVO_CREADO,
	ARCHIVO_NO_EXISTE,
	ARCHIVO_EXISTE,
	POSICIONARSE_ARCHIVO,
	ABRIR_ARCHIVO,
	CREAR_ARCHIVO,
	TRUNCAR_ARCHIVO,
	LEER_ARCHIVO,
	ESCRIBIR_ARCHIVO,
	ESCRIBIR_EN_ARCHIVO_BLOQUES,
	INICIAR_PROCESO,
	FINALIZAR_PROCESO,
	LIBERAR_SWAP,
	CERRAR_ARCHIVO,
	BUSCAR_ARCHIVO,
	ARCHIVO_ABIERTO
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
	JNZ,
	SLEEP,
	WAIT,
	SIGNAL,
	MOV_IN,
	MOV_OUT,
	F_OPEN,
	F_CLOSE,
	F_SEEK,
	F_READ,
	F_WRITE,
	F_TRUNCATE,
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
	char nombre_recurso [50];
	int instancias_recurso;

}t_recurso;

typedef struct
{
	char *nombre_archivo;  
    int tamanio;
	bool lock_escritura;
	bool lock_lectura;
	uint32_t *bloque_inicial;
	//(el primer bloque del archivo, apenas lo creamos con la cant de bloques es el puntero al 1er bloque, asi es mas facil buscarlo)
} fcb_proceso;

typedef struct {   
	fcb_proceso* fcb;
    uint32_t* puntero_posicion; //si lo manejamos con file, como lo habias hecho creoque es mas facil
	char* modo_apertura;
	//esto es distinto al bloque inical porque podemos usar FSEEK. el bloque incial es siempre el mismo para moverse mas facil
} t_archivo_proceso;

typedef struct {   
	fcb_proceso* fcb;
    uint32_t* puntero_posicion;
	t_list* cola_solicitudes;
} t_archivo;


typedef struct
{
	int pid;
	int program_counter;
 	t_registros_cpu registros_cpu;
	int prioridad;
	estado estado_pcb;
	int quantum;
	t_recurso* recursos_asignados;
	char* recurso_pedido; 
	int sleep;
	char* motivo_bloqueo; 
	int pagina_pedida;
	char* path_proceso;
	char* nombre_archivo;
    char* modo_apertura;
    int posicion;
    uint32_t direccion_fisica_proceso;
    int tamanio_archivo;
	t_list* archivos_abiertos;

}t_pcb; 

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
void agregar_lista_de_cadenas_a_paquete(t_paquete* , t_list*);
void agregar_puntero_a_paquete(t_paquete* , void* , uint32_t);
void agregar_a_paquete(t_paquete* , void* , int );
void agregar_bytes_a_paquete(t_paquete* , void* , uint32_t);
void* serializar_paquete(t_paquete* , int );
void free_array (char ** );
t_paquete* recibir_paquete(int );
char* sacar_cadena_de_paquete(void** );
int sacar_entero_de_paquete(void** );
uint32_t sacar_entero_sin_signo_de_paquete(void** );
char** sacar_array_cadenas_de_paquete(void** );
t_list* sacar_lista_de_cadenas_de_paquete(void**);
void* sacar_puntero_de_paquete(void** );
void* sacar_bytes_de_paquete(void** , uint32_t );
void enviar_paquete(t_paquete* , int );
void eliminar_paquete(t_paquete* );

//SERIALIZACION
void* serializar_paquete(t_paquete* , int );

//======================================================= Mensaje ==========================================================================================================
void enviar_mensaje(char*, int);
void recibir_mensaje(int ,t_log*);




#endif
