#include "filesystem.h"

bloque_swap* particion_swap;
char* swap_mapeado;
void* fat_mapeado;
uint32_t* tabla_fat_en_memoria;
//============================================== INICIALIZACION ============================================

void levantar_fat(size_t tamanio_fat)
{
    char *path = config_valores_filesystem.path_fat;
    uint32_t tam_entrada = tamanio_fat / sizeof(uint32_t);
   	uint32_t valor_inicial = 0;

    FILE *archivo_fat = fopen(path, "rb+");
    if (archivo_fat == NULL)
    {
        // Si no existe el archivo FAT, créalo e inicialízalo con 0
        archivo_fat = fopen(path, "wb+");
        if (archivo_fat != NULL)
        {
            
            for (size_t i = 1; i < tam_entrada; i++)
            {
                fwrite(&valor_inicial, sizeof(uint32_t), 1, archivo_fat);
            }
            fflush(archivo_fat);
            fclose(archivo_fat);
        }
    }

	int fd_tabla_FAT = open(path, O_RDWR); 
	tabla_fat_en_memoria = mmap(NULL, tam_entrada, PROT_WRITE, MAP_SHARED, fd_tabla_FAT, 0);
	close(fd_tabla_FAT);
}

fcb* levantar_fcb (char * nombre) {

    char * path = string_from_format ("%s/%s.fcb", config_valores_filesystem.path_fcb, nombre);

    t_config * archivo = config_create (path);

    fcb* archivo_FCB = malloc (sizeof (fcb)); 
    archivo_FCB->nombre_archivo = config_get_string_value (archivo, "NOMBRE_ARCHIVO");
    archivo_FCB->bloque_inicial = config_get_int_value(archivo, "BLOQUE_INICIAL");
    archivo_FCB->tamanio_archivo = config_get_int_value (archivo, "TAMANIO_ARCHIVO");

	config_destroy (archivo);
    free(path);
    return archivo_FCB;
}

void levantar_archivo_bloque() {
    char *path_bloques = config_valores_filesystem.path_bloques;

    int fd_bloques = open(path_bloques, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd_bloques == -1) {
        perror("Error al abrir el archivo de bloques");
        abort();
    }

    // Mapear el archivo
    swap_mapeado = mmap(NULL, tamanio_swap, PROT_WRITE, MAP_SHARED, fd_bloques, 0);
    fat_mapeado = mmap(NULL, espacio_de_FAT, PROT_WRITE, MAP_SHARED, fd_bloques, tamanio_swap);

    if (swap_mapeado == MAP_FAILED || fat_mapeado == MAP_FAILED) {
        perror("Error al mapear el archivo en memoria");
        close(fd_bloques);
        abort();
    }

    close(fd_bloques);
}

//================================================= OPERACIONES ARCHIVOS ============================================
void crear_archivo (char *nombre_archivo, int socket_kernel) //literalmente lo unico que funciona
{
    char *path_archivo = string_from_format ("%s/%s.fcb", config_valores_filesystem.path_fcb, nombre_archivo);
	
	FILE *archivo = fopen(path_archivo, "w"); //en cambio ahí sí lo crea
	fclose(archivo);		

	//creamos la config para manejar el archivo mas facil
	t_config *archivo_nuevo = config_create (path_archivo);
	
	if (archivo_nuevo!=NULL)
	{
		//nos guardamos la direccion de memoria del archivos (uint por el envio de paquete, por las dudas pa que no rompa)
		uint32_t direccion = (uint32_t)archivo;

		config_set_value(archivo_nuevo, "NOMBRE_ARCHIVO", nombre_archivo);
		config_set_value(archivo_nuevo, "BLOQUE_INICIAL", "");
		config_set_value(archivo_nuevo, "TAMANIO_ARCHIVO", "0");
		config_save_in_file(archivo_nuevo,path_archivo);
		log_info(filesystem_logger, "Creamos el config y lo guardamos en disco\n");
		
		t_paquete *paquete = crear_paquete(ARCHIVO_CREADO);
		agregar_entero_a_paquete(paquete, direccion);
		enviar_paquete(paquete, socket_kernel);
		eliminar_paquete(paquete);

		config_destroy (archivo_nuevo);
		free (path_archivo);
	}
} 

