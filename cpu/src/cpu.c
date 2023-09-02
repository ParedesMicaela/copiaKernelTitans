#include "cpu_util.h"

int main(void)
{
/// LOG ///
	cpu_logger = log_create("/home/utnso/tp-2023-2c-KernelTitans/cpu", "cpu.log", 1, LOG_LEVEL_INFO);

/// CARGA LA CONFIGURACION DE LA CPU ///

	//cargar_configuracion("/home/utnso/tp-2023-1c-Desayuno_Pesado/cpu/Default/cpu.config");
	//log_info(cpu_logger, "Archivo de configuracion cargado \n");

/// CREAR CONEXION CON MEMORIA ///

	socket_memoria = crear_conexion(config_valores_cpu.ip_memoria, config_valores_cpu.puerto_memoria);
	log_info(cpu_logger, "CPU se ha conectado con memoria \n");

/// CREA LA CONEXION CON EL KERNEL ///

	int socket_server = iniciar_servidor(config_valores_cpu.ip_cpu,config_valores_cpu.puerto_escucha);
    log_info(cpu_logger, "CPU listo para recibir al modulo cliente \n");
    int socket_cliente = esperar_cliente(socket_server);
    log_info(cpu_logger,"Se conecto un cliente\n");

    return 0;
}
