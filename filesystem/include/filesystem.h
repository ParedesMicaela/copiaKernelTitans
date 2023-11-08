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
extern int server_fd;
extern t_list *bloques_reservados;

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
    uint32_t bloque_inicial;
} fcb;

// Define la estructura de un bloque de swap

typedef struct {
    uint8_t* data;  //tam bloque es de 64, usamos uint8_t para todo el archivo
} bloque_swap;

extern fcb config_valores_fcb;

extern arch_config config_valores_filesystem;

void cargar_configuracion(char*);
void*conexion_inicial_memoria();
void atender_clientes_filesystem(void* ); 
void levantar_archivo_bloque(size_t tamanio_swap, size_t tamanio_fat);
void levantar_fat(size_t tamanio_fat);
void levantar_fcb(char* path);
int crear_archivo (char *nombre_archivo);
fcb *abrir_archivo (char *nombre_archivo);
t_list* reservar_bloques(int cantidad_bloques);
void liberar_bloques(t_list* bloques_a_liberar);
void liberar_bloque_individual(bloque_swap* bloque);
bloque_swap* crear_bloque_swap(int tam_bloque);

char* concatenarCadenas(const char* str1, const char* str2);
int dividirRedondeando(int numero1 , int numero2);

//..................................FUNCIONES UTILES ARCHIVOS.....................................................................

int abrirArchivo(char *nombre, char **vectorDePaths,int cantidadPaths);

int crearArchivo(char *nombre,char *carpeta, char ***vectoreRutas, int *cantidadPaths);

int truncarArchivo(char *nombre,char *carpeta, char **vectoreRutas, int cantidadPaths, int tamanioNuevo);

//..................................FUNCIONES ARCHIVOS DEL MERGE.....................................................................

//fcb *abrir_archivo (char *nombre_archivo);
char *leer_archivo(char *nombre_archivo, FILE *puntero_archivo, char *direccion_fisica);
int escribirArchivo(char *nombreArchivo, FILE *puntero_archivo, size_t bytesAEscribir, void *memoriaAEscribir);

#endif
