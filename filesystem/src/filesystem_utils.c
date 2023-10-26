#include "filesystem.h"

fcb config_valores_fcb;
//..................................CONFIGURACIONES.....................................................................

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

// ATENDER CLIENTES //

void atender_clientes_filesystem(void* conexion) {
    int cliente_fd = *(int*)conexion;
    char* nombre_archivo = NULL;
    int tamanio_archivo = -1;
    FILE* puntero_archivo = NULL;
    char* direccion_fisica = NULL;
    char* informacion = NULL;
    int bloques_a_reservar = -1;
    int bloques_a_liberar = -1;

    t_paquete* paquete = recibir_paquete(cliente_fd);
    void* stream = paquete->buffer->stream;

    switch(paquete->codigo_operacion)
    {
        case ABRIR_ARCHIVO:
                // nombre_archivo = sacar_cadena_de_paquete(&stream);
                log_info(filesystem_logger, "Abrir Archivo: %s", nombre_archivo);
                //if(existe(nombre_archivo)) { enviar_tamanio_archivo();}
                //else (enviar_mensaje ("No existe"));
        break;
        case CREAR_ARCHIVO:
               log_info(filesystem_logger, "Crear Archivo: %s", config_valores_fcb.nombre_archivo);
               //crear_fcb (tamanio = 0, bool_bloque_inicial = false);
               //enviar_mensaje("OK");
        break;
        case TRUNCAR_ARCHIVO:
            // nombre_archivo = sacar_cadena_de_paquete(&stream);
            // tamanio_archivo = sacar_entero_de_paquete(&stream);
            log_info(filesystem_logger, "Truncar Archivo: %s- Tamaño: %d", nombre_archivo, tamanio_archivo);
            //if(algo) {ampliar_tamanio_archivo (nombre_archivo, tamanio_archivo)}
            //else {reducir_tamanio_archivo (nombre_archivo, tamanio_archivo)}
        break;
        case LEER_ARCHIVO:
            // puntero_archivo = sacar_puntero_de_paquete(&stream);
            // direccion_fisica = sacar_cadena_de_paquete(&stream);
            log_info(filesystem_logger, "Leer Archivo: %s - Puntero: %p - Memoria: %s ", nombre_archivo, (void*)puntero_archivo, direccion_fisica);
            /* informacion = leer_informacion(bloque)
             t_paquete* paquete = crear_paquete(ESCRIBIR_INFORMACION);
            agregar_cadena_a_paquete(paquete, informacion);
            enviar_paquete(paquete, socket_memoria);
            eliminar_paquete(paquete);
            int respuesta = 0;
            recv(socket_memoria, &respuesta,sizeof(int),0);

            if (respuesta != 1)
            {
            log_error(kernel_logger, "No se pudieron crear estructuras en memoria");
            }
            enviar_mensaje ("Se leyó el archivo con exito");
        */ 
        break;
        case ESCRIBIR_ARCHIVO:
            // direccion_fisica = sacar_cadena_de_paquete(&stream);
            // puntero_archivo = sacar_puntero_de_paquete(&stream);
            log_info(filesystem_logger, "Escribir Archivo: %s - Puntero: %p - Memoria: %s ", nombre_archivo, (void*)puntero_archivo, direccion_fisica);
                /* 
             t_paquete* paquete = crear_paquete(LEER_INFORMACION);
            agregar_cadena_a_paquete(paquete, direccion_fisica);
            agregar_puntero_a_paquete(paquete, puntero_archivo);
            enviar_paquete(paquete, socket_memoria);
            eliminar_paquete(paquete);
        */ 
       break;
       case INICIAR_PROCESO:
            //bloques_a_reservar = sacar_entero_de_paquete(&stream);
            // t_list* bloques_reservados = reservar_bloques(bloques_a_reservar);
            /* t_paquete* paquete = crear_paquete(BLOQUES_RESERVADOS);
            agregar_array_cadenas_a_paquete(paquete, bloques_reservados);
            enviar_paquete(paquete, socket_memoria);
            eliminar_paquete(paquete);*/
        break;
        case FINALIZAR_PROCESO:
            //bloques_a_liberar = sacar_entero_de_paquete(&stream);
            // liberar_bloques_swap(bloques_a_liberar);
        break;
        default:
            log_warning(filesystem_logger, "Operacion desconocida \n");
        break;
    }
    eliminar_paquete(paquete);
}

