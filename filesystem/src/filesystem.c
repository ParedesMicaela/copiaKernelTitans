#include "filesystem.h"

//Variables Globales//
t_log* filesystem_logger;
t_config* config;
int server_fd;
int* cliente_fd;
int socket_memoria;
arch_config config_valores_filesystem;
int tamanio_fat;
int tamanio_swap;
int tamanio_archivo_bloques;
int espacio_de_FAT;
t_bitarray* bitmap_archivo_bloques;
t_dictionary* diccionario_archivos_abiertos;
char* path_archivo_bloques;
sem_t escritura_completada;
sem_t lectura_completada;


//============================================================================================================

int main(void)
{
	filesystem_logger = log_create("/home/utnso/tp-2023-2c-KernelTitans/filesystem/cfg/filesystem.log", "filesystem.log", 1, LOG_LEVEL_INFO);

	cargar_configuracion("/home/utnso/tp-2023-2c-KernelTitans/filesystem/cfg/filesystem.config");

    // COMUNICACIÓN MEMORIA //
	socket_memoria = crear_conexion(config_valores_filesystem.ip_memoria, config_valores_filesystem.puerto_memoria);

    /// CREA LA CONEXION CON KERNEL Y MEMORIA ///
    int server_fd = iniciar_servidor(config_valores_filesystem.ip_filesystem,config_valores_filesystem.puerto_escucha);

    tamanio_fat = (config_valores_filesystem.cant_bloques_total - config_valores_filesystem.cant_bloques_swap) * sizeof(uint32_t);
    tam_bloque = config_valores_filesystem.tam_bloque;
    tamanio_swap = config_valores_filesystem.cant_bloques_swap * config_valores_filesystem.tam_bloque;
    tamanio_archivo_bloques = config_valores_filesystem.cant_bloques_total * config_valores_filesystem.tam_bloque;
    espacio_de_FAT = tamanio_archivo_bloques - tamanio_swap;
    path_archivo_bloques = config_valores_filesystem.path_bloques;
    

    procesos_en_filesystem = list_create();

    diccionario_archivos_abiertos = dictionary_create();

    sem_init(&(escritura_completada), 0 ,0);
    sem_init(&(lectura_completada), 0 ,0);

    crear_fat();
    crear_archivo_de_bloque();
    inicializar_swap();
    

    while(1) 
    {
        int* cliente_fd = malloc(sizeof(int));
        *cliente_fd = esperar_cliente(server_fd);
        pthread_t multihilo;
	    pthread_create(&multihilo,NULL,(void*) atender_clientes_filesystem, cliente_fd);
	    pthread_detach(multihilo);
    }    

    return EXIT_SUCCESS;
}

