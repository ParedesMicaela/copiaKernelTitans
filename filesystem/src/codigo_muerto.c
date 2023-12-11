/*
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

void actualizar_bloque_fat(char* nombre_archivo, int numero_bloque, uint32_t direccion_bloque, int numero_bloque_a_reemplazar)
{
	t_bloque* datos_bloque = malloc(sizeof(t_bloque));
	datos_bloque->nombre_archivo = nombre_archivo;
	datos_bloque->numero_bloque = numero_bloque;
	datos_bloque->direccion_bloque = direccion_bloque;
	datos_bloque->puntero_siguiente_bloque = malloc(sizeof(uint32_t));
	
	//todo esto de aca creo que se puede hacer con un t_dictionari igual... pienso
	for (size_t i = 1; i < tamanio_fat; i++) // empezamos en el 1 porque el 0 esta reservado
	{
		t_bloque* bloque_comparado = list_get(tabla_fat,i);
		if(datos_bloque->nombre_archivo == (bloque_comparado->nombre_archivo))
		{
			//no se si reemplaza o borrra y vuelve a abrir, esto revisar porque el tema memoria puede ser jodido
			//list_replace(tabla_fat,i,datos_bloque);
			list_replace_and_destroy_element(tabla_fat,i,datos_bloque,(void*)liberar_bloque(bloque_comparado));
			break;
		}
	}
}



int escribirArchivo(char *nombreArchivo, uint32_t *puntero_archivo, size_t bytesAEscribir, void *memoriaAEscribir)
{ 
	//abrir la tabla de fat buscar el block number y agarrar la direccino de ese bloque
	
	//vamos a ese numero de bloque	
	
	//escbirir el bloque con lo que nos mandaron
    /*
	uint32_t end_of_file = UINT32_MAX; 
    fseek(archivo_fat, block_number * bytesAEscribir, SEEK_SET);
    fwrite(&memoriaAEscribir, bytesAEscribir, 1, archivo_fat);
    fflush(archivo_fat)
}


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


/* TO DO terminar/revisar bien truncar_archivo
int truncar_archivo (fcb* archivo, uint32_t tamanio_archivo) {
    // Cantidad de bloques asignados al archivo de antemano. 
    int cantBloquesAsignados = CANT_BLOQUES (archivo->tamanio_archivo);
    // Cantidad de bloques a asignar.
    int cantBloquesAAsignar = CANT_BLOQUES (tamanio_archivo);
    // Si ya se asignaron los bloques necesarios terminar.
    if (cantBloquesAsignados == cantBloquesAAsignar) {
        archivo->tamanio_archivo = tamanio_archivo;
        return 0;
    }
    // Si la cantidad requerida es mayor a la que permite acceder el puntero indirecto y el puntero directo terminar con error.
    //if (cantBloquesAAsignar > tamanioBloques / TAMANIO_PUNTERO + 1) return -7;

    //debug ("Variables: %d %d %d %d", cantBloquesAsignados, cantBloquesAAsignar, tamanio, archivo->tamanio);
    
    // Caso 1: Hay que asignar bloques.
    if (tamanio_archivo > archivo->tamanio_archivo) {
        // Si no hay bloques asignados, se le asigna un bloque directo y uno de punteros (indirecto).
        if (!archivo->tamanio_archivo) {
            //archivo->ptrDirecto = proximoBloqueLibre (),
            cantBloquesAsignados++;
            //archivo->ptrIndirecto = proximoBloqueLibre ();
            // Si no hay bloques disponibles para asignar, se termina con error.
            //if (archivo->ptrDirecto == UINT32_MAX || archivo->ptrIndirecto == UINT32_MAX) return -5;
            //archivo->tamanio = tamanioBloques;
        }

    
        

        // Ciclo: Termina cuando se asignaron los bloques requeridos.
        while (cantBloquesAsignados < cantBloquesAAsignar) { 
            uint32_t proxBloque = proximoBloqueLibre ();
            // Si se falla al copiar el puntero del bloque libre al puntero indirecto, se termina con error.
            if (asignarBloqueAArchivo (archivo, proxBloque) < 0) return -3;
            cantBloquesAsignados++, archivo->tamanio += tamanioBloques;
        }
    }

    // Caso 2: Hay que eliminar bloques.
    else {
        // Eliminar bloques si hay en el puntero indirecto y hasta el anteultimo.
        uint32_t ultBloque = ultimoBloqueDeArchivo (archivo);
        while (cantBloquesAsignados > cantBloquesAAsignar && ultBloque != archivo->ptrDirecto) {
            //debug ("Variables: %d %d %d %d", cantBloquesAsignados, cantBloquesAAsignar, ultBloque, archivo->tamanio);
            eliminarBloque (ultBloque);
            //debug ("Bloque %d eliminado.", ultimoBloqueDeArchivo (archivo));
            eliminarPtr (archivo, cantBloquesAsignados - 2);
            cantBloquesAsignados--, archivo->tamanio -= tamanioBloques;
            ultBloque = ultimoBloqueDeArchivo (archivo);
        }

        // Segun cada caso, el ultimo puede ser el puntero directo o uno del puntero indirecto.
        if (cantBloquesAAsignar == 0 && archivo->tamanio > 0) {
            eliminarBloque (archivo->ptrDirecto);
            archivo->ptrDirecto = 0;
            eliminarBloque (archivo->ptrIndirecto);
            archivo->ptrIndirecto = 0;
        }
    }
    archivo->tamanio = tamanio;
    msync(ptrBloques, tamanioBloques * cantBloques, MS_SYNC);
    msync(ptrBitMap, tamanioBitmap, MS_SYNC);
    if (actualizarFCB (archivo) < 0) return -4;
    return 0;

}
*/
  //MARTINCHO TO DO: ACORDATE DE METER LOS CASES EN LAs FUNCIONES......LINEA 47 ARRANCA

