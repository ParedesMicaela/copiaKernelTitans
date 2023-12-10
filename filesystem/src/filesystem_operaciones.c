#include "filesystem.h"

bloque_swap* particion_swap;
char* swap_mapeado;
void* fat_mapeado;
uint32_t* tabla_fat_en_memoria;
int tam_bloque;

int proximo_bloque_inicial = 1;
static uint32_t buscar_bloque_en_FAT(uint32_t bloque_final, uint32_t bloque_inicial);
static void escribir_en_memoria(int tam_bloque, void* contenido, uint32_t direccion_fisica);
static void leer_contenido_archivo (uint32_t nro_bloque);
//============================================== INICIALIZACION ============================================

void levantar_fat()
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
            fclose(archivo_fat); //Por el truncar

            // Vuelve a abrir el archivo en modo "wb+" para truncarlo
            archivo_fat = fopen(path, "wb+");
            if (archivo_fat == NULL)
            {
                log_error(filesystem_logger, "Error al abrir el archivo FAT en modo escritura");
                return;
            }

            if (ftruncate(fileno(archivo_fat), tamanio_fat) == -1)
            {
                log_error(filesystem_logger, "Error al truncar la tabla FAT");
            }

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
    archivo_FCB->nombre_archivo = string_duplicate(config_get_string_value (archivo, "NOMBRE_ARCHIVO"));
    archivo_FCB->bloque_inicial = config_get_int_value(archivo, "BLOQUE_INICIAL");
    archivo_FCB->tamanio_archivo = config_get_int_value (archivo, "TAMANIO_ARCHIVO");

	config_destroy (archivo);
    free(path);
    return archivo_FCB;
}

FILE* levantar_archivo_bloque() {
	
    archivo_de_bloques = fopen(path_archivo_bloques, "rb+");

    if (path_archivo_bloques == NULL) {
        log_error(filesystem_logger, "No se pudo abrir el archivo.");
    }
	return archivo_de_bloques;
}

void crear_archivo_de_bloque()
{
	uint32_t fd;

    fd = open(path_archivo_bloques, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        log_error(filesystem_logger,"Error al abrir el Archivo de Bloques");
    }

  
    if (ftruncate(fd, tamanio_archivo_bloques) == -1) {
        log_error(filesystem_logger,"Error al truncar el Archivo de Bloques");
    }

    close (fd);
}