void abrir_archivo (char *nombre_archivo, int socket_kernel)
{
	// /home/utnso/tp-2023-2c-KernelTitans/filesystem/fs/fat.dat
	char *path_archivo = string_from_format("%s/%s.fcb", config_valores_filesystem.path_fcb, nombre_archivo);
	
	printf("entramos al path: %s\n", path_archivo);

	if (!access (path_archivo, F_OK))
	{
		log_info(filesystem_logger,"Tamanio del archivo (que ya existe): %d \n", config_valores_fcb.tamanio_archivo);
						
		int tamanio = config_valores_fcb.tamanio_archivo;
		t_paquete *paquete = crear_paquete(ARCHIVO_ABIERTO);
		agregar_entero_a_paquete(paquete, tamanio);
		
		//hacemos lo de la direccion del archivo		
		FILE *archivo = fopen(path_archivo,"r");
		uint32_t direccion = (uint32_t)archivo;
		agregar_entero_a_paquete(paquete,direccion);
		log_info(filesystem_logger, "Enviando confirmacion de que existe el archivo solicitado\n");

		//fclose(archivo); esto creo que no, solo cuando mandan FCLOSE
		enviar_paquete(paquete, socket_kernel);
		eliminar_paquete(paquete);

    }else // el archivo no existe
	{
		log_info(filesystem_logger, "No existe el archivo solicitado\n");
	    t_paquete *paquete = crear_paquete(ARCHIVO_NO_EXISTE);
		agregar_entero_a_paquete(paquete, 1);
		enviar_paquete(paquete, socket_kernel);
		eliminar_paquete(paquete);
	}
	
	free(path_archivo); 
}

void truncar_archivo(char *nombre, int tamanio_nuevo)
{	
	int tamanio_archivos_bloques = config_valores_filesystem.tam_bloque * config_valores_filesystem.cant_bloques_total;
	int cant_espacio_swap = config_valores_filesystem.cant_bloques_swap - 1;
	int cant_espacio_fat = tamanio_archivos_bloques - cant_espacio_swap;

	fcb* fcb_a_truncar = levantar_fcb(nombre);

	log_info(filesystem_logger,"Truncando archivo: Nombre %s, tamanio %d, bloque inicial %d \n",fcb_a_truncar->nombre_archivo, fcb_a_truncar->tamanio_archivo, fcb_a_truncar->bloque_inicial);

	//comparar el tamanio del archivo actual con el nuevo
	if(fcb_a_truncar->tamanio_archivo < tamanio_nuevo && tamanio_nuevo < cant_espacio_fat)
	{
		log_info(filesystem_logger,"Ampliamos\n");
		ampliar_tamanio_archivo(tamanio_nuevo, fcb_a_truncar);
	}
	else if (fcb_a_truncar->tamanio_archivo > tamanio_nuevo)
	{
		log_info(filesystem_logger,"reducimos\n");
		reducir_tamanio_archivo(tamanio_nuevo, fcb_a_truncar);
	}
	else printf("Bueno bueno, para... No se puede hacer eso");
	
	//actualizamos la estructura fcb y la cargamos al archivo
	int bloque_inicial;
	fcb *nuevo_fcb;
	nuevo_fcb->nombre_archivo = nombre;
	nuevo_fcb->tamanio_archivo = tamanio_nuevo;
	nuevo_fcb->bloque_inicial = bloque_inicial; //no está definido, mejor dicho no se lo pasan como parametro
	actualizar_fcb(nuevo_fcb);
	printf("terminamos y mandamos el fcb a actualizarse");
}

/*
Se deberá solicitar al módulo Memoria la información que se encuentra 
a partir de la dirección física recibida y se escribirá en el bloque 
correspondiente del archivo a partir del puntero recibido.
*/
void escribir_archivo(char* nombre_archivo, uint32_t puntero_archivo, void* contenido){
	
	fcb* archivo_a_leer = levantar_fcb (nombre_archivo);

	uint32_t bloque_inicial = archivo_a_leer->bloque_inicial; 
	int tamanio_archivo = archivo_a_leer->tamanio_archivo;
	int tam_bloque = config_valores_filesystem.tam_bloque;
	uint32_t bloque_final = puntero_archivo/tam_bloque;
	
	free(archivo_a_leer);
    
	escribir_contenido_en_bloque(bloque_inicial,bloque_final, contenido, tam_bloque);

	int ok_write = 1;
	send(socket_kernel, &ok_write, sizeof(int), 0);

}

void escribir_contenido_en_bloque(uint32_t bloque_final, void* contenido, int tam_bloque) {
		
		uint32_t nro_de_bloque = tabla_fat_en_memoria[bloque_final];
		int tamanio = nro_de_bloque * tam_bloque;
		    
		//Copio los datos desde el archivo contenido al bloque del fat mappeado 
    	memcpy((char*)fat_mapeado + tamanio, contenido, tamanio);
		
		//Le había hecho un malloc en memoria
		free(contenido);
}

