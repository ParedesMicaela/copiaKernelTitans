#include "cpu_utils.h"

pthread_t hilo_interrupt;
int socket_cliente_memoria;
int socket_servidor_dispatch;
int socket_servidor_interrupt;
int socket_cliente_dispatch;
int socket_cliente_interrupt;

int main(void)
{
    // creamos el logger
    cpu_logger = log_create("/home/utnso/tp-2023-2c-KernelTitans/cpu/cfg/cpu.log", "cpu.log", 1, LOG_LEVEL_INFO);

    // cargamos la config
    cargar_configuracion("/home/utnso/tp-2023-2c-KernelTitans/cpu/cfg/cpu.config");
    log_info(cpu_logger, "Archivo de configuracion cargado \n");

    // creamos conexion con memoria
    int socket_cliente_memoria = crear_conexion(config_valores_cpu.ip_memoria, config_valores_cpu.puerto_memoria);
    log_info(cpu_logger, "Conexion con memoria realizada \n");
    realizar_handshake(socket_cliente_memoria);

    // creamos la conexion con el kernel
    /// kernel se conecta por dispatch e interrupt
    int socket_servidor_dispatch = iniciar_servidor(config_valores_cpu.ip_cpu, config_valores_cpu.puerto_escucha_dispatch);
    int socket_servidor_interrupt = iniciar_servidor(config_valores_cpu.ip_cpu, config_valores_cpu.puerto_escucha_interrupt);
    log_info(cpu_logger, "Servidores iniciados en CPU \n");

    // esperamos los 2 clientes de kernel
    int socket_cliente_dispatch = esperar_cliente(socket_servidor_dispatch);
    int socket_cliente_interrupt = esperar_cliente(socket_servidor_interrupt);
    printf("Se conectaron los clientes de dispatch e interrupt\n");

    // creamos el hilo para atender las interrupciones
    pthread_create(&hilo_interrupt, NULL, (void *)atender_interrupt, &socket_cliente_interrupt);
    pthread_detach(hilo_interrupt);

    while (1)
    {
        // vamos a atender a nuestro cliente el kernel por aca para hacer todo MENOS las interrupciones
        atender_dispatch(socket_cliente_dispatch, socket_cliente_memoria);
    }

    // liberamos la memoria dinamica
    log_warning(cpu_logger, "Apagandose cpu");
    return 0;
}
