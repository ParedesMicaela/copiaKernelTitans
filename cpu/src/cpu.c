#include "cpu_utils.h"

pthread_t hilo_interrupt;
int socket_cliente_memoria;
int socket_servidor_dispatch;
int socket_servidor_interrupt;
int socket_cliente_dispatch;
int socket_cliente_interrupt;

int main(void)
{
    cpu_logger = log_create("/home/utnso/tp-2023-2c-KernelTitans/cpu/cfg/cpu.log", "cpu.log", 1, LOG_LEVEL_INFO);

    cargar_configuracion("/home/utnso/tp-2023-2c-KernelTitans/cpu/cfg/cpu.config");

    socket_cliente_memoria = crear_conexion(config_valores_cpu.ip_memoria, config_valores_cpu.puerto_memoria);
    realizar_handshake(socket_cliente_memoria);

    int socket_servidor_dispatch = iniciar_servidor(config_valores_cpu.ip_cpu, config_valores_cpu.puerto_escucha_dispatch);
    int socket_servidor_interrupt = iniciar_servidor(config_valores_cpu.ip_cpu, config_valores_cpu.puerto_escucha_interrupt);

    int socket_cliente_dispatch = esperar_cliente(socket_servidor_dispatch);
    int socket_cliente_interrupt = esperar_cliente(socket_servidor_interrupt);

    pthread_create(&hilo_interrupt, NULL, (void *)atender_interrupt, &socket_cliente_interrupt);
    pthread_detach(hilo_interrupt);

    while (1)
    {
        atender_dispatch(socket_cliente_dispatch);
    }

    return 0;
}