void solicitar_informacion_memoria(uint32_t direccion_fisica, int tam_bloque, char* nombre_archivo, uint32_t puntero_archivo)
{
	t_paquete* paquete = crear_paquete(LEER_EN_MEMORIA);
	agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
	agregar_entero_a_paquete(paquete, tam_bloque);
	agregar_entero_sin_signo_a_paquete(paquete, puntero_archivo);
	agregar_cadena_a_paquete(paquete, nombre_archivo);
	enviar_paquete(paquete, socket_memoria);
	eliminar_paquete(paquete);
}
void leer_archivo(char *nombre_archivo, uint32_t puntero_archivo, uint32_t direccion_fisica)
{	
	fcb* archivo_a_leer = levantar_fcb (nombre_archivo);

	uint32_t bloque_inicial = archivo_a_leer->bloque_inicial; 
	int tamanio_archivo = archivo_a_leer->tamanio_archivo;
	int tam_bloque = config_valores_filesystem.tam_bloque;
	uint32_t bloque_final = puntero_archivo/tam_bloque;
	
	free(archivo_a_leer);
	
	//char *path_fat = config_valores_filesystem.path_fat;

	recorrer_tabla_fat(bloque_inicial,bloque_final, tam_bloque, direccion_fisica);
}

void escribir_en_memoria(int tam_bloque, void* contenido, uint32_t direccion_fisica) {
	t_paquete *paquete = crear_paquete(ESCRIBIR_EN_MEMORIA); 
	agregar_entero_a_paquete(paquete, tam_bloque);
	agregar_bytes_a_paquete(paquete,contenido,tam_bloque); //Revisar dsps 
	agregar_entero_sin_signo_a_paquete(paquete,direccion_fisica); 
	enviar_paquete(paquete,socket_memoria);
	eliminar_paquete(paquete);
	free(contenido);
}

void recorrer_tabla_fat(uint32_t bloque_inicial, uint32_t bloque_final, int tam_bloque, uint32_t direccion_fisica) {

	uint32_t indice = bloque_inicial; 

 	while (tabla_fat_en_memoria[indice] != UINT32_MAX && indice != bloque_final) {		

		uint32_t dato_a_copiar = tabla_fat_en_memoria[indice];
		int tamanio = dato_a_copiar * tam_bloque;

		//Creo un buffer temporal
		void *buffer = malloc(espacio_de_FAT);
		    
		//Copio los datos desde el archivo mapeado al nuevo buffer 
		memcpy(buffer, (char*)fat_mapeado + tamanio, tamanio);

		//Mandamos el contenido(buffer) a que se persista en memoria
		escribir_en_memoria(tam_bloque, buffer, direccion_fisica);

		//Leemos el próximo bloque
		indice = tabla_fat_en_memoria[bloque_inicial]; 
	}

	//Avisa al kernel que terminó
	int ok_read = 1;
    send(socket_kernel, &ok_read, sizeof(int), 0);
}
//============================================= ACCESORIOS DE ARCHIVOS =================================================================

void actualizar_fcb(fcb* nuevo_fcb)
{
	char * path = string_from_format ("%s/%s.fcb", config_valores_filesystem.path_fcb, nuevo_fcb->nombre_archivo);

    t_config * archivo = config_create (path);
	char* bloque; 	sprintf(bloque, "%d", nuevo_fcb->bloque_inicial);
	char* tamanio; 	sprintf(tamanio, "%d", nuevo_fcb->tamanio_archivo);
	printf("convirtio bien");
	
	config_set_value(config, "NOMBRE_ARCHIVO", nuevo_fcb->nombre_archivo);
	config_set_value(config, "BLOQUE_INICIAL", bloque);
	config_set_value(config, "TAMANIO_ARCHIVO", tamanio);
	config_save_in_file(config,path);
	config_destroy(config);
	printf("actualizarmos bien el fcb");
}

