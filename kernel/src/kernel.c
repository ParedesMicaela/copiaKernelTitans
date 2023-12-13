#include "kernel.h"

//================================================= Variables Globales =====================================================================
t_log* kernel_logger;
t_config* config;
int server_fd;
int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_memoria;
int socket_filesystem;
arch_config_kernel config_valores_kernel;
pthread_t consola;
pthread_t largo_plazo;
pthread_t corto_plazo;
//pthread_mutex_t readline_mutex;

//========================================================================================================================================
int main(void)
{
	kernel_logger = log_create("/home/utnso/tp-2023-2c-KernelTitans/kernel/cfg/kernel.log", "kernel.log", 1, LOG_LEVEL_INFO);

	cargar_configuracion("/home/utnso/tp-2023-2c-KernelTitans/kernel/cfg/kernel.config");
	tamanio_recursos = string_array_size(config_valores_kernel.recursos);
    
    //conexion con CPU
    socket_cpu_dispatch = crear_conexion(config_valores_kernel.ip_cpu, config_valores_kernel.puerto_cpu_dispatch);
	socket_cpu_interrupt = crear_conexion(config_valores_kernel.ip_cpu, config_valores_kernel.puerto_cpu_interrupt);

    //conexion con memoria
    socket_memoria = crear_conexion(config_valores_kernel.ip_memoria, config_valores_kernel.puerto_memoria);

    //conexion con filesystem
    socket_filesystem = crear_conexion(config_valores_kernel.ip_filesystem, config_valores_kernel.puerto_filesystem);

    //iniciamos el servidor
    server_fd = iniciar_servidor(config_valores_kernel.ip_kernel,config_valores_kernel.puerto_escucha);

    iniciar_tabla_archivos_abiertos();

    inicializar_planificador();
    
    pthread_create(&largo_plazo, NULL, (void* ) planificador_largo_plazo, NULL);
    pthread_create(&corto_plazo, NULL, (void* ) planificador_corto_plazo, NULL);
    pthread_create(&consola, NULL, consola_interactiva, NULL);
   
    using_history(); 

    pthread_join(largo_plazo,NULL); 
    pthread_join(corto_plazo,NULL);
    pthread_join(consola, NULL);

    return EXIT_SUCCESS;
}