void mapear_archivo_de_bloques() {

    int fd_bloques = open(path_archivo_bloques, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd_bloques == -1) {
        perror("Error al abrir el archivo de bloques mapear");
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

void abrir_archivo (char *nombre_archivo, int* socket_kernel)
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

		//diccionario de archivos abiertos
		dictionary_put(diccionario_archivos_abiertos,nombre_archivo,archivo);
		log_info(filesystem_logger, "Guardamos el tipo FILE en el diccionario\n");

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

void truncar_archivo(char *nombre, int tamanio_nuevo, int socket_kernel)
{	
	int tamanio_archivos_bloques = config_valores_filesystem.tam_bloque * config_valores_filesystem.cant_bloques_total;
	int cant_espacio_swap = config_valores_filesystem.cant_bloques_swap;
	int cant_espacio_fat = tamanio_archivos_bloques - cant_espacio_swap;

	fcb* fcb_a_truncar = levantar_fcb(nombre);

	log_info(filesystem_logger,"Truncando archivo: Nombre %s, tamanio %d, Bloque inicial %d \n",fcb_a_truncar->nombre_archivo, fcb_a_truncar->tamanio_archivo, fcb_a_truncar->bloque_inicial);

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
	else{
		printf("Bueno bueno, para... No se puede hacer eso\n");
		abort();
	}
	
	//actualizamos la estructura fcb y la cargamos al archivo
	fcb *nuevo_fcb = malloc(sizeof(fcb)); //inicializo
	nuevo_fcb->nombre_archivo = nombre;
	nuevo_fcb->tamanio_archivo = tamanio_nuevo;
	nuevo_fcb->bloque_inicial = calcular_bloque_inicial_archivo(tamanio_nuevo); //no está definido, mejor dicho no se lo pasan como parametro -> correcto, precisamente nosotros al crear un archivo, sin bloque inicial asignado, al truncarlo le vamos a pasar a asignar un bloque
	actualizar_fcb(nuevo_fcb);
	free(nuevo_fcb);
	log_info(filesystem_logger,"Terminamos y mandamos el fcb a actualizarse\n");

	int truncado = 1;
    send(socket_kernel, &truncado, sizeof(int), 0);
}

int calcular_bloque_inicial_archivo(int tamanio)
{
	int devolver = proximo_bloque_inicial;
	proximo_bloque_inicial+=tamanio;
	return devolver;
}

void escribir_archivo(char* nombre_archivo, uint32_t puntero_archivo, void* contenido){
	
	fcb* archivo_a_leer = levantar_fcb (nombre_archivo);

	uint32_t bloque_inicial = archivo_a_leer->bloque_inicial; 
	int tamanio_archivo = archivo_a_leer->tamanio_archivo;
	int tam_bloque = config_valores_filesystem.tam_bloque;
	uint32_t bloque_a_escribir = puntero_archivo/tam_bloque;
	free(archivo_a_leer);

	escribir_contenido_en_bloque(bloque_a_escribir,bloque_inicial, contenido);
}


void escribir_contenido_en_bloque(uint32_t bloque_a_escribir, uint32_t bloque_inicial, void* contenido) {
	
	uint32_t nro_bloque = buscar_bloque_en_FAT(bloque_a_escribir, bloque_inicial);

	leer_contenido_archivo(nro_bloque);
}

static uint32_t buscar_bloque_en_FAT(uint32_t cantidad_bloques_a_leer, uint32_t bloque_inicial)
{
	uint32_t nro_bloque = 0;

	int retardo = config_valores_filesystem.retardo_acceso_fat;

	uint32_t indice = bloque_inicial; 

	for (int i = 0; i < cantidad_bloques_a_leer; i++) {
		usleep(1000* retardo);
		cantidad_bloques_a_leer = tabla_fat_en_memoria[i];
	}

	//El indice que cumple la condicion, es donde está el dato
	nro_bloque = cantidad_bloques_a_leer;

	leer_contenido_archivo(nro_bloque);
	
	return nro_bloque;
}

static void leer_contenido_archivo (uint32_t nro_bloque) {

	void* buffer = NULL;
	int retardo = config_valores_filesystem.retardo_acceso_bloque;

	uint32_t direccion_bloque = tam_bloque * nro_bloque;

	uint32_t offset = direccion_bloque + tamanio_swap;
	
	//Abrimos archivo
	archivo_de_bloques = levantar_archivo_bloque();

	//Nos posicionamos en el dato
	fseek(archivo_de_bloques, offset, SEEK_SET);

	void* buffer = malloc(tam_bloque);

	usleep(1000* retardo);
	fread(buffer, tam_bloque, 1, archivo_de_bloques);

	mem_hexdump(buffer, sizeof(buffer));

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
void leer_archivo(char *nombre_archivo, uint32_t puntero_archivo, uint32_t direccion_fisica, int cliente_fd)
{	
	uint32_t dato_a_copiar;
	int tamanio;
	void* buffer = NULL;

	fcb* archivo_a_leer = levantar_fcb (nombre_archivo);

	uint32_t bloque_inicial = archivo_a_leer->bloque_inicial; 
	int tamanio_archivo = archivo_a_leer->tamanio_archivo;
	int tam_bloque = config_valores_filesystem.tam_bloque;
	uint32_t bloque_final = puntero_archivo/tam_bloque;
	
	free(archivo_a_leer);
	
	//recorrer_tabla_fat(bloque_inicial,bloque_final, tam_bloque, direccion_fisica, cliente_fd);
	
	//El dato que se encuentra adentro del bloque final
	//dato_a_copiar = buscar_bloque_en_FAT(bloque_final, bloque_inicial);

	tamanio = dato_a_copiar * tam_bloque;

	//Creo un buffer temporal
	buffer = malloc(tam_bloque); 
		    
	//Copio los datos desde el archivo mapeado al nuevo buffer 
	memcpy(buffer, (char*)fat_mapeado + tamanio, tamanio);

	//Mandamos el contenido(buffer) a que se persista en memoria
	escribir_en_memoria(tam_bloque, buffer, direccion_fisica);

	//Avisa al kernel que terminó
	int ok_read = 1;
    send(cliente_fd, &ok_read, sizeof(int), 0);
}

static void escribir_en_memoria(int tam_bloque, void* contenido, uint32_t direccion_fisica) {
	t_paquete *paquete = crear_paquete(ESCRIBIR_EN_MEMORIA); 
	agregar_entero_a_paquete(paquete, tam_bloque);
	agregar_bytes_a_paquete(paquete,contenido,tam_bloque); //Revisar dsps 
	agregar_entero_sin_signo_a_paquete(paquete,direccion_fisica); 
	enviar_paquete(paquete,socket_memoria);
	eliminar_paquete(paquete);
	free(contenido);
}

void cerrar_archivo(char* nombre_archivo)
{
	FILE* archivo = dictionary_get(diccionario_archivos_abiertos,nombre_archivo);

	fclose(archivo);

	log_info(filesystem_logger, "Cerramos el archivo\n");
}
//============================================= ACCESORIOS DE ARCHIVOS =================================================================

void actualizar_fcb(fcb* nuevo_fcb)
{
	char * path = string_from_format ("%s/%s.fcb", config_valores_filesystem.path_fcb, nuevo_fcb->nombre_archivo);

    t_config * archivo = config_create (path);
	
	int tamaño_buffer_bloque = snprintf(NULL, 0, "%d", nuevo_fcb->bloque_inicial) + 1;
    char *bloque = (char *)malloc(tamaño_buffer_bloque * sizeof(char));
    if (bloque == NULL)perror("Error: no se pudo asignar memoria para el bloque.\n");
    sprintf(bloque, "%d", nuevo_fcb->bloque_inicial);
	log_info(filesystem_logger,"El bloque inical seria %s\n", bloque);

	int tamaño_buffer_tamanio = snprintf(NULL, 0, "%d", nuevo_fcb->tamanio_archivo) + 1;
    char *tamanio = (char *)malloc(tamaño_buffer_tamanio * sizeof(char));
    if (tamanio == NULL)perror("Error: no se pudo asignar memoria para el bloque.\n");
    sprintf(tamanio, "%d", nuevo_fcb->tamanio_archivo);
	log_info(filesystem_logger,"El tamanio seria %s\n", tamanio);
	
	config_set_value(archivo, "NOMBRE_ARCHIVO", nuevo_fcb->nombre_archivo);
	config_set_value(archivo, "BLOQUE_INICIAL", bloque);
	config_set_value(archivo, "TAMANIO_ARCHIVO", tamanio);
	
	free(bloque);
	free(tamanio);

	config_remove_key(archivo,"CANT_BLOQUES_SWAP");
	config_remove_key(archivo,"RETARDO_ACCESO_BLOQUE");
	config_remove_key(archivo,"CANT_BLOQUES_TOTAL");
	config_remove_key(archivo,"PATH_BLOQUES");
	config_remove_key(archivo,"PATH_FAT");
	config_remove_key(archivo,"RETARDO_ACCESO_FAT");
	config_remove_key(archivo,"TAM_BLOQUE");
	config_remove_key(archivo,"PUERTO_ESCUCHA");
	config_remove_key(archivo,"PATH_FCB");
	config_remove_key(archivo,"IP_MEMORIA");
	config_remove_key(archivo,"PUERTO_MEMORIA");
	
	config_save_in_file(archivo,path);

	free(path);
	config_destroy(archivo);
	log_info(filesystem_logger,"Actualizarmos bien el fcb\n");
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
		posicion_bloque_agregado < bloques_a_agregar;
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
			
			FILE* archivo_fat = fopen(config_valores_filesystem.path_fat, "wb+");
			fseek(archivo_fat,posicion_bloque_agregado,SEEK_SET); //Revisar dsps
			
			//bloques
			archivo_de_bloques = levantar_archivo_bloque();
			int posicion_archivo_bloques = cant_espacio_swap + posicion_bloque_agregado;
			fseek(archivo_de_bloques,posicion_archivo_bloques,SEEK_SET); //Revisar dsps

			//ACTUALIZAMOS LOS DOS ARCHIVOS
			if(archivo_fat != NULL && archivo_de_bloques != NULL)
			{
				//------------------ACTUALIZAMOS TABLA FAT-----------------//
				if(( posicion_bloque_agregado + 1 ) == posicion_ultimo_bloque ) 
				//El que tenemos creado en realidad seria el anteultimo y el siguiente el ultimo
				{
					//escribimos el anteultimo
					fwrite(nuevo_ultimo_bloque_fat, sizeof(uint32_t), 1, archivo_fat);
					//escribimos ahora si el ultimo bloque
					//fwrite(UINT32_MAX, sizeof(uint32_t), 1, archivo_fat); //escribimos el ultimo bloque
					uint32_t max_value = UINT32_MAX;
					fwrite(&max_value, sizeof(uint32_t), 1, archivo_fat);
					free(nuevo_ultimo_bloque_fat);
					//free(nuevo_ultimo_bloque);
					log_info(filesystem_logger,"Actualizamos el ultimo bloque de archivo fat\n");
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
					//nuevo_ultimo_bloque_bloques->data = "\0"; 
					fwrite(nuevo_ultimo_bloque_bloques, tam_bloque, 1, archivo_de_bloques);
					//nuevo_ultimo_bloque_bloques->data = UINT32_MAX; 
					fwrite(nuevo_ultimo_bloque_bloques, tam_bloque, 1, archivo_de_bloques);
					
				}
				else
				//este no es el ultimo
				{
					//nuevo_ultimo_bloque_bloques->data = "\0"; 
					fwrite(nuevo_ultimo_bloque_bloques, tam_bloque, 1, archivo_de_bloques);
				}
				
				//fflush(archivo_fat);
				//fflush(archivo_de_bloques);

			}else printf("no existe/no se pudo abrir la tabla fat o el archivo de bloques"); 
	
			fclose(archivo_fat);

			fclose(archivo_de_bloques);
			//msync(fat_mapeado, espacio_de_FAT, MS_SYNC);
			//msync(tabla_fat_en_memoria, tamanio_fat * sizeof(uint32_t), MS_SYNC);

		}	
		

	}
	log_info(filesystem_logger,"En teoria termino el for y se crearon los bloques\n");
}

void reducir_tamanio_archivo (int nuevo_tamanio_archivo, fcb* fcb_archivo)
{
	int tamanio_archivos_bloques = config_valores_filesystem.tam_bloque * config_valores_filesystem.cant_bloques_total;
	int cant_espacio_swap = config_valores_filesystem.cant_bloques_swap - 1;
	int cant_espacio_fat = tamanio_archivos_bloques - cant_espacio_swap;

	//calculamos cantidad a agregar //en realidad a sacar uis
	//int bloques_a_agregar = nuevo_tamanio_archivo - fcb_archivo->tamanio_archivo;
	int bloques_a_quitar = nuevo_tamanio_archivo - fcb_archivo->tamanio_archivo;
	
	int posicion_primer_bloque_a_quitar = fcb_archivo->bloque_inicial + bloques_a_quitar;

	for
	(
		int posicion_bloque_agregado = 0;
		posicion_bloque_agregado < bloques_a_quitar;
		posicion_bloque_agregado++
	)
	{


	FILE *archivo_fat = fopen(config_valores_filesystem.path_fat, "rb+");
    archivo_de_bloques = levantar_archivo_bloque(); //al reducir el tamaño, no hace falta abrirlos en escritura

        if (archivo_fat == NULL || archivo_de_bloques == NULL)
        {
            perror("No se pudo abrir la tabla fat o el archivo de bloques");
            exit(EXIT_FAILURE);
        }

	  // actualizar tabla FAT
        if (posicion_bloque_agregado == bloques_a_quitar - 1)
        {
            // el último bloque que vamos a quitar
            fseek(archivo_fat, posicion_primer_bloque_a_quitar * sizeof(uint32_t), SEEK_SET);
            uint32_t bloque_siguiente = UINT32_MAX;
            fwrite(&bloque_siguiente, sizeof(uint32_t), 1, archivo_fat);
        }
        else
        {
            // no es el último bloque que vamos a quitar
            fseek(archivo_fat, (posicion_primer_bloque_a_quitar + posicion_bloque_agregado) * sizeof(uint32_t), SEEK_SET);
            uint32_t bloque_siguiente = posicion_primer_bloque_a_quitar + posicion_bloque_agregado + 1;
            fwrite(&bloque_siguiente, sizeof(uint32_t), 1, archivo_fat);
        }

        // actualizar archivo de bloques
        fseek(archivo_de_bloques, (cant_espacio_swap + posicion_primer_bloque_a_quitar + posicion_bloque_agregado) * tam_bloque, SEEK_SET);
        bloque_swap bloque_vacio;
        //bloque_vacio.data = "\0";
        fwrite(&bloque_vacio, tam_bloque, 1, archivo_de_bloques);

        // cerrar archivos
        fclose(archivo_fat);
        fclose(archivo_de_bloques);

		//msync(fat_mapeado, espacio_de_FAT, MS_SYNC);
		//msync(tabla_fat_en_memoria, tamanio_fat * sizeof(uint32_t), MS_SYNC);
    }

    log_info(filesystem_logger, "Se redujo el tamaño del archivo\n");

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