//AMPLIAR Y REDUCIR NICO
void ampliar_tamanio_archivo (int nuevo_tamanio_archivo, fcb* fcb_archivo)
{
	int tamanio_archivos_bloques = config_valores_filesystem.tam_bloque * config_valores_filesystem.cant_bloques_total;
	int cant_espacio_swap = config_valores_filesystem.cant_bloques_swap - 1;
	int cant_espacio_fat = tamanio_archivos_bloques - cant_espacio_swap;

	//calculamos cantidad a agregar
	int bloques_a_agregar = nuevo_tamanio_archivo - fcb_archivo->tamanio_archivo;
	
	int posicion_ultimo_bloque = fcb_archivo->bloque_inicial + bloques_a_agregar;

	for
	(
		int posicion_bloque_agregado = 0;
		posicion_bloque_agregado <= bloques_a_agregar;
		posicion_bloque_agregado++
	)
	{

		uint32_t* nuevo_ultimo_bloque_fat = (uint32_t*)malloc(sizeof(config_valores_filesystem.tam_bloque));
		bloque_swap* nuevo_ultimo_bloque_bloques = (bloque_swap*)malloc(sizeof(config_valores_filesystem.tam_bloque));
		

		
		//definir el puntero
		if (nuevo_ultimo_bloque_fat == NULL && nuevo_ultimo_bloque_bloques == NULL)
		{
			perror("No se pudo alocar memoria para hacer la entrada fat\n");
			abort();
		}
		else
		{
			//ABRIMOS LOS DOS ARCHIVOS QUE VAMOS A MODIFICAR
			
			//fat
			FILE* archivo_fat = fopen(config_valores_filesystem.path_fat, "wb+");
			fseek(archivo_fat,posicion_bloque_agregado,SEEK_SET);
			
			//bloques
			FILE* archivo_bloques = fopen(config_valores_filesystem.path_bloques, "wb+");
			int posicion_archivo_bloques = cant_espacio_swap + posicion_bloque_agregado;
			fseek(archivo_bloques,posicion_archivo_bloques,SEEK_SET);

			//ACTUALIZAMOS LOS DOS ARCHIVOS
			if(archivo_fat != NULL && archivo_bloques != NULL)
			{
				//------------------ACTUALIZAMOS TABLA FAT-----------------//
				if(( posicion_bloque_agregado + 1 ) == posicion_ultimo_bloque ) 
				//El que tenemos creado en realidad seria el anteultimo y el siguiente el ultimo
				{
					//escribimos el anteultimo
					fwrite(nuevo_ultimo_bloque_fat, sizeof(uint32_t), 1, archivo_fat);
					//escribimos ahora si el ultimo bloque
					fwrite(UINT32_MAX, sizeof(uint32_t), 1, archivo_fat); //escribimos el ultimo bloque
					
					free(nuevo_ultimo_bloque_fat);
					//free(nuevo_ultimo_bloque);
					log_info(filesystem_logger,"actualizmos el ultimo bloque de archivo fat\n");
				}
				else
				//si el siguiente no es el ultimo bloque solo guardamos el valor del puntero
				{
					//escribimos el bloque
					fwrite(nuevo_ultimo_bloque_fat, sizeof(uint32_t), 1, archivo_fat);
					
					free(nuevo_ultimo_bloque_fat);
					//free(nuevo_ultimo_bloque);
					log_info(filesystem_logger,"actualizmos un bloque de archivo fat\n");
				}
				

				//------------------ACTUALIZAMOS ARCHIVO BLOQUES-----------------//
				if((cant_espacio_swap + posicion_bloque_agregado + 1) == (cant_espacio_swap + posicion_ultimo_bloque ) )
				//El que tenemos creado en realidad seria el anteultimo y el siguiente el ultimo
				{
					nuevo_ultimo_bloque_bloques->data = "/0"; 
					fwrite(nuevo_ultimo_bloque_bloques, sizeof(bloque_swap), 1, archivo_bloques);
					nuevo_ultimo_bloque_bloques->data = UINT32_MAX; 
					fwrite(nuevo_ultimo_bloque_bloques, sizeof(bloque_swap), 1, archivo_bloques);
					
					log_info(filesystem_logger,"actualizmos el ultimo bloque de archivo bloques\n");
				}
				else
				//este no es el ultimo
				{
					nuevo_ultimo_bloque_bloques->data = "/0"; 
					fwrite(nuevo_ultimo_bloque_bloques, sizeof(bloque_swap), 1, archivo_bloques);
					
					log_info(filesystem_logger,"actualizmos un bloque de archivo bloques\n");
				}
				
				fflush(archivo_fat);
				fflush(archivo_bloques);

			}else printf("no existe/no se pudo abrir la tabla fat o el archivo de bloques"); 
	
			fclose(archivo_fat);
			fclose(archivo_bloques);
			
		}	
	}
	log_info(filesystem_logger,"En teoria termino el for y se crearon los bloques\n");
}