void levantar_fcb(char* path) {

	config = config_create(path); //Leo el archivo de configuracion
      if (config != NULL)
	  {
		config_valores_fcb.nombre_archivo = config_get_string_value(config, "NOMBRE_ARCHIVO");
		config_valores_fcb.tamanio_archivo = config_get_int_value(config, "TAMANIO_ARCHIVO");
		config_valores_fcb.bloque_inicial = config_get_int_value(config, "BLOQUE_INICIAL");
	  }
	  else
	  {
		perror("\nArchivo de configuracion de fcb no encontrado \n");
		abort();
	  }
}

void levantar_fat(size_t tamanio_fat) {
    char* path = config_valores_filesystem.path_fat;
    FILE* archivo_fat = fopen(path, "rb+");
    if (archivo_fat == NULL) {
        // Si no se encuentra el fat se inicializa con 0s.
		archivo_fat = fopen(path, "wb+");
        if (archivo_fat == NULL) {
            perror("\n No se puedo abrir o crear el archivo_fat \n");
            abort();
        }
		else printf("\nTerminamos abriendo con wb+\n");

        uint32_t valor_inicial = 0; //Revisar que no sea while(!UINT32_MAX)
        for (size_t i = 0; i < tamanio_fat / sizeof(uint32_t); i++) {
            fwrite(&valor_inicial, sizeof(uint32_t), 1, archivo_fat);
        }
        fflush(archivo_fat);
    } else log_info(filesystem_logger,"\n Error al levantar archivo fcb \n");
    
/*
    // Escritura 
    uint32_t nuevo_valor = UINT32_MAX; 
    fseek(archivo_fat, block_number * sizeof(uint32_t), SEEK_SET);
    fwrite(&nuevo_valor, sizeof(uint32_t), 1, archivo_fat);
    fflush(archivo_fat);

    // Lectura
    uint32_t valor;
    fseek(archivo_fat, block_number * sizeof(uint32_t), SEEK_SET);
    fread(&valor, sizeof(uint32_t), 1, archivo_fat);
*/
    fclose(archivo_fat);
}

void levantar_archivo_bloque(size_t tamanio_swap, size_t tamanio_fat) {
    
	char* path = config_valores_filesystem.path_bloques;

    FILE* archivo_bloque = fopen(path, "rb"); // Leemos en modo binario, puede ser wb+
    
	if (archivo_bloque == NULL)
	{
    perror("\n\nNo se pudo abrir el archivo de bloques \n\n");
    abort();
    }
	else printf("\n\n Se abrio el archivo de bloques\n\n");

    // Le asignamos un espacio de memoria al swap
	uint32_t* particion_swap = (uint32_t*)malloc(tamanio_swap);
    if (particion_swap == NULL)
	{
    perror("\n\nNo se pudo alocar memoria para el swap\n \n");
    abort();
    }
	else
	{
	fread(particion_swap, 1, 1, archivo_bloque);
	printf("\n\n Se aloco la memoria para el swap\n\n");
	}

    // Le asignamos un espacio de memoria al fat
	uint32_t* particion_fat = (uint32_t*)malloc(tamanio_fat);
    if (particion_fat == NULL)
	{
    perror("\n\nNo se pudo alocar memoria para el fat \n\n");
    abort();
    }
	else
	{
    fread(particion_fat, 1, tamanio_fat, archivo_bloque);
	printf("\n\n Se aloco la memoria para el fat\n\n");
	free(particion_fat);
	}

	fclose(archivo_bloque);

    printf("\n\nSe levanto archivo de bloque\n\n");

}          

//..................................FUNCIONES UTILES.....................................................................

char* concatenarCadenas(const char* str1, const char* str2) {
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);

    char* resultado = (char*)malloc((len1 + len2 + 1) * sizeof(char));

    if (resultado == NULL) {
        perror("Error de asignación de memoria");
        return NULL;
    }

    strcpy(resultado, str1);
    strcat(resultado, str2);

    return resultado;
}

int dividirRedondeando(int numero1 , int numero2)
{
	if(numero1 % numero2 == 0)
	{
		return (numero1)/(numero2);
	}
	else
	{
		return (numero1)/(numero2) + 1;
	}
}

