#include "cpu_utils.h"

//======================== Variables Globales ============
pthread_mutex_t mutex_interrupcion;
int interrupcion;

//======================= Funciones Globales ==============

//================================================== Configuracion =====================================================================
void cargar_configuracion(char* path){
    config = config_create(path); 

    if (config == NULL) {
        perror("Archivo de configuracion de cpu no encontrado \n");
        abort();
    }

	config_valores_cpu.ip_cpu=config_get_string_value(config,"IP_CPU");
	config_valores_cpu.ip_memoria=config_get_string_value(config,"IP_MEMORIA");
	config_valores_cpu.puerto_memoria=config_get_string_value(config,"PUERTO_MEMORIA");
	config_valores_cpu.puerto_escucha_dispatch=config_get_string_value(config,"PUERTO_ESCUCHA_DISPATCH");
	config_valores_cpu.puerto_escucha_interrupt=config_get_string_value(config,"PUERTO_ESCUCHA_INTERRUPT");

    log_info(cpu_logger, "Configuracion de CPU cargada");
}

//================================================== Handshake memoria =====================================================================
void* conexion_inicial_memoria(void* arg){

	//hacemos toda la cosa para pedir el handshake
	pedir_handshake(socket_memoria);
	log_info(cpu_logger, "Pedido de handshake enviado a memoria\n");
	int codigo_memoria;

	while(1){
		codigo_memoria=recibir_operacion_nuevo(socket_memoria);
		switch(codigo_memoria){
			case PAQUETE:
				log_info(cpu_logger,"Recibi configuracion por handshake \n");
				configuracion_segmento=recibir_handshake(socket_memoria);
				return NULL;
			break;
			case -1:
				log_error(cpu_logger, "Fallo la comunicacion. Abortando \n");
				 //free(configuracion_segmento);
				return (void*)(EXIT_FAILURE);
			break;
			default:
				 log_warning(cpu_logger, "Operacion desconocida \n");
				 //free(configuracion_segmento);
			break;
		}
	}
	 return (void*)(EXIT_SUCCESS);
}

//================================================== Dispatch =====================================================================

//por aca el kernel nos va a mandar el pcb y es el canal importante donde recibimos y mandamos el contexto ejecucion
void atender_dispatch(int socket_cliente_dispatch, int socket_cliente_memoria)
{
    log_info(cpu_logger, "Espero recibir paquete");
    t_paquete* paquete = recibir_paquete(socket_cliente_dispatch);
    void* stream = paquete->buffer->stream;
    log_info(cpu_logger, "Ya recibi paquete");

    t_contexto_ejecucion* contexto_ejecucion = malloc(sizeof(t_contexto_ejecucion));
	if(paquete->codigo_operacion == PCB)
	{
        contexto_ejecucion->pid = sacar_entero_de_paquete(&stream);
        contexto_ejecucion->program_counter = sacar_entero_de_paquete(&stream);
        contexto_ejecucion->prioridad = sacar_entero_de_paquete(&stream);
        contexto_ejecucion->registros = sacar_array_cadenas_de_paquete(&stream);

        log_info(cpu_logger, "Recibi un PCB del Kernel");

        //despues hay que ver estas funciones
        //iniciar_registros(contexto_ejecucion->registros_cpu);
        //ciclo_de_instruccion(contexto_ejecucion->socket_kernel);
    }
    else
    {
        sleep(3);
		perror("No se recibio correctamente el PCB");
    }
}

//================================================== Interrupt =====================================================================

//este canal se va a usar para mensajes de interrupcion
void atender_interrupt(void* cliente)
{
    int conexion = *(int*)cliente;

    while (1)
    {
        t_paquete* paquete = recibir_paquete(conexion);
        log_info(cpu_logger, "Recibi un aviso por interrupt del kernel");
        void* stream = paquete->buffer->stream;

        if(paquete->codigo_operacion == DESALOJO)
        {
        	//esto despues lo usamos en el check interrupt para saber que si hay una interrupcion, devolvemos el contexto de ejecucion
            int entero = sacar_entero_de_paquete(&stream);

            //lo tenemos que poner en un semaforo para que nada lo modifique mientras le sumo 1, cosa que despues no se pierda que
            //efectivamente hubo un interrupcion
            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion +=1;
            pthread_mutex_unlock(&mutex_interrupcion);
        }
        else{
            printf("No recibi una interrupcion\n");
        }
        }
}

