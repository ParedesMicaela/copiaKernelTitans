#include "cpu_utils.h"

pthread_t hilo_interrupt;
pthread_t cpu;
int socket_cliente_memoria;
int socket_servidor_dispatch;
int socket_servidor_interrupt;
int socket_cliente_dispatch;
int socket_cliente_interrupt;
sem_t revision_interrupt;
sem_t checkeo_completado;
sem_t check_finalizado;
sem_t check_interrupt;
sem_t inicia_ciclo;

int main(void)
{
    cpu_logger = log_create("/home/utnso/tp-2023-2c-KernelTitans/cpu/cfg/cpu.log", "cpu.log", 1, LOG_LEVEL_INFO);

    cargar_configuracion("/home/utnso/tp-2023-2c-KernelTitans/cpu/cfg/cpu.config");

    socket_cliente_memoria = crear_conexion(config_valores_cpu.ip_memoria, config_valores_cpu.puerto_memoria);
    realizar_handshake(socket_cliente_memoria);

    socket_servidor_dispatch = iniciar_servidor(config_valores_cpu.ip_cpu, config_valores_cpu.puerto_escucha_dispatch);
    socket_servidor_interrupt = iniciar_servidor(config_valores_cpu.ip_cpu, config_valores_cpu.puerto_escucha_interrupt);

    socket_cliente_dispatch = esperar_cliente(socket_servidor_dispatch);
    socket_cliente_interrupt = esperar_cliente(socket_servidor_interrupt);

    sem_init(&(revision_interrupt), 0, 0);
    sem_init(&(checkeo_completado), 0, 1);
    sem_init(&(check_interrupt), 0, 0);
    sem_init(&(check_finalizado), 0, 0);
    sem_init(&(inicia_ciclo), 0, 1);

    pthread_create(&cpu, NULL, (void* ) atender_dispatch, NULL);
    pthread_join(cpu, NULL);

    pthread_create(&hilo_interrupt, NULL, (void *)atender_interrupt, NULL);
    pthread_join(hilo_interrupt, NULL);

    return 0;
}
