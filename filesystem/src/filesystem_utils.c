#include "filesystem.h"


//mmap trae a memoria
//msink sincroiniza el void* al archivo
//cada que escribir, metes en el archivo con msink

int socket_kernel;
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
	char* modo_apertura = NULL;
    int nuevo_tamanio_archivo = -1;
    uint32_t puntero_archivo = NULL; //antes era FILE*
    uint32_t direccion_fisica; //antes era char*
    char* informacion = NULL;
	int tamanio = 0;
	int pid =-1;
	int pag_pf;
    int bloques_a_reservar = -1;
	bool corriendo = true;
	
	while (corriendo) //hace falta esto???
	{		
		//recibe paquete que le envia el kernel cada que le llega algo de archivos
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
				log_info(filesystem_logger, "Crear Archivo: %s", nombre_archivo);
				crear_archivo(nombre_archivo, cliente_fd); 
				break;

			case TRUNCAR_ARCHIVO:
				nombre_archivo = sacar_cadena_de_paquete(&stream);
				nuevo_tamanio_archivo = sacar_entero_de_paquete(&stream);
				log_info(filesystem_logger, "Truncar Archivo: %s- Tamaño: %d\n", nombre_archivo, nuevo_tamanio_archivo);
				truncar_archivo(nombre_archivo, nuevo_tamanio_archivo);
				break;

			case LEER_ARCHIVO:
				nombre_archivo = sacar_cadena_de_paquete(&stream);
				tamanio = sacar_entero_de_paquete(&stream);
				puntero_archivo = sacar_entero_sin_signo_de_paquete(&stream); 
				direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
				log_info(filesystem_logger, "Leer Archivo: %s - Puntero: %d - Memoria: %d", nombre_archivo, puntero_archivo, direccion_fisica);
				leer_archivo(nombre_archivo, puntero_archivo, direccion_fisica); 
			break;

			case ESCRIBIR_ARCHIVO:
				//nombre_archivo = sacar_cadena_de_paquete(&stream); idem leer, seguro haya que agregar el nombre, sino cómo sé el archivo que estoy agarrando 			
				direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
				puntero_archivo = sacar_entero_sin_signo_de_paquete(&stream);
				log_info(filesystem_logger, "Escribir Archivo: %s - Puntero: %p - Memoria: %s ", nombre_archivo, (void*)puntero_archivo, direccion_fisica);
					/* 
				t_paquete* paquete = crear_paquete(LEER_INFORMACION);
				agregar_cadena_a_paquete(paquete, direccion_fisica);
				agregar_puntero_a_paquete(paquete, puntero_archivo);
				enviar_paquete(paquete, socket_memoria);
				eliminar_paquete(paquete);
			*/ 

				//despues de enviar el paquete voy a recibir el paquete de respuesta de memoria con el contenido		
				//contenido/informacion_a_escribir = sacar_cadena_de_paquete(&stream); -> después agregarlo 
				//direccion_fisica = sacar_cadena_de_paquete(&stream);
				//puntero_archivo = sacar_puntero_de_paquete(&stream);
				void *contenido; // esto después vuela cuando agreguemos el contenido/info a escribir ->  línea 108
				escribir_archivo(nombre_archivo,puntero_archivo,contenido,direccion_fisica);
				//como pingo es escribir
				//como un log
			break;

		case INICIALIZAR_SWAP:		
				bloques_reservados_a_enviar = list_create();
				pid = sacar_entero_de_paquete(&stream);
				bloques_a_reservar = sacar_entero_de_paquete(&stream);
				bloques_reservados_a_enviar = reservar_bloques(pid,bloques_a_reservar); 
				if(bloques_reservados_a_enviar != NULL) {
					enviar_bloques_reservados(bloques_reservados_a_enviar);
				}
				else{
					log_info(filesystem_logger,"No se pudieron reservar los bloques");
				}
			break;

			case LIBERAR_SWAP:
				//bloques_a_liberar = sacar_lista_de_cadenas_de_paquete(&stream); 
				pid = sacar_entero_de_paquete(&stream);
				liberar_bloques(pid);
			break;

			case PEDIR_PAGINA_PARA_ESCRITURA:
				pid = sacar_entero_de_paquete(&stream);
				pag_pf = sacar_entero_de_paquete(&stream);
				//t_pagina* pagina = buscar_pagina(pag_pf); 
			break;

			case ESCRIBIR_PAGINA_SWAP:
				//pag_a_escribir = sacar_pagina_de_paquete(&stream);
				//escribir_pag(pag_a_escribir);
				enviar_pagina_para_escritura();
			break;

			default:
				printf("Operacion desconocida \n");
				abort();
			break;
		}
		eliminar_paquete(paquete);
	}
}

