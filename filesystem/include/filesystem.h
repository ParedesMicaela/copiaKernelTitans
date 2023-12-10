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
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>



extern t_config *config;
extern t_log *filesystem_logger;
extern int socket_memoria;
extern int socket_kernel; 
extern int server_fd;
extern t_list *bloques_reservados;
extern t_list* procesos_en_filesystem;
extern t_list* lista_bloques_swap;
extern int tamanio_swap;
extern int tamanio_fat;
extern int tamanio_archivo_bloques;
extern int espacio_de_FAT;
extern t_bitarray* mapa_bits_swap;
extern t_dictionary* diccionario_archivos_abiertos;


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
    char* nombre_archivo; 
    int tamanio_archivo; 
    int bloque_inicial;
} fcb;

typedef struct 
{
    int pid;
    t_list* bloques_reservados;
} t_proceso_en_filesystem;

typedef struct {
    int nro_pagina;
    int bit_presencia_swap;
    int posicion_swap;
    int pid;
    //int marco;
} bloque_swap; 

typedef struct{
    t_list* bloques_de_swap;
}t_archivo_de_bloques;

extern t_list *tabla_fat;
extern fcb config_valores_fcb;
extern arch_config config_valores_filesystem;
extern t_bitarray* bitmap_archivo_bloques;
extern bloque_swap* particion_swap;
extern FILE* archivo_de_bloques;
extern char* path_archivo_bloques;


void inicializar_bitarray();
void cargar_configuracion(char*);
void*conexion_inicial_memoria();
void atender_clientes_filesystem(void* ); 
FILE* levantar_archivo_bloque();
void levantar_fat(size_t tamanio_fat);
fcb* levantar_fcb (char * path);
void crear_archivo (char *nombre_archivo, int socket_kernel);
void abrir_archivo (char *nombre_archivo, int socket_kernel);
void liberar_bloque_individual(bloque_swap* bloque);
char* devolver_direccion_archivo(char* nombre);

int* buscar_y_rellenar_fcb(char* nombre);

bloque_swap* buscar_pagina(int nro_pagina);

char* concatenarCadenas(const char* str1, const char* str2);
int dividirRedondeando(int numero1 , int numero2);
//..................................FUNCIONES UTILES ARCHIVOS.....................................................................

//int abrirArchivo(char *nombre, char **vectorDePaths,int cantidadPaths);

//int crearArchivo(char *nombre,char *carpeta, char ***vectoreRutas, int *cantidadPaths);

//void truncarArchivo(char *nombre, int tamanioNuevo);
void truncar_archivo(char *nombre, int tamanio_nuevo, int socket_kernel);

void ampliar_tamanio_archivo (int nuevo_tamanio_archivo,fcb* fcb_archivo);

void reducir_tamanio_archivo (int nuevo_tamanio_archivo,fcb* fcb_archivo);

void destruir_entrada_fat(bloque_swap* ultimo_bloque_fat);

t_archivo* buscar_archivo_en_carpeta_fcbs(char* nombre);

void actualizar_fcb(fcb* nuevo_fcb);

int calcular_bloque_inicial_archivo(int tamanio);

void* liberar_bloque(bloque_swap* bloque_a_liberar);

void destruir_entrada_fat(bloque_swap* ultimo_bloque_fat);

//..................................FUNCIONES ARCHIVOS DEL MERGE.....................................................................
void leer_archivo(char *nombre_archivo, uint32_t puntero_archivo, uint32_t direccion_fisica);
void escribir_archivo(char* nombre_archivo, uint32_t puntero_archivo, void* contenido);
void cerrar_archivo(char* nombre_archivo);
void mapear_archivo_de_bloques();

//..................................FUNCIONES SWAP.....................................................................
t_list* reservar_bloques(int pid, int cantidad_bloques);
void liberar_bloques(int pid);
bloque_swap* buscar_bloque_swap(int nro_pagina, int pid);
void bloque_libre_swap (int i);
bloque_swap* asignar_bloque_swap(int tam_bloque, int index, int pid);
void crear_filesystem_swap();
void ocupar_bloque(int i);
void swap_out(int pid, int nro_pagina);
void inicializar_swap();
void crear_archivo_de_bloque();

#endif
