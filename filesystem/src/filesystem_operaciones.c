#include "filesystem.h"

bloque_swap* particion_swap;
//============================================== INICIALIZACION ============================================
void levantar_fat(size_t tamanio_fat)
{    
	char* path = config_valores_filesystem.path_fat;
	uint32_t tamanio_bloque = config_valores_filesystem.tam_bloque;
    bloque_swap* archivo_fat = fopen(path, "rb+");

	uint32_t tam_entrada = tamanio_fat / sizeof(uint32_t);

	tabla_fat = list_create();

	//si no existe el archivo fat, lo abre y pone su valor en 0
	if (archivo_fat == NULL)
	{
		archivo_fat = fopen(path, "wb+");

		uint32_t valor_inicial = 0; //Revisar que no sea while(!UINT32_MAX)
	    for (size_t i = 1; i < tamanio_fat; i++) // ver que dicen en issue, si queda uint32 o bloque_swap o tamanio que nos den
		{
			fwrite(&valor_inicial, sizeof(tam_entrada), 1, archivo_fat);
        }
        fflush(archivo_fat);
    } else 
	{
		log_info(filesystem_logger,"Comenzamos a levantar la tabla fat, que no esta vacia \n");

		for (size_t i = 0; i < tamanio_fat; i++)
		{	
			//asumo que lo que querias hacer aca era leer el archivo_fat y guardarlo en entrada_fat. Puse sizeof bloque_swap y le saque el puntero&
			
			uint32_t* entrada_fat = malloc(sizeof(uint32_t));
			fread(entrada_fat, sizeof(bloque_swap), 1, archivo_fat);

			//guardo la entrada_fat en la tabla de fat
			list_add(tabla_fat, entrada_fat);
        }
		//en teoria con esto tenemos la tabla llena, a modo de lista
        fflush(archivo_fat);
    }
    fclose(archivo_fat);

}

void levantar_archivo_bloque(size_t tamanio_swap, size_t tamanio_fat)
{
    int tamanio_bloque = config_valores_filesystem.tam_bloque;
	char* path = config_valores_filesystem.path_bloques;
	int cant_bloques_swap = config_valores_filesystem.cant_bloques_swap;

    FILE* archivo_bloque = fopen(path, "rb"); // Leemos en modo binario, puede ser wb+

	lista_bloques_swap = list_create();
    
	log_info(filesystem_logger, "Se abrio el archivo de bloques, rellenamos estructuras\n");

	// Le asignamos un espacio de memoria al swap (tipo swap creo)
	particion_swap = malloc(sizeof(tamanio_swap));

	//rellenamos lista con datos de archivo
	for (size_t i = 0; i <= cant_bloques_swap; i++)
	{		
		bloque_swap* dato_bloque = malloc(sizeof(bloque_swap));

		fread(dato_bloque, sizeof(bloque_swap), 1, archivo_bloque);
		list_add_in_index(lista_bloques_swap,i,(void*)dato_bloque);
	}
	log_info(filesystem_logger, "Se aloco la memoria para el swap y se relleno estructura\n");

	// Le asignamos un espacio de memoria al fat
	uint32_t* particion_fat = malloc(sizeof(tamanio_fat));

	//rellenamos lista con datos de archivo
	for (size_t i = 0; i <= tamanio_fat; i++)
	{		
		uint32_t* dato_bloque = malloc(sizeof(uint32_t));
		if(i==0){
			bitarray_set_bit(bitmap_archivo_bloques, i);
		}
		list_add_in_index(tabla_fat,i,dato_bloque);
	}
	log_info(filesystem_logger, "Se aloco la memoria para el fat y se relleno estructura\n");

	fclose(archivo_bloque);
    log_info(filesystem_logger, "Se levanto archivo de bloque\n");
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
		perror("Archivo de configuracion de fcb no encontrado \n");
		abort();
	  }
}