//..................................FUNCIONES UTILES ARCHIVOS.....................................................................
/*
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
*/
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

//eto creo que no hace falta pero lo dejo por las dudas
//esto no anda y no se porque mierda
int devolver_tamanio_de_fcb(char* nombre)
{
    char * path = string_from_format ("%s/%s.fcb", config_valores_filesystem.path_fcb, nombre);

    t_config * archivo = config_create (path);

    fcb* archivo_FCB = malloc (sizeof (fcb)); 
    archivo_FCB->nombre_archivo = config_get_string_value (archivo, "NOMBRE_ARCHIVO");
    archivo_FCB->bloque_inicial = config_get_int_value(archivo, "BLOQUE_INICIAL");
    archivo_FCB->tamanio_archivo = config_get_int_value (archivo, "TAMANIO_ARCHIVO");

	int tamanio = archivo_FCB->tamanio_archivo;

	free(archivo_FCB);
    config_destroy (archivo);
    free(path);
    return tamanio;
}

//es lo mismo que la de arriba, falta optimizar 
int devolver_bloque_inicial_de_fcb(char* nombre)
{
	//creamos el comandito pa entrar al directorio y empezar a buscar el archivo
	char comando[256];

	//lo tengo que buscar como nombre.dat porque sino no me lo va a encontrar
	char* nombre_archivo = string_from_format("%s.dat", nombre);
	char *path_archivo = string_from_format("%s/%s.dat", config_valores_filesystem.path_fcb, nombre);

	//le agregue la parte del /home/utns/tp.../ porque sino no andaba pero me parece que hay que hacerlo mas generico como lo hicimos en la linea 415
	snprintf(comando, sizeof(comando), "ls -1 %s", "/home/utnso/tp-2023-2c-KernelTitans/filesystem/fs/fcbs"); //carpeta de lo fcbs
	FILE *tuberia_conexion = popen(comando, "r");

    if (tuberia_conexion != NULL) {
        
		char nombre_archivo_buscado[256];

        while (fscanf(tuberia_conexion, "%255s", nombre_archivo_buscado) != EOF) //no creo que tenga mas de 255
		{
            if (strcmp(nombre_archivo, nombre_archivo_buscado) == 0)
			{
                printf("Se encontró el archivo: %s\n", nombre_archivo); //LLEGUE HASTA ACA, TIRA SEG FAULT PERO VA ANDANDO
				
				//creamos config para guardar datos en una estructura y mandarla. En el config_create va el path
				t_config* config = config_create(path_archivo);
				

				//agarramos los datos del archivo y los guardamos en un fcb
				char* nombre = config_get_string_value(config, "NOMBRE_ARCHIVO");
				int tamanio = (int*)config_get_string_value(config, "TAMANIO_ARCHIVO");
				int bloque_inicial = config_get_int_value(config, "BLOQUE_INICIAL");

				//guardamos datos en esturctura
				
				pclose(tuberia_conexion);		
				
				return bloque_inicial; //despues sacamos lo que no usa
            }
        }
		printf("No encontró el archivo %s, se llego al final\n", nombre_archivo);
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


*/
//..................................FUNCIONES UTILES PUNTEROS....................................................................