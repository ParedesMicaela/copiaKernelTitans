#include "cpu_utils.h"

pthread_t hilo_interrupt;

//cuando toco una tecla finaliza
void sighandler(int s) {
	finalizar_cpu();
	exit(0);
}

int main()
{
	signal(SIGINT, sighandler);

	//creamos el logger
	cpu_logger = log_create("/home/utnso/tp-2023-2c-KernelTitans/cpu/cpu.log", "cpu.log", 1, LOG_LEVEL_INFO);

	//cargamos la config
	cargar_configuracion("/home/utnso/tp-2023-2c-KernelTitans/cpu/Default/cpu.config");
	log_info(cpu_logger, "Archivo de configuracion cargado \n");

	//creamos conexion con memoria
	int socket_cliente_memoria = crear_conexion(config_valores_cpu.ip_memoria, config_valores_cpu.puerto_memoria);
	realizar_handshake(socket_cliente_memoria);

	//creamos un hilo para la conexion a memoria para memoria que pueda atender a varios clientes al mismo tiempo y sea mas rapido
	pthread_t conexion_memoria_i;

	//vamos a reservar memoria dinamica para guardar los datos de la conexion
	conexion_t* datos=malloc(sizeof(conexion_t));
			if (datos == NULL) {
				   perror("No se pudo asignar memoria \n");
				   exit(EXIT_FAILURE);
			}

	datos->ip=config_valores_cpu->ip_memoria;
	datos->puerto=config_valores_cpu->puerto_memoria;

	//iniciamos hilo de conexion con memoria
	pthread_create(&conexion_memoria_i,NULL,conexion_inicial_memoria,NULL);
		void* thread_result;
		pthread_join(conexion_memoria_i, &thread_result);

		if (thread_result == (void*)(EXIT_SUCCESS)) {
			log_info(cpu_logger, "Se a completado el Handshake correctamente\n");
		} else {
			log_error(cpu_logger, "Ha habido un error en el handshake \n");
			exit(EXIT_FAILURE);
		}
		 log_info(cpu_logger, "CPU se ha conectado con memoria \n");

    //creamos la conexion con el kernel
	///kernel se conecta por dispatch e interrupt
	int socket_servidor_dispatch = iniciar_servidor(config_valores_cpu.cpu,config_valores_cpu.puerto_escucha_dispatch);
    int socket_servidor_interrupt = iniciar_servidor(config_valores_cpu.cpu,config_valores_cpu.puerto_escucha_interrupt);
    log_info(cpu_logger, "Servidores iniciados en CPU \n");


	//esperamos los 2 clientes de kernel
    int socket_cliente_dispatch = esperar_cliente(socket_servidor_dispatch);
    int socket_cliente_interrupt = esperar_cliente(socket_servidor_interrupt);
	printf("Se conectaron los clientes de dispatch e interrupt\n");

	//creamos el hilo para atender las interrupciones
    pthread_create(&hilo_interrupt, NULL, (void*)atender_interrupt, &socket_cliente_interrupt);
    pthread_detach(hilo_interrupt);

    while(1)
     {
         atender_dispatch(socket_cliente_dispatch, socket_cliente_memoria);
     }


    //liberamos la memoria dinamica
    log_warning(cpu_logger, "Apagandose cpu");
    free(datos);
    return 0;
}