//================================================= OPERACIONES ARCHIVOS ============================================
void crear_archivo (char *nombre_archivo, int socket_kernel) //literalmente lo unico que funciona
{
    char *path_archivo = string_from_format ("%s/%s.dat", config_valores_filesystem.path_fcb, nombre_archivo);
	
	FILE *archivo = fopen(path_archivo, "w"); //en cambio ahí sí lo crea
	fclose(archivo);		

	//creamos la config para manejar el archivo mas facil
	t_config *archivo_nuevo = config_create (path_archivo);
	
	if (archivo_nuevo!=NULL)
	{
		//nos guardamos la direccion de memoria del archivos (uint por el envio de paquete, por las dudas pa que no rompa)
		uint32_t direccion = (uint32_t)archivo;
		
		//ahora tenemos que saber que bloque le asignamos, por lo que vamos a la ultima entrada de la tabla fat
		int ultimo_bloque_fat = list_size(tabla_fat);
		if(ultimo_bloque_fat==0){
			printf("El ultimo bloque es el 0 y este esta reservado, se asiga bloque inicial 1");
			ultimo_bloque_fat = 1;
		}
		
		//ENUNCIADO En la operación crear archivo, se deberá crear un archivo FCB con tamaño 0 y sin bloque inicial.
		
		// Convertir el entero a una cadena de caracteres con snprintf
		char* ultimo_bloque_fat_string;
		snprintf(ultimo_bloque_fat_string, sizeof(char*), "%d", ultimo_bloque_fat);

		config_set_value(archivo_nuevo, "NOMBRE_ARCHIVO", nombre_archivo);
		config_set_value(archivo_nuevo, "BLOQUE_INICIAL", ultimo_bloque_fat_string);
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
	char *path_archivo = string_from_format("%s/%s.dat", config_valores_filesystem.path_fcb, nombre_archivo);
	
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
	//copiamos en un tipo fcb los datos del archivo con la direccion
	int tamanio = devolver_tamanio_de_fcb(nombre);
	int bloquecito_inicial = devolver_bloque_inicial_de_fcb(nombre);
	
	fcb fcb_archivo;
	fcb_archivo.nombre_archivo = nombre;
	fcb_archivo.tamanio_archivo = tamanio;
	fcb_archivo.bloque_inicial = bloquecito_inicial;
				
	//comparar el tamanio del archivo actual con el nuevo
	if(fcb_archivo.tamanio_archivo < tamanio_nuevo)
	{
		ampliar_tamanio_archivo(tamanio_nuevo, fcb_archivo);
	}
	else if (fcb_archivo.tamanio_archivo > tamanio_nuevo)
	{
		reducir_tamanio_archivo(tamanio_nuevo, fcb_archivo);
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

//idem que leer, tengo que tocar acá
void escribir_archivo(char* nombre_archivo, uint32_t *puntero_archivo, void* contenido, uint32_t* direccion_fisica){

	FILE *archivo;
	char *path_archivo = string_from_format("%s/%s.dat", config_valores_filesystem.path_fcb, nombre_archivo);

  	archivo = fopen(path_archivo, "wb"); //abro en modo escritura
    	if (archivo == NULL) {
        	perror("Error al abrir el archivo");
        	//free(contenido); esto seguro al final
        	return;
    	}

	if (fwrite(contenido, 1, sizeof(contenido), archivo) != sizeof(contenido)) { //escribo el contenido en el archivo o mejor dicho... el contenido que es lo que quieren que escriba en el archivo lo escribe
        perror("Error al escribir en el archivo");
        //free(contenido); idem linea 781 pero pensándolo bien seguro lo deje
        fclose(archivo);
        return;
    }
 
    fclose(archivo); 
    
	int ok_read = 1;
	send(socket_kernel, &ok_read, sizeof(int), 0);

}

void *leer_archivo(char *nombre_archivo,uint32_t direccion_fisica, int socket_kernel)
{	
	char *path_archivo = string_from_format("%s/%s.dat", config_valores_filesystem.path_fcb, nombre_archivo);
	FILE *archivo = fopen(path_archivo, "r"); // lo abrimos en modo lectura 

	if (archivo == NULL){
		perror("No se puede leer el archivo");
		return NULL;
	}
	
	fseek(archivo, 0, SEEK_SET); 

	int tamanio = (int*)config_get_string_value(config, "TAMANIO_ARCHIVO"); //el tamanio lo guardamos como un string porque no hay otra forma de hacerlo (linea 392). Lo convierto a int aca

	void *contenido;
	contenido = (void*)malloc(tamanio + 1); // +1 por el \0
	if (contenido == NULL) {
        perror("Error para guardar el contenido");
        fclose(archivo);
        return NULL;
    }

    if (fread(contenido, 1, tamanio, archivo) != tamanio) {
        perror("Error al leer el archivo");
        fclose(archivo);
        return NULL;
    }
	
	fclose(archivo);

	t_paquete *paquete = crear_paquete(ESCRIBIR_EN_MEMORIA); 
	agregar_bytes_a_paquete(paquete,contenido,32); //ojo después hay que ver bien el tema del tamaño para que sea algo variable como nos dijo Dami 
	agregar_entero_sin_signo_a_paquete(paquete,direccion_fisica); 
	enviar_paquete(paquete,socket_memoria);
	eliminar_paquete(paquete);
    free(contenido); 

	//esperamos que memoria me confirme que lo escribio
	int escritura_ok = 0;
	recv(socket_memoria, &escritura_ok, sizeof(int), 0);

	if (escritura_ok != 1)
	{
	printf("No se pudo escribir en memoria :(\n");
	}else{

		//le avisamos al kernel que ya la memoria escribio el contenido del archivo
		int ok_read = 1;
        send(socket_kernel, &ok_read, sizeof(int), 0);
	}


	/*
    Lectura (solo de un solo bloque)
    uint32_t valor;
    fseek(archivo_fat, block_number * bytesAEscribir, SEEK_SET);
    fread(&valor, bytesAEscribir, 1, archivo_fat);

	Escribir Archivo
		Se deberá solicitar al módulo Memoria la información que se encuentra a partir de la dirección física recibida y se escribirá en el bloque correspondiente del archivo a partir del puntero recibido.
		Nota: El tamaño de la información a leer/escribir de la memoria coincidirá con el tamaño del bloque / página. Siempre se leerá/escribirá un bloque completo, los punteros utilizados siempre serán el 1er byte del bloque o página.
	*/
}

//============================================= ACCESORIOS DE ARCHIVOS =================================================================

void actualizar_fcb(fcb* nuevo_fcb)
{
	char* direccion_archivo = devolver_direccion_archivo(nuevo_fcb->nombre_archivo);
	
	t_config* config = config_create(direccion_archivo);

	config_set_value(config, "NOMBRE_ARCHIVO", nuevo_fcb->nombre_archivo);
	config_set_value(config, "BLOQUE_INICIAL", nuevo_fcb->bloque_inicial);
	config_set_value(config, "TAMANIO_ARCHIVO", nuevo_fcb->tamanio_archivo);
	config_save_in_file(config,direccion_archivo);
	config_destroy(config);
	printf("actualizarmos bien el fcb");
}

//AMPLIAR Y REDUCIR NICO
void ampliar_tamanio_archivo (int nuevo_tamanio_archivo, fcb fcb_archivo)
{
	//calculamos cantidad a agregar
	int bloques_a_agregar = nuevo_tamanio_archivo - fcb_archivo.tamanio_archivo;
	
	int bloques_totales = fcb_archivo.bloque_inicial + fcb_archivo.tamanio_archivo;

	for
	(
		int bloque_agregado = bloques_totales;
		bloque_agregado <= (bloques_totales + bloques_a_agregar);
		bloque_agregado++
	)
	{
		uint32_t* nuevo_ultimo_bloque = (uint32_t*)malloc(sizeof(config_valores_filesystem.tam_bloque));
		
		//definir el puntero
		if (nuevo_ultimo_bloque == NULL)
		{
			perror("No se pudo alocar memoria para hacer la entrada fat\n");
			abort();
		}
		else
		{
			//futuras pruebas ftell(archivo) dice donde estas parado
		
			//no creo que esten abiertos
			//en el fat ponemos el puntero, porque es el mapa
			FILE* archivo_fat = fopen(config_valores_filesystem.path_fat, "wb+");
			fseek(archivo_fat,bloque_agregado,SEEK_SET);

			//en el archivo de bloques si ponemos el dato, que por ahora es nada
			FILE* archivo_bloques = fopen(config_valores_filesystem.path_bloques, "wb+");
			int posicion_archivo_bloques = list_size(lista_bloques_swap) + list_size(tabla_fat);
			fseek(archivo_fat,posicion_archivo_bloques,SEEK_SET); // revisar

			if(archivo_fat != NULL && archivo_bloques != NULL)
			{
				//actualizamos archivo fat
				if(bloque_agregado == (bloques_totales + bloques_a_agregar) ) //es el bloque final
				{
					fwrite(UINT32_MAX, sizeof(uint32_t), 1, archivo_fat);
				}
				else
				{
					fwrite(&nuevo_ultimo_bloque, sizeof(uint32_t), 1, archivo_fat);
				}
				
				//actualizamos la tabla local fat
				list_add(tabla_fat,nuevo_ultimo_bloque);

				//actualizamos lista de bloques local
				bloque_swap* nuevo_bloque_fat;
				nuevo_bloque_fat->data = NULL; //es nuevo, no tiene nada
				list_add(tabla_fat,nuevo_bloque_fat);
				
				//actualizamos archivo_bloque (esta bien asi? quiero guardar el dato de la estructura)
				if(bloque_agregado == (bloques_totales + bloques_a_agregar) ) //es el bloque final
				{
					nuevo_bloque_fat->data = UINT32_MAX;
					fwrite(nuevo_bloque_fat, sizeof(uint32_t), 1, archivo_fat);
				}
				else
				{
					fwrite(&nuevo_bloque_fat, sizeof(bloque_swap*), 1, archivo_fat);
				}
				
				fflush(archivo_fat);
				fflush(archivo_bloques);

			}else printf("no existe/no se pudo abrir la tabla fat o el archivo de bloques"); 
	
			fclose(archivo_fat);
			fclose(archivo_bloques);
			
		}	
	}
	printf("en teoria termino el for y se crearon los bloques");

	
}

void reducir_tamanio_archivo (int nuevo_tamanio_archivo, fcb fcb_archivo)
{
	int bloques_a_quitar = fcb_archivo.tamanio_archivo - nuevo_tamanio_archivo;
	
	int bloques_totales = fcb_archivo.bloque_inicial + fcb_archivo.tamanio_archivo;

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


//eto creo que no hace falta pero lo dejo por las dudas
int devolver_tamanio_de_fcb(char* nombre)
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
				
				return tamanio; //despues sacamos lo que no usa
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

