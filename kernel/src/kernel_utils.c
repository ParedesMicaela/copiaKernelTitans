#include "kernel.h"
//..................................CONFIGURACIONES.......................................................................

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

//---------------------------------------MANEJO CLIENTES - CONEXIONES -----------------------
int atender_clientes_kernel(int socket_servidor){
  int socket_cliente = esperar_cliente(socket_servidor);
  // TODO
  return 0;
}

void manejar_conexion(int socket_cliente) {
  //TODO
  //return 0;
}
