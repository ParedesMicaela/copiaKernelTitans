#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <pthread.h>
#include "socket.h"
#include "logconfig.h"
#include "operaciones.h"

extern t_config *config;
extern t_log *filesystem_logger;
extern int socket_memoria;
extern int socket_kernel; 
extern int server_fd;
extern t_list *bloques_reservados;
extern t_list* procesos_en_filesystem;
extern t_list* lista_bloques_swap;
extern t_list* lista_bloques_fat;

extern size_t tamanio_fat;

//STRUCTS//
typedef struct  
 {
	char* ip_filesystem;
	char* puerto_filesystem;
    char* ip_memoria;
    char* puerto_memoria;
    char* puerto_escucha;
    char* path_fat;
    char* path_bloques;
    char* path_fcb;
    int cant_bloques_total;
    int cant_bloques_swap;
    int tam_bloque;
    int retardo_acceso_bloque;
    int retardo_acceso_fat;

} arch_config;

typedef struct 
{
    char* nombre_archivo; //Puede ser  char nombre_archivo[256]
    int tamanio_archivo; //Puede ser size_t uint32_t
    int bloque_inicial;
} fcb;

typedef struct 
{
    int pid;
    t_list* bloques_reservados;
} t_proceso_en_filesystem;

// Define la estructura de un bloque de swap

typedef struct {
    char* data;  
} bloque_swap; //DIego por el momento usa esto


typedef struct {
    //char* nombre_archivo; //esto al final ni lo usamos
    int numero_bloque;
    uint32_t* direccion_bloque;
    uint32_t* puntero_siguiente_bloque;
    uint8_t data;
} t_bloque; // esto es un bloque..xd

extern t_list *tabla_fat;

extern fcb config_valores_fcb;

extern arch_config config_valores_filesystem;

void cargar_configuracion(char*);
void*conexion_inicial_memoria();
void atender_clientes_filesystem(void* ); 
void levantar_archivo_bloque(size_t tamanio_swap, size_t tamanio_fat);
void levantar_fat(size_t tamanio_fat);
void levantar_fcb(char* path);
void crear_archivo (char *nombre_archivo, int socket_kernel);
void abrir_archivo (char *nombre_archivo, int socket_kernel);
void liberar_bloque_individual(bloque_swap* bloque);
char* devolver_direccion_archivo(char* nombre);

int* buscar_y_rellenar_fcb(char* nombre);

char* concatenarCadenas(const char* str1, const char* str2);
int dividirRedondeando(int numero1 , int numero2);
//..................................FUNCIONES UTILES ARCHIVOS.....................................................................

//int abrirArchivo(char *nombre, char **vectorDePaths,int cantidadPaths);

//int crearArchivo(char *nombre,char *carpeta, char ***vectoreRutas, int *cantidadPaths);

//void truncarArchivo(char *nombre, int tamanioNuevo);
void truncar_archivo(char *nombre, int tamanio_nuevo);

void ampliar_tamanio_archivo (int nuevo_tamanio_archivo,fcb fcb_archivo);

void reducir_tamanio_archivo (int nuevo_tamanio_archivo,fcb fcb_archivo);

void destruir_entrada_fat(t_bloque* ultimo_bloque_fat);

t_archivo* buscar_archivo_en_carpeta_fcbs(char* nombre);

void* liberar_bloque(t_bloque* bloque_a_liberar);

void destruir_entrada_fat(t_bloque* ultimo_bloque_fat);

//..................................FUNCIONES ARCHIVOS DEL MERGE.....................................................................

//fcb *abrir_archivo (char *nombre_archivo);

//antes era char* e int*
void *leer_archivo(char *nombre_archivo, uint32_t direccion_fisica, int socket_kernel);
void escribir_archivo(char* nombre_archivo, uint32_t *puntero_archivo, void* contenido, uint32_t*  direccion_fisica);

//..................................FUNCIONES SWAP.....................................................................
t_list* reservar_bloques(int pid, int cantidad_bloques);
void liberar_bloques(int pid);
bloque_swap* crear_bloque_swap(int tam_bloque);
#endif