/*
int contarArchivosEnCarpeta(const char *carpeta, char ***vectoreRutas) {
    DIR *dir;
    struct dirent *ent;
    int contador = 0;
    int contador2 = 0;
    char *mediaRutaAbsoluta = concatenarCadenas(carpeta,"/");
    char *rutaAbsoluta;
    dir = opendir(carpeta);

    if (dir == NULL)
    {
        log_info(logger, "No se pudo abrir la carpeta");
        return -1; // Retorna -1 en caso de error
    }

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_REG)
        { // Verifica si es un archivo regular
        	contador++;
        }
    }
    *vectoreRutas = malloc(contador * sizeof(char*));
    closedir(dir);
    dir = opendir(carpeta);
    while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) { // Verifica si es un archivo regular
            	(*vectoreRutas)[(contador2)]=malloc((strlen(ent->d_name) + 1) * sizeof(char) + (strlen(mediaRutaAbsoluta) + 1) * sizeof(char));
            	rutaAbsoluta = concatenarCadenas(mediaRutaAbsoluta,ent->d_name);
            	strcpy((*vectoreRutas)[(contador2)], rutaAbsoluta);
            	contador2++;
            	free(rutaAbsoluta);
            }
        }
    closedir(dir);
    free (mediaRutaAbsoluta);
    return contador;
}
*/

//..................................FUNCIONES UTILES ARCHIVOS.....................................................................

int abrirArchivo(char *nombre, char **vectorDePaths,int cantidadPaths)
{
	log_info(filesystem_logger,"Abrir Archivo: %s",nombre);
	int i=0;
	t_config* config_inicial;
	while (i<cantidadPaths)
	{
		config_inicial= iniciar_config(vectorDePaths[i]);
		if(strcmp(nombre,config_get_string_value(config_inicial,"NOMBRE_ARCHIVO")) == 0)
		{
			config_destroy(config_inicial);
			return 1;
		}
		i++;
		config_destroy(config_inicial);
	}
	return 0;
}

int crearArchivo(char *nombre,char *carpeta, char ***vectoreRutas, int *cantidadPaths)
{
	log_info(filesystem_logger, "Crear Archivo: %s",nombre);
	FILE* archivo;
	t_config* configArchivo;
	char **vectorPruebas;
	char *mediaRutaAbsoluta = concatenarCadenas(carpeta,"/");
	char *rutaArchivo = malloc ((strlen(nombre) + 1) * sizeof(char) +(strlen(mediaRutaAbsoluta) + 1) * sizeof(char));
	rutaArchivo = concatenarCadenas(mediaRutaAbsoluta, nombre);

	archivo = fopen(rutaArchivo , "w+");
	fclose(archivo);
	configArchivo = iniciar_config(rutaArchivo);
	config_set_value(configArchivo,"NOMBRE_ARCHIVO", nombre);
	config_set_value(configArchivo,"TAMANIO_ARCHIVO", "0");
	config_save(configArchivo);
	vectorPruebas = realloc(*vectoreRutas,(*cantidadPaths + 1) * sizeof(char*));
	if (vectorPruebas != NULL)
	{
		(*vectoreRutas) = vectorPruebas;

		(*vectoreRutas)[*cantidadPaths] = malloc((strlen(rutaArchivo) + 1) * sizeof(char));
		strcpy((*vectoreRutas)[*cantidadPaths],rutaArchivo);
		*cantidadPaths = *cantidadPaths + 1;
		config_destroy(configArchivo);
		free (mediaRutaAbsoluta);
		free(rutaArchivo);
		return 1;
	}
	else
	{
		config_destroy(configArchivo);
		free (mediaRutaAbsoluta);
		free(rutaArchivo);
		return 0;
	}
}

int truncarArchivo(char *nombre,char *carpeta, char **vectoreRutas, int cantidadPaths, int tamanioNuevo)
{
	log_info(filesystem_logger, "Truncar Archivo: %s - Tamaño: %d",nombre,tamanioNuevo);
	int i=0;
	t_config* configArchivoActual;
	int tamanioBloques;
	int tamanioOriginal;
	int cantidadBloquesOriginal;
	int cantidadBloquesNueva;

	while (i<cantidadPaths)
	{
		configArchivoActual = iniciar_config(vectoreRutas[i]);
		if(strcmp(nombre,config_get_string_value(configArchivoActual,"NOMBRE_ARCHIVO")) == 0)
		{
			log_info(filesystem_logger,"Truncar archivo dice: ENCONTRE EL ARCHIVO A TRUNCAR");
			break;
		}
		i++;
		config_destroy(configArchivoActual);
	}

	tamanioOriginal = config_get_int_value(configArchivoActual,"TAMANIO_ARCHIVO");
	//tamanioBloques = config_get_int_value(superBloque,"BLOCK_SIZE");;
	config_set_value(configArchivoActual,"TAMANIO_ARCHIVO",string_itoa(tamanioNuevo));
	cantidadBloquesOriginal = dividirRedondeando(tamanioOriginal, tamanioBloques);
	cantidadBloquesNueva = dividirRedondeando(tamanioNuevo, tamanioBloques);
	config_save(configArchivoActual);
	if (tamanioOriginal < tamanioNuevo && cantidadBloquesOriginal != cantidadBloquesNueva)
	{
		log_info(filesystem_logger,"Se agranda el archivo");
		//agregarBloques(cantidadBloquesOriginal,cantidadBloquesNueva,configArchivoActual);
		config_destroy(configArchivoActual);
		return 1;
	}
	else
	{
		if(cantidadBloquesOriginal != cantidadBloquesNueva)
		{
			log_info(filesystem_logger,"Se achica el archivo");
			//sacarBloques(cantidadBloquesOriginal,cantidadBloquesNueva,configArchivoActual, tamanioOriginal);
			config_destroy(configArchivoActual);
			return 1;
		}
	}
	log_info(filesystem_logger, "No se modificaron la canidad de bloques del archivo");
	config_destroy(configArchivoActual);
	return 1;
}

