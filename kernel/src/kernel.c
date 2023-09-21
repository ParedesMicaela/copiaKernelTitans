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

//========================================================================================================================================
int main(void)
{
	kernel_logger = log_create("/home/utnso/tp-2023-2c-KernelTitans/kernel/cfg/kernel.log", "kernel.log", 1, LOG_LEVEL_INFO);

	cargar_configuracion("/home/utnso/tp-2023-2c-KernelTitans/kernel/cfg/kernel.config");

	log_info(kernel_logger, "Archivo de configuracion cargado \n");

    inicializar_planificador();
    
    //conexion con CPU
    socket_cpu_dispatch = crear_conexion(config_valores_kernel.ip_cpu, config_valores_kernel.puerto_cpu_dispatch);
	socket_cpu_interrupt = crear_conexion(config_valores_kernel.ip_cpu, config_valores_kernel.puerto_cpu_interrupt);

    log_info(kernel_logger, "Kernel conectado con cpu \n");

    //conexion con memoria
    socket_memoria = crear_conexion(config_valores_kernel.ip_memoria, config_valores_kernel.puerto_memoria);

    log_info(kernel_logger, "Kernel conectado con memoria \n");

   //conexion con filesystem
   socket_filesystem = crear_conexion(config_valores_kernel.ip_filesystem, config_valores_kernel.puerto_filesystem);

   log_info(kernel_logger, "Kernel conectado con filesystem \n");

   //iniciamos el servidor
   server_fd = iniciar_servidor(config_valores_kernel.ip_kernel,config_valores_kernel.puerto_escucha);

   log_info(kernel_logger, "Servidor creado \n");

   log_info(kernel_logger, "Kernel listo para recibir al modulo cliente \n");
   
   //inicializar_consola_interactiva();

   iniciar_proceso("/home/utnso/path.txt", 16, 1);   

   void finalizar_kernel();

   return EXIT_SUCCESS;
}