void reducir_tamanio_archivo (int nuevo_tamanio_archivo, fcb* fcb_archivo)
{
	int bloques_a_quitar = fcb_archivo->tamanio_archivo - nuevo_tamanio_archivo;
	
	int bloques_totales = fcb_archivo->bloque_inicial + fcb_archivo->tamanio_archivo;

	for(int bloque_quitado = 0; bloque_quitado <= (bloques_totales - bloques_a_quitar); bloque_quitado++)
	{
		//futuras pruebas ftell(archivo) dice donde estas parado
		
		//no creo que esten abiertos
		//en el fat ponemos el puntero, porque es el mapa
		FILE* archivo_fat = fopen(config_valores_filesystem.path_fat, "wb+");

		fseek(archivo_fat, bloque_quitado * sizeof(uint32_t), SEEK_SET);

		//en el archivo de bloques si ponemos el dato, que por ahora es nada
		FILE* archivo_bloques = fopen(config_valores_filesystem.path_bloques, "wb+");
		int posicion_archivo_bloques = list_size(lista_bloques_swap) + list_size(tabla_fat);
		fseek(archivo_fat,posicion_archivo_bloques,SEEK_SET); // revisar

		if(archivo_fat != NULL && archivo_bloques != NULL)
		{
			//actualizamos archivo fat 
			if(bloque_quitado == (bloques_totales - bloques_a_quitar) ) //es el bloque final
			{
				//fwrite espera un puntero al dato
				uint32_t bloque_nuevo = UINT32_MAX;
				fwrite(&bloque_nuevo, sizeof(uint32_t), 1, archivo_fat);

			}
			else
			{
				int bloque_nuevo = 0;
				fwrite(&bloque_nuevo, sizeof(uint32_t), 1, archivo_fat);
			}
			
			//actualizamos la tabla local fat
			uint32_t bloque_nuevo=0;
			list_replace(tabla_fat,bloque_quitado,bloque_nuevo);
			//list_replace_and_destroy_element(tabla_fat,bloque_quitado,bloque_nuevo,(void*)vaciar_bloque(bloque_nuevo));
			
			//actualizamos lista de bloques local
			bloque_swap* nuevo_bloque_fat = malloc(sizeof(bloque_swap));
			nuevo_bloque_fat->data = NULL;
			//list_replace_and_destroy_element(tabla_fat,bloque_quitado,bloque_nuevo,liberar_bloque_individual(bloque_nuevo));
			
			//actualizamos archivo bloque 
			if(bloque_quitado == (bloques_totales - bloques_a_quitar) ) //es el bloque final
			{
				nuevo_bloque_fat->data = UINT32_MAX;
				fwrite(nuevo_bloque_fat, sizeof(bloque_swap*), 1, archivo_bloques); //vemos si es un bloque* otra cosa 
			}
			else
			{
				//list_remove_and_destroy_element()
				fwrite(0, sizeof(bloque_swap), 1, archivo_fat);
			}
						
			fflush(archivo_fat);
			fflush(archivo_bloques);

		}else printf("no existe/no se pudo abrir la tabla fat o el archivo de bloques"); 

		fclose(archivo_fat);
		fclose(archivo_bloques);
	}
	printf("en teoria termino el for y se eliminaron ls bloques");
}


char* devolver_direccion_archivo(char* nombre)
{
	//creamos el comandito pa entrar al directorio y empezar a buscar el archivo
	char comando[256];
	snprintf(comando, sizeof(comando), "ls -1 %s", "filesystem/fs/fcbs"); //carpeta de lo fcbs
	FILE *tuberia_conexion = popen(comando, "r");

    if (tuberia_conexion != NULL) {
        
		char nombre_archivo_buscado[256];

        // Leer los nombres de los archivos en el directorio (en cada ciclo de while avanza???)
        while (fscanf(tuberia_conexion, "%255s", nombre_archivo_buscado) != EOF) //no creo que tenga mas de 255
		{
            if (strcmp(nombre_archivo_buscado, nombre) == 0)
			{
                printf("Se encontró el archivo: %s\n", nombre_archivo_buscado);
				char* direccion = sprintf("filesystem/fs/fcbs/%s.dats",nombre_archivo_buscado);
				
				pclose(tuberia_conexion);		
				return direccion;
            }
        }
		printf("No encontró el archivo %s, se llego al final\n", nombre_archivo_buscado);
        pclose(tuberia_conexion);
		return NULL;
    }
	else 
	{
        perror("Error al abrir la carpeta");
        pclose(tuberia_conexion);
		return NULL;
    }
}