/*
int escribirArchivo(char *nombreArchivo,size_t punteroSeek,size_t bytesAEscribir,int direccion,void *memoriaAEscribir)
{
	log_info(filesystem_logger,"Escribir Archivo: %s - Puntero: %ld - Memoria: %d - Tamaño: %ld",nombreArchivo,punteroSeek,direccion,bytesAEscribir);
	int i=0;
	t_config* configArchivoActual;
	size_t bloqueAEscribir;
	FILE *bloques = fopen(config_get_string_value(config,"PATH_BLOQUES"),"r+");
	size_t tamanioBloque = config_get_int_value(superBloque,"BLOCK_SIZE");
	size_t escritoAnteriormente = 0;

	while (i<cantidadPaths)
	{
		configArchivoActual = iniciar_config(vectorDePathsPCBs[i]);
		if(strcmp(nombreArchivo,config_get_string_value(configArchivoActual,"NOMBRE_ARCHIVO")) == 0)
		{
			log_info(filesystem_logger,"Escribir  dice: ENCONTRE EL ARCHIVO A escribir");
			break;
		}
		i++;
		config_destroy(configArchivoActual);
	}
	int cantidadBloquesAEscribir = cantidadDeBloquesAAcceder(configArchivoActual,punteroSeek,bytesAEscribir);
	bloqueAEscribir = punteroSeek /tamanioBloque;
	log_info(filesystem_logger,"El bloque del archivo a escribir es el bloque %ld",bloqueAEscribir);
	if(bloqueAEscribir == 0)
	{
		moverPunteroAbloqueDelArchivo(bloques,configArchivoActual,bloqueAEscribir);
		//Me fijo si todo lo que voy a leer esta en un solo bloque
		if(cantidadBloquesAEscribir)
		{
			//Escribo todo lo que puedo en el primer bloque del archivo y luego paso al segundo
			log_info(filesystem_logger,"La informacion a escribir NO entra en un solo bloque");
			fseek(bloques,punteroSeek,SEEK_CUR);
			fwrite(memoriaAEscribir,tamanioBloque-punteroSeek,1,bloques);
			
			escritoAnteriormente = tamanioBloque-(punteroSeek-bloqueAEscribir * tamanioBloque);
			for(int i = 1;i<cantidadBloquesAEscribir;i++)
			{
				moverPunteroAbloqueDelArchivo(bloques,configArchivoActual,bloqueAEscribir + i);
				fwrite(memoriaAEscribir + escritoAnteriormente,(bytesAEscribir - escritoAnteriormente)-((cantidadBloquesAEscribir - (i+1)) * tamanioBloque),1,bloques);
				escritoAnteriormente = (size_t)memoriaAEscribir + (bytesAEscribir - escritoAnteriormente)-((cantidadBloquesAEscribir - (i+1)) * tamanioBloque);	
			}
			fclose(bloques);
			return 1;
		}
		//escribo todo en el bloque 0
		else
		{
			log_info(filesystem_logger,"La informacion a escribir entra en un solo bloque");
			fseek(bloques,punteroSeek,SEEK_CUR);
			fwrite(memoriaAEscribir,bytesAEscribir,1,bloques);
			fclose(bloques);
			return 1;
		}
	}
	//no tengo que escribir el bloque del puntero directo. Paso a buscar el primer bloque
	else
	{
		//Hay mas de un bloque para leer
		if(cantidadBloquesAEscribir)
		{
			log_info(filesystem_logger,"La informacion a escribir NO entra en un solo bloque");
			moverPunteroAbloqueDelArchivo(bloques,configArchivoActual,bloqueAEscribir);
			//Escribo todo lo que puedo del primer bloque
			fseek(bloques,punteroSeek-(tamanioBloque * bloqueAEscribir),SEEK_CUR);
			fwrite(memoriaAEscribir,tamanioBloque-(punteroSeek-bloqueAEscribir * tamanioBloque),1,bloques);
			escritoAnteriormente = tamanioBloque-(punteroSeek-bloqueAEscribir * tamanioBloque);
			for(int i = 1;i<cantidadBloquesAEscribir;i++)
			{
				moverPunteroAbloqueDelArchivo(bloques,configArchivoActual,bloqueAEscribir + i);
				fwrite(memoriaAEscribir + escritoAnteriormente,(bytesAEscribir - escritoAnteriormente)-((cantidadBloquesAEscribir - (i+1)) * tamanioBloque),1,bloques);
				escritoAnteriormente = (size_t)memoriaAEscribir + (bytesAEscribir - escritoAnteriormente)-((cantidadBloquesAEscribir - (i+1)) * tamanioBloque);

				
			}
			fclose(bloques);
			return 1;
		}
		//Hay solo un bloque que leer
		else
		{
			log_info(filesystem_logger,"La informacion a escribir entra en un solo bloque");
			moverPunteroAbloqueDelArchivo(bloques,configArchivoActual,bloqueAEscribir);
			fseek(bloques,punteroSeek-(tamanioBloque * bloqueAEscribir),SEEK_CUR);
			fwrite(memoriaAEscribir,bytesAEscribir,1,bloques);
			fclose(bloques);
			return 1;
		}
	}
	fclose(bloques);
	return 0;

}

void *leerArchivo(char *nombreArchivo,size_t punteroSeek,size_t bytesALeer, int direccion)
{
	log_info(filesystem_logger,"Leer Archivo: %s - Puntero: %ld - Memoria: %d - Tamaño: %ld",nombreArchivo,punteroSeek,direccion,bytesALeer);
	int i=0;
	t_config* configArchivoActual;
	size_t bloqueAleer;
	void *infoDelArchivo;
	FILE *bloques = fopen(config_get_string_value(config,"PATH_BLOQUES"),"r+");
	void *punteroFinal;
	size_t tamanioBloque = config_get_int_value(superBloque,"BLOCK_SIZE");
	size_t leidoAnteriormente = 0;
	while (i<cantidadPaths)
	{
		configArchivoActual = iniciar_config(vectorDePathsPCBs[i]);
		if(strcmp(nombreArchivo,config_get_string_value(configArchivoActual,"NOMBRE_ARCHIVO")) == 0)
		{
			log_info(filesystem_logger,"Leer  dice: ENCONTRE EL ARCHIVO A LEER");
			break;
		}
		i++;
		config_destroy(configArchivoActual);
	}
	bloqueAleer = punteroSeek /tamanioBloque;
	int cantidadBloquesALeer = cantidadDeBloquesAAcceder(configArchivoActual,punteroSeek,bytesALeer);
	log_info(filesystem_logger,"El bloque del archivo a leer es el bloque %ld",bloqueAleer);
	//Busco el bloque desde donde voy a leer usando los punteros del archivo
	if(bloqueAleer == 0)
	{
		moverPunteroAbloqueDelArchivo(bloques,configArchivoActual,bloqueAleer);
		//Me fijo si todo lo que voy a leer esta en un solo bloque
		if(cantidadBloquesALeer)
		{
			log_info(filesystem_logger,"La informacion a leer NO esta en un solo bloque");
			infoDelArchivo= malloc(bytesALeer);
			fseek(bloques,punteroSeek,SEEK_CUR);
			fread(infoDelArchivo,tamanioBloque-punteroSeek,1,bloques);
			
			//Ahora me voy al bloque siguiente
			leidoAnteriormente = tamanioBloque-(punteroSeek-bloqueAleer * tamanioBloque);
			for(int i = 1;i<cantidadBloquesALeer;i++)
			{
				moverPunteroAbloqueDelArchivo(bloques,configArchivoActual,bloqueAleer + i);
				fread(infoDelArchivo + leidoAnteriormente,(bytesALeer - leidoAnteriormente)-((cantidadBloquesALeer - (i+1)) * tamanioBloque),1,bloques);
				leidoAnteriormente = (size_t)infoDelArchivo + (bytesALeer - leidoAnteriormente)-((cantidadBloquesALeer - (i+1)) * tamanioBloque);
			}
			fclose(bloques);
			return infoDelArchivo;
		}
		//leo todo del primer bloque
		else
		{
			log_info(filesystem_logger,"La informacion a leer esta en un solo bloque");
			infoDelArchivo= malloc(bytesALeer);
			fseek(bloques,punteroSeek,SEEK_CUR);
			fread(infoDelArchivo,bytesALeer,1,bloques);
			fclose(bloques);
			return infoDelArchivo;

		}
	}
	//no tengo que leer el bloque del puntero directo. Paso a buscar los demas bloques
	else
	{
		moverPunteroAbloqueDelArchivo(bloques,configArchivoActual,bloqueAleer);

		//Hay mas de un bloque para leer
		if(cantidadBloquesALeer)
		{
			//Leo lo que puedo del primer bloque
			infoDelArchivo= malloc(bytesALeer);			
			fseek(bloques,punteroSeek-(tamanioBloque * bloqueAleer),SEEK_CUR);
			fread(infoDelArchivo,tamanioBloque-(punteroSeek-bloqueAleer * tamanioBloque),1,bloques);
			
			leidoAnteriormente = tamanioBloque-(punteroSeek-bloqueAleer * tamanioBloque);
			for(int i = 1;i<cantidadBloquesALeer;i++)
			{
				moverPunteroAbloqueDelArchivo(bloques,configArchivoActual,bloqueAleer + i);
				fread(infoDelArchivo + leidoAnteriormente,(bytesALeer - leidoAnteriormente)-((cantidadBloquesALeer - (i+1)) * tamanioBloque),1,bloques);
				leidoAnteriormente = (size_t)infoDelArchivo + (bytesALeer - leidoAnteriormente)-((cantidadBloquesALeer - (i+1)) * tamanioBloque);
			}
			fclose(bloques);
			return infoDelArchivo;
		}
		//Hay solo un bloque que leer
		else
		{
			infoDelArchivo = malloc(bytesALeer);
			fseek(bloques,punteroSeek-(tamanioBloque * bloqueAleer),SEEK_CUR);
			fread(infoDelArchivo,bytesALeer,1,bloques);
			fclose(bloques);
			return infoDelArchivo;
		}
	}
	fclose(bloques);
	return punteroFinal;
}

void sacarBloques(int cantidadBloquesOriginal ,int cantidadBloquesNueva,t_config* configArchivoActual,int tamanioOriginal)
{
	int cantidadBloquesAEliminar =cantidadBloquesOriginal - cantidadBloquesNueva;
	uint32_t punteroIndirecto = config_get_int_value(configArchivoActual,"PUNTERO_INDIRECTO");
	uint32_t punteroACadaBloque;
	//size_t tamanioBloque = config_get_int_value(superBloque,"BLOCK_SIZE");
	FILE *bloques = fopen(config_get_string_value(config,"PATH_BLOQUES"),"r+");

	if (cantidadBloquesOriginal <= 1)
	{
		//busco un bloque libre para agregar como bloque de punteros
		log_info(filesystem_logger,"El archivo solo tiene un bloque. No se modifica nada referido a los bloques");
		return;
	}
	if (cantidadBloquesNueva == 1)
	{
		// Me muevo al ultimo puntero del bloque de punteros para eliminar puntero por puntero
		log_info(filesystem_logger,"Acceso Bloque - Archivo: %s - Bloque Archivo: bloque de punteros - Bloque File System %d",config_get_string_value(configArchivoActual,"NOMBRE_ARCHIVO"),punteroIndirecto);
		delay(config_get_int_value(config,"RETARDO_ACCESO_BLOQUE"));
		//fseek(bloques,punteroIndirecto * tamanioBloque,SEEK_SET);
		fseek(bloques,sizeof(uint32_t)*(cantidadBloquesOriginal - 1), SEEK_CUR);

		for(int i=0;i<cantidadBloquesAEliminar ;i++)
		{
			fseek(bloques,-sizeof(uint32_t), SEEK_CUR);
			fread(&punteroACadaBloque,sizeof(uint32_t),1,bloques);
			//limpiarBitmap(bitmap, punteroACadaBloque);
			fseek(bloques,-sizeof(uint32_t), SEEK_CUR);

		}
		//limpiarBitmap(bitmap, config_get_int_value(configArchivoActual,"PUNTERO_INDIRECTO"));
		config_remove_key(configArchivoActual,"PUNTERO_INDIRECTO");
		config_save(configArchivoActual);
	}
	if (cantidadBloquesNueva > 1)
	{
		// Me muevo al ultimo puntero del bloque de punteros para eliminar puntero por puntero
		log_info(filesystem_logger,"Acceso Bloque - Archivo: %s - Bloque Archivo: bloque de punteros - Bloque File System %d",config_get_string_value(configArchivoActual,"NOMBRE_ARCHIVO"),punteroIndirecto);
		delay(config_get_int_value(config,"RETARDO_ACCESO_BLOQUE"));
		//fseek(bloques,punteroIndirecto * tamanioBloque,SEEK_SET);
		fseek(bloques,sizeof(uint32_t)*(cantidadBloquesOriginal - 1), SEEK_CUR);

		for(int i=0;i<cantidadBloquesAEliminar ;i++)
		{
			fseek(bloques,-sizeof(uint32_t), SEEK_CUR);
			fread(&punteroACadaBloque,sizeof(uint32_t),1,bloques);
			//limpiarBitmap(bitmap, punteroACadaBloque);
			fseek(bloques,-sizeof(uint32_t), SEEK_CUR);

		}
	}
	fclose(bloques);
	return;
}

void agregarBloques(int cantidadBloquesOriginal ,int cantidadBloquesNueva,t_config* configArchivoActual)
{
	int cantidadBloquesAAGregar = cantidadBloquesNueva - cantidadBloquesOriginal;
	FILE *bloques = fopen(config_get_string_value(config,"PATH_BLOQUES"),"r+");
	int punteroABloquePunteros=0;
	uint32_t punteroACadaBloque;
	if(cantidadBloquesOriginal == 0 && cantidadBloquesNueva == 1)
	{
		//Busco un bloque libre para agregar el primer bloque de datos
		log_info(filesystem_logger,"Se busca bloque libre para agregar como primer bloque");
		for(int i=0;i<config_get_int_value(superBloque, "BLOCK_COUNT");i++)
		{
			if (accesoBitmap(bitmap, i) == 0)
			{
				//Encontre un bloque vacio lo marco como ocupado
				setearBitmap(bitmap,i);
				config_set_value(configArchivoActual,"PUNTERO_DIRECTO",string_itoa(i));
				config_save(configArchivoActual);
				sincronizarBitmap();
				break;

			}
		}
	}
	if (cantidadBloquesOriginal == 0 && cantidadBloquesNueva >1)
		{
			//Busco un bloque libre para agregar el primer bloque de datos
			log_info(filesystem_logger,"Se busca bloque libre para agregar como primer bloque");
			for(int i=0;i<config_get_int_value(superBloque, "BLOCK_COUNT");i++)
			{
				if (accesoBitmap(bitmap, i) == 0)
				{
					//Encontre un bloque vacio lo marco como ocupado
					setearBitmap(bitmap,i);
					config_set_value(configArchivoActual,"PUNTERO_DIRECTO",string_itoa(i));
					config_save(configArchivoActual);
					sincronizarBitmap();
					break;

				}
			}
			//busco un bloque libre para agregar como bloque de punteros
			log_info(logger,"Se busca bloque libre para agregar los punteros a bloques");
			for(int i=0;i<config_get_int_value(superBloque, "BLOCK_COUNT");i++)
			{
				if (accesoBitmap(bitmap, i) == 0)
				{
					//Encontre un bloque vacio lo marco como ocupado
					setearBitmap(bitmap,i);
					config_set_value(configArchivoActual,"PUNTERO_INDIRECTO",string_itoa(i));
					config_save(configArchivoActual);
					punteroABloquePunteros = i;
					sincronizarBitmap();
					break;

				}
			}
			// Me ubico en el bloque de punteros
			log_info(logger,"Acceso Bloque - Archivo: %s - Bloque Archivo: bloque de punteros - Bloque File System %d",config_get_string_value(configArchivoActual,"NOMBRE_ARCHIVO"),punteroABloquePunteros);
			fseek(bloques,punteroABloquePunteros * config_get_int_value(superBloque,"BLOCK_SIZE"),SEEK_SET);
			delay(config_get_int_value(config,"RETARDO_ACCESO_BLOQUE"));
			log_info(logger,"Busco bloques libres para agregar al archivo");
			//Busco los espacios libres en el bitmap para agregar los bloques
			for (int i=0;i< cantidadBloquesAAGregar -1;i++)
			{
				int j=1;
				int posicion=0;
				while(j != 0)
				{
					j=accesoBitmap(bitmap, posicion);
					posicion++;
				}
				punteroACadaBloque = posicion - 1;
				setearBitmap(bitmap, posicion -1);
				fwrite(&punteroACadaBloque,sizeof(uint32_t),1,bloques);
			}
			fclose(bloques);
			return;
		}
	if(cantidadBloquesOriginal == 1 && cantidadBloquesNueva >1)
	{
		//busco un bloque libre para agregar como bloque de punteros
		log_info(filesystem_logger,"Se busca bloque libre para agregar los punteros a bloques");
		for(int i=0;i<config_get_int_value(superBloque, "BLOCK_COUNT");i++)
		{
			if (accesoBitmap(bitmap, i) == 0)
			{
				//Encontre un bloque vacio lo marco como ocupado
				setearBitmap(bitmap,i);
				config_set_value(configArchivoActual,"PUNTERO_INDIRECTO",string_itoa(i));
				config_save(configArchivoActual);
				punteroABloquePunteros = i;
				sincronizarBitmap();
				break;

			}
		}
		// Me ubico en el bloque de punteros
		log_info(filesystem_logger,"Acceso Bloque - Archivo: %s - Bloque Archivo: bloque de punteros - Bloque File System %d",config_get_string_value(configArchivoActual,"NOMBRE_ARCHIVO"),punteroABloquePunteros);
		fseek(bloques,punteroABloquePunteros * config_get_int_value(superBloque,"BLOCK_SIZE"),SEEK_SET);
		delay(config_get_int_value(config,"RETARDO_ACCESO_BLOQUE"));
		for (int i=0;i< cantidadBloquesAAGregar;i++)
		{
			int j=1;
			int posicion=0;
			while(j != 0)
			{
				j=accesoBitmap(bitmap, posicion);
				posicion++;
			}
			setearBitmap(bitmap, posicion -1);
			punteroACadaBloque = posicion - 1;
			fwrite(&punteroACadaBloque,sizeof(uint32_t),1,bloques);
		}
	}
	if(cantidadBloquesOriginal > 1)
	{
		log_info(filesystem_logger,"Acceso Bloque - Archivo: %s - Bloque Archivo: bloque de punteros - Bloque File System %d",config_get_string_value(configArchivoActual,"NOMBRE_ARCHIVO"),config_get_int_value(configArchivoActual,"PUNTERO_INDIRECTO"));
		fseek(bloques,config_get_int_value(configArchivoActual,"PUNTERO_INDIRECTO") * config_get_int_value(superBloque,"BLOCK_SIZE"),SEEK_SET);
		fseek(bloques,sizeof(uint32_t)*(cantidadBloquesOriginal - 1),SEEK_CUR);
		delay(config_get_int_value(config,"RETARDO_ACCESO_BLOQUE"));
		for (int i=0;i< cantidadBloquesAAGregar;i++)
		{
			int j=1;
			int posicion=0;
			while(j != 0)
			{
				j=accesoBitmap(bitmap, posicion);
						posicion++;
			}
			punteroACadaBloque = posicion - 1;
			fwrite(&punteroACadaBloque,sizeof(uint32_t),1,bloques);
		}
	}
	fclose(bloques);
	return;
}
*/
//..................................FUNCIONES UTILES PUNTEROS.....................................................................


