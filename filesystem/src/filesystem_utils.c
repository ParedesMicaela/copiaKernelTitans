#include "filesystem.h"

fcb config_valores_fcb;
t_list* bloques_reservados_a_enviar;
t_list *tabla_fat; //lista con direcciones
t_list *lista_bloques_swap;
t_list *lista_bloques_fat;
FILE* archivo_fat;

//=======================================================================================================================

void cargar_configuracion(char* path) {

       config = config_create(path); //Leo el archivo de configuracion

      if (config == NULL) {
          perror("Archivo de configuracion de filesystem no encontrado \n");
          abort();
      }

      config_valores_filesystem.ip_filesystem = config_get_string_value(config, "IP_FILESYSTEM");
      config_valores_filesystem.puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");
      config_valores_filesystem.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
      config_valores_filesystem.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
      config_valores_filesystem.puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
      config_valores_filesystem.path_fat = config_get_string_value(config, "PATH_FAT");
      config_valores_filesystem.path_bloques = config_get_string_value(config, "PATH_BLOQUES");
      config_valores_filesystem.path_fcb = config_get_string_value(config, "PATH_FCB");
      config_valores_filesystem.cant_bloques_total = config_get_int_value(config, "CANT_BLOQUES_TOTAL");
      config_valores_filesystem.cant_bloques_swap = config_get_int_value(config, "CANT_BLOQUES_SWAP");
      config_valores_filesystem.tam_bloque = config_get_int_value(config, "TAM_BLOQUE");
      config_valores_filesystem.retardo_acceso_bloque = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");
      config_valores_filesystem.retardo_acceso_fat = config_get_int_value(config, "RETARDO_ACCESO_FAT");

}

void atender_clientes_filesystem(void* conexion) {
    int cliente_fd = *(int*)conexion;
    char* nombre_archivo = NULL;
    int nuevo_tamanio_archivo = -1;
    uint32_t puntero_archivo = 0; 
    uint32_t direccion_fisica; 
	int tamanio = 0;
	int pid =-1;
	int nro_pag;
    int bloques_a_reservar = -1;
	void* contenido_a_escrbir = NULL;
	int tam_bloque = config_valores_filesystem.tam_bloque;
	int posicion_swap;
	
	while (1) 
	{		
		t_paquete* paquete = recibir_paquete(cliente_fd);
		void* stream = paquete->buffer->stream;

		switch(paquete->codigo_operacion)
		{
			case ABRIR_ARCHIVO:
				nombre_archivo = sacar_cadena_de_paquete(&stream);
				log_info(filesystem_logger, "Abrir Archivo: %s", nombre_archivo);
				abrir_archivo(nombre_archivo, cliente_fd);
				break;

			case CREAR_ARCHIVO:
				nombre_archivo = sacar_cadena_de_paquete(&stream);
				crear_archivo(nombre_archivo, cliente_fd); 
				break;

			case TRUNCAR_ARCHIVO:
				nombre_archivo = sacar_cadena_de_paquete(&stream);
				nuevo_tamanio_archivo = sacar_entero_de_paquete(&stream);
				log_info(filesystem_logger, "Truncar Archivo: %s - Tamaño: %d\n", nombre_archivo, nuevo_tamanio_archivo);
				truncar_archivo(nombre_archivo, nuevo_tamanio_archivo, cliente_fd);
				break;

			case LEER_ARCHIVO:
				nombre_archivo = sacar_cadena_de_paquete(&stream);
				tamanio = sacar_entero_de_paquete(&stream);
				puntero_archivo = sacar_entero_sin_signo_de_paquete(&stream); 
				direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
				log_info(filesystem_logger, "Leer Archivo: %s - Puntero: %d - Memoria: %d", nombre_archivo, puntero_archivo, direccion_fisica);
				leer_archivo(nombre_archivo, puntero_archivo, direccion_fisica); 

				sem_wait(&lectura_completada);
				//Avisa al kernel que terminó
				int ok_read = 1;
				send(cliente_fd, &ok_read, sizeof(int), 0);
			break;

			case ESCRITURA_EN_MEMORIA_CONFIRMADA:
				int numero = sacar_entero_de_paquete(&stream);
				sem_post(&lectura_completada);
			break;
			
			case SOLICITAR_INFO_ARCHIVO_MEMORIA:
				nombre_archivo = sacar_cadena_de_paquete(&stream);
				tamanio = sacar_entero_de_paquete(&stream); 			
				puntero_archivo = sacar_entero_sin_signo_de_paquete(&stream);
				direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
				log_info(filesystem_logger, "Escribir Archivo: %s - Puntero: %d - Memoria: %d ", nombre_archivo, puntero_archivo, direccion_fisica);
				solicitar_informacion_memoria(direccion_fisica, tam_bloque, nombre_archivo, puntero_archivo);
				sem_wait(&escritura_completada);

				int escribir_ok = 1;
				send(cliente_fd, &escribir_ok, sizeof(int), 0);

			break;

			case ESCRIBIR_EN_ARCHIVO_BLOQUES:
				contenido_a_escrbir = sacar_bytes_de_paquete(&stream, tam_bloque); 			
				puntero_archivo = sacar_entero_sin_signo_de_paquete(&stream);
				nombre_archivo = sacar_cadena_de_paquete(&stream); 
				escribir_archivo(nombre_archivo,puntero_archivo,contenido_a_escrbir);
				sem_post(&escritura_completada);	

			break;

			case INICIALIZAR_SWAP:		
				pid = sacar_entero_de_paquete(&stream);
				bloques_a_reservar = sacar_entero_de_paquete(&stream);
				bloques_reservados_a_enviar = reservar_bloques(pid,bloques_a_reservar); 
				if(bloques_reservados_a_enviar != NULL) {
					enviar_bloques_reservados(bloques_reservados_a_enviar, pid);
				}
				else{
					log_info(filesystem_logger,"No se pudieron reservar los bloques");
				}
			break;

			case CERRAR_ARCHIVO:
				nombre_archivo = sacar_cadena_de_paquete(&stream);
				log_info(filesystem_logger, "Cerrar Archivo: %s", nombre_archivo);
				cerrar_archivo(nombre_archivo); 
				break;

			case LIBERAR_SWAP:
				pid = sacar_entero_de_paquete(&stream);
    			t_proceso_en_filesystem* proceso = buscar_proceso_en_filesystem(pid);
				t_list* lista_aux;
				lista_aux = sacar_lista_de_cadenas_de_paquete(&stream);
				list_add_all(proceso->bloques_reservados, lista_aux);
				list_destroy(lista_aux);

				liberar_bloques(proceso->bloques_reservados, proceso);
			break;

			case PAGINA_SWAP_OUT:
				pid = sacar_entero_de_paquete(&stream);
				nro_pag = sacar_entero_de_paquete(&stream);
				swap_out(pid, nro_pag); 
			break;

			case PAGINA_SWAP_IN:
				pid = sacar_entero_de_paquete(&stream);
				nro_pag = sacar_entero_de_paquete(&stream);
				posicion_swap = sacar_entero_de_paquete(&stream);
				//marco = sacar_entero_de_paquete(&stream);
				swap_in(pid, nro_pag,posicion_swap);
			break;

			default:
				//printf("Operacion desconocida \n");
				//abort();
			break;
		}
		eliminar_paquete(paquete);
	}
}

