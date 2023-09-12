#include "kernel.h"

//================================================== Configuracion =====================================================================
void cargar_configuracion(char* path) {

	  config = config_create(path); 

      if (config == NULL) {
          perror("Archivo de configuracion de kernel no encontrado \n");
          abort();
      }

      config_valores_kernel.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
      config_valores_kernel.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
      config_valores_kernel.ip_filesystem = config_get_string_value(config, "IP_FILESYSTEM");
      config_valores_kernel.puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");
      config_valores_kernel.ip_cpu = config_get_string_value(config, "IP_CPU");
      config_valores_kernel.puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
      config_valores_kernel.puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
      config_valores_kernel.ip_kernel = config_get_string_value(config, "IP_KERNEL");
      config_valores_kernel.puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
      config_valores_kernel.algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
      config_valores_kernel.quantum = config_get_int_value(config, "QUANTUM");
      config_valores_kernel.grado_multiprogramacion_ini = config_get_int_value(config, "GRADO_MULTIPROGRAMACION_INI");
      config_valores_kernel.recursos = config_get_array_value(config, "RECURSOS");
      config_valores_kernel.instancias_recursos = config_get_array_value(config, "INSTANCIAS_RECURSOS");
}

//================================================== Manejo de Clientes =====================================================================
int atender_clientes_kernel(int socket_servidor)
{
  //el kernel es un servidor entonces va a recibir pedidos todo el tiempo de cpu filesystem y memoria
  int socket_cliente = esperar_cliente(socket_servidor);
  
  if(socket_cliente != -1) {

    //el kernel tiene que hacer muchas cosas entonces le mandamos un hilo para que se ocupe de atender gente
  	pthread_t hilo_cliente;

    /*tenemos que crear un puntero al socket porque al ser un hilo, necesito que la info del socket este 
    disponible durante todo momento que el hilo lo necesite y no quiero que nadie ni nada modifique mi
    socket_cliente entonces mejor me guardo espacio de memoria para asegurarme que nunca va a modificarse*/
  	int* puntero_socket_cliente = malloc(sizeof(int)); 
  	*puntero_socket_cliente = socket_cliente;

    //despues de que el hilo termine quiero que me borre todo los recursos que uso
  	pthread_create(&hilo_cliente, NULL, (void*) manejar_conexion, puntero_socket_cliente); 
    pthread_detach(hilo_cliente);
  	return 1;

  	} else {
  		log_error(kernel_logger, "Error al escuchar clientes... Finalizando servidor \n");
  	}

  return 0;
}

void manejar_conexion(int socket_cliente) {
  //aca seguro vamos a recibir distintos tipos de paquetes y tenemos que crear funcion para cada tipo
  //return 0;
}

void finalizar_kernel(){
	 log_info(kernel_logger,"Finalizando el modulo Kernel");
	 log_destroy(kernel_logger);
	 liberar_conexion(server_fd);
	 liberar_conexion(socket_memoria);
	 liberar_conexion(socket_cpu_dispatch);
   liberar_conexion(socket_cpu_interrupt);
}