/*
      __                                                      
     /  l                                                     
   .'   :               __.....__..._  ____                   
  /  /   \          _.-"        "-.  ""    "-.                
 (`-: .---:    .--.'          _....J.         "-.             
  """:     \,.'    \  __..--""       `+""--.     `.           
    :     .'/    .-"""-. _.            `.   "-.    `._.._     
    ;  _.'.'  .-j       `.               \     "-.   "-._`.   
    :    / .-" :          \  `-.          `-      "-.      \  
     ;  /.'    ;          :;               ."        \      `,
     :_:/      ::\        ;:     (        /   .-"   .')      ;
       ;-"      ; "-.    /  ;           .^. .'    .' /    .-" 
      /     .-  :    `. '.  : .- / __.-j.'.'   .-"  /.---'    
     /  /      `,\.  .'   "":'  /-"   .'       \__.'          
    :  :         ,\""       ; .'    .'      .-""              
   _J  ;         ; `.      /.'    _/    \.-"                  
  /  "-:        /"--.--..-'     .'       ;                    
 /     /  ""-..'            .--'.-'/  ,  :                    
:`.   :     / :             `--" ,',_:  _ \                   
:  \  '._  :__;             .'.-"; ; ; j `.l                  
 \  \          "-._         `"  :_/ :_/                       
  `.;\             "-._                                       
    :_"-._             "-.                                    
      `.  l "-.     )     `.                                  
        ""^--""^-. :        \                                 
                  ";         \                                
                  :           `._                             
                  ; /    \ `._   ""---.                       
                 / /   _      `.--.__.'                       
                : :   / ;  :".  \                             
                ; ;  :  :  ;  `. `.                           
               /  ;  :   ; :    `. `.                         
              /  /:  ;   :  ;     "-'                         
             :_.' ;  ;    ; :                                 
                 /  /     :_l                                 
                 `-'

*/