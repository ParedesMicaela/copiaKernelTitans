#include "filesystem.h"

bloque_swap* particion_swap;
char* swap_mapeado;
void* fat_mapeado;
uint32_t* tabla_fat_en_memoria;
int tam_bloque;
FILE* archivo_tabla_fat;

static uint32_t buscar_bloque_en_FAT(uint32_t bloque_final, uint32_t bloque_inicial);
static void escribir_en_memoria(int tam_bloque, void* contenido, uint32_t direccion_fisica);
static void escribir_contenido_en_bloque (uint32_t nro_bloque, uint32_t puntero, void* contenido); 
static void actualizar_tabla_fat_reducir(int posicion_bloque_agregado, int bloques_a_quitar, int posicion_primer_bloque_a_quitar); 
static void actualizar_tabla_fat_ampliar(int posicion_bloque_agregado, int posicion_ultimo_bloque);
//static void actualizar_archivo_de_bloques_ampliar(int posicion_bloque_agregado, int posicion_ultimo_bloque, bloque_archivo_bloques* nuevo_ultimo_bloque_bloques);
//static void actualizar_archivo_de_bloques_reducir(int posicion_bloque_agregado, int bloques_a_quitar, int posicion_primer_bloque_a_quitar);
//============================================== INICIALIZACION ============================================

void crear_fat()
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

FILE* levantar_tabla_FAT() {
	
	char* path_fat = config_valores_filesystem.path_fat;

    archivo_tabla_fat = fopen(path_fat, "rb+");

    if (path_fat == NULL) {
        log_error(filesystem_logger, "No se pudo abrir el archivo.");
    }
	return archivo_tabla_fat;
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

void abrir_archivo (char *nombre_archivo, int socket_kernel)
{
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

void escribir_archivo(char* nombre_archivo, uint32_t puntero_archivo, void* contenido){
	
	fcb* archivo_a_leer = levantar_fcb (nombre_archivo);

	uint32_t bloque_inicial = archivo_a_leer->bloque_inicial; 
	uint32_t bloque_a_escribir = puntero_archivo/tam_bloque;
	free(archivo_a_leer);

	escribir_contenido_en_archivo(bloque_a_escribir,bloque_inicial, contenido, puntero_archivo);
}

void escribir_contenido_en_archivo(uint32_t bloque_a_escribir, uint32_t bloque_inicial, void* contenido, uint32_t puntero) 
{
	//Buscamos el bloque en el Mapa(FAT)
	uint32_t nro_bloque = buscar_bloque_en_FAT(bloque_a_escribir, bloque_inicial);

	//Escribimos en el bloque encontrado el contenido (con un corrimiento dado por el puntero)
	escribir_contenido_en_bloque(nro_bloque, puntero, contenido);
}

static uint32_t buscar_bloque_en_FAT(uint32_t cantidad_bloques_a_recorrer, uint32_t bloque_inicial)
{
    // Empezamos desde el bloque inicial
    uint32_t nro_bloque = bloque_inicial;

    int retardo = config_valores_filesystem.retardo_acceso_fat;

    for (int i = 0; i < cantidad_bloques_a_recorrer; i++) {
        //Accesos a la Tabla FAT
		usleep(1000 * retardo);
        
        // Verificamos que no hayamos alcanzado el final de la FAT (EOF)
        if (tabla_fat_en_memoria[nro_bloque] == UINT32_MAX) {
            break; // Si llegamos al final, salimos del bucle
        }

        // Movemos al siguiente bloque en la FAT
        nro_bloque = tabla_fat_en_memoria[nro_bloque];
    }

    return nro_bloque;
}

static void escribir_contenido_en_bloque (uint32_t nro_bloque, uint32_t puntero, void* contenido) 
{
	int retardo = config_valores_filesystem.retardo_acceso_bloque;

	//direccion ultimo bloque a leer
	uint32_t direccion_bloque = tam_bloque * nro_bloque;

	//tiene que ser puntero + tam_swap
	uint32_t offset = direccion_bloque + tamanio_swap + puntero;
	
	//Abrimos archivo
	archivo_de_bloques = levantar_archivo_bloque();

	//Nos posicionamos en el dato
	fseek(archivo_de_bloques, offset, SEEK_SET);

	//Accedemos al bloque
	usleep(1000* retardo);

	if (fwrite(contenido, tam_bloque, 1, archivo_de_bloques) != 1) {
		log_error(filesystem_logger, "La cagamos chicos \n");
    }

	//mem_hexdump(contenido, sizeof(contenido));
	fclose(archivo_de_bloques);
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
	uint32_t nro_bloque;
	uint32_t direccion_bloque;
	uint32_t offset;
	void* contenido = NULL;
	uint32_t bloque_a_escribir = puntero_archivo/tam_bloque;
	int retardo = config_valores_filesystem.retardo_acceso_bloque;

	//Obtengo el bloque_inicial
	fcb* archivo_a_leer = levantar_fcb (nombre_archivo);
	uint32_t bloque_inicial = archivo_a_leer->bloque_inicial; 
	free(archivo_a_leer);
	
	//Busco el bloque con el contenido
	nro_bloque = buscar_bloque_en_FAT(bloque_a_escribir, bloque_inicial);
	
	//Direccion en archivo_de_bloques (direccion del ultimo bloque a leer?)
	direccion_bloque = tam_bloque * nro_bloque;

	//Me muevo a la particion 
	offset = puntero_archivo + direccion_bloque + tamanio_swap;

	//Abrimos archivo
	archivo_de_bloques = levantar_archivo_bloque();

	//Nos posicionamos en el dato
	fseek(archivo_de_bloques, offset, SEEK_SET);

	//Creo un buffer temporal
	contenido = malloc(tam_bloque); 

	usleep(1000* retardo);
	fread(contenido, tam_bloque, 1, archivo_de_bloques);

	//mem_hexdump(contenido, sizeof(contenido));

	//Mandamos el contenido a que se persista en memoria
	escribir_en_memoria(tam_bloque, contenido, direccion_fisica);

}

static void escribir_en_memoria(int tam_bloque, void* contenido, uint32_t direccion_fisica) {
	t_paquete *paquete = crear_paquete(ESCRIBIR_EN_MEMORIA); 
	agregar_entero_a_paquete(paquete, tam_bloque);
	agregar_bytes_a_paquete(paquete,contenido,tam_bloque); 
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

void truncar_archivo(char *nombre, int tamanio_nuevo, int socket_kernel)
{	
	fcb* fcb_a_truncar = levantar_fcb(nombre);
	char* nombre_archivo = fcb_a_truncar->nombre_archivo;
	int tamanio_actual_archivo = fcb_a_truncar->tamanio_archivo;
	int bloque_inicial = fcb_a_truncar->bloque_inicial;
	free(fcb_a_truncar);

	log_info(filesystem_logger,"Truncando archivo: Nombre %s, tamanio %d, Bloque inicial %d \n",nombre_archivo, tamanio_actual_archivo, bloque_inicial);

	if(tamanio_actual_archivo < tamanio_nuevo && tamanio_nuevo < espacio_de_FAT)
	{
		ampliar_tamanio_archivo(tamanio_nuevo, nombre_archivo, tamanio_actual_archivo, bloque_inicial);
		log_info(filesystem_logger,"Ampliamos\n");
	}
	else if (tamanio_actual_archivo > tamanio_nuevo)
	{
		reducir_tamanio_archivo(tamanio_nuevo, nombre_archivo, tamanio_actual_archivo, bloque_inicial);
		log_info(filesystem_logger,"Reducimos\n");
	}
	else{
		printf("Bobi no truncaste\n");
	}

	cargamos_cambios_a_fcb(tamanio_nuevo, nombre);

	//Confirmamos truncado
	int truncado = 1;
    send(socket_kernel, &truncado, sizeof(int), 0);
}

void ampliar_tamanio_archivo (int nuevo_tamanio_archivo, char* nombre_archivo, int tamanio_actual_archivo, int bloque_inicial)
{
	int bloques_a_agregar = ((nuevo_tamanio_archivo - tamanio_actual_archivo)/tam_bloque);
	
	int posicion_ultimo_bloque = bloque_inicial + bloques_a_agregar;

	//Nos ponemos en la posicion que tenemos que agregar dependiendo del blooque inicial
	for (int posicion_bloque_agregado = 0; posicion_bloque_agregado < bloques_a_agregar; posicion_bloque_agregado++)
	{
		uint32_t* nuevo_ultimo_bloque_fat = malloc(tam_bloque);
		bloque_archivo_bloques* nuevo_ultimo_bloque_bloques = malloc(tam_bloque);

		if (nuevo_ultimo_bloque_fat == NULL && nuevo_ultimo_bloque_bloques == NULL)
		{
			perror("No se pudo alocar memoria para hacer la entrada fat\n");
			abort();
		}

		actualizar_tabla_fat_ampliar(posicion_bloque_agregado, posicion_ultimo_bloque);		
		
		//actualizar_archivo_de_bloques_ampliar(posicion_bloque_agregado, posicion_ultimo_bloque, nuevo_ultimo_bloque_bloques);
	}
}

static void actualizar_tabla_fat_ampliar(int posicion_bloque_agregado, int posicion_ultimo_bloque) {
	
	uint32_t eof = UINT32_MAX;
	uint32_t valor_inicial = 0;
	archivo_tabla_fat = levantar_tabla_FAT();
	fseek(archivo_tabla_fat,posicion_bloque_agregado,SEEK_SET); //Revisar dsps

	if((posicion_bloque_agregado + 1) == posicion_ultimo_bloque ) {

		//escribimos el anteultimo bloque
		fwrite(&valor_inicial, sizeof(uint32_t), 1, archivo_tabla_fat);

		//escribimos en el ultimo bloque
		fwrite(&eof, sizeof(uint32_t), 1, archivo_tabla_fat);
		
		log_info(filesystem_logger,"Actualizamos el ultimo bloque de archivo fat\n");
	}
	
	//si el siguiente no es el ultimo bloque solo guardamos el valor del puntero
	else
	{
		fwrite(&valor_inicial, sizeof(uint32_t), 1, archivo_tabla_fat);
	}

	fclose(archivo_tabla_fat);
}

void reducir_tamanio_archivo (int nuevo_tamanio_archivo, char* nombre_archivo, int tamanio_actual_archivo, int bloque_inicial)
{
	int bloques_a_quitar = nuevo_tamanio_archivo - tamanio_actual_archivo;
	
	int posicion_primer_bloque_a_quitar = bloque_inicial + bloques_a_quitar;

	for (int posicion_bloque_agregado = 0; posicion_bloque_agregado < bloques_a_quitar; posicion_bloque_agregado++) {

	actualizar_tabla_fat_reducir(posicion_bloque_agregado, bloques_a_quitar, posicion_primer_bloque_a_quitar);
	

    }

    log_info(filesystem_logger, "Se redujo el tamaño del archivo\n");

}

void actualizar_tabla_fat_reducir(int posicion_bloque_agregado, int bloques_a_quitar, int posicion_primer_bloque_a_quitar) {

	archivo_tabla_fat = levantar_tabla_FAT();

	if (archivo_tabla_fat == NULL) {
		perror("No se pudo abrir la tabla fat");
		abort();
	}

    if (posicion_bloque_agregado == bloques_a_quitar - 1)
    {
		// el último bloque que vamos a quitar
		fseek(archivo_tabla_fat, posicion_primer_bloque_a_quitar * sizeof(uint32_t), SEEK_SET);
		uint32_t bloque_siguiente = UINT32_MAX;
		fwrite(&bloque_siguiente, sizeof(uint32_t), 1, archivo_tabla_fat);
    }
    else
    {
		// no es el último bloque que vamos a quitar
		fseek(archivo_tabla_fat, (posicion_primer_bloque_a_quitar + posicion_bloque_agregado) * sizeof(uint32_t), SEEK_SET);
		uint32_t bloque_siguiente = posicion_primer_bloque_a_quitar + posicion_bloque_agregado + 1;
		fwrite(&bloque_siguiente, sizeof(uint32_t), 1, archivo_tabla_fat);
    }

	fclose(archivo_tabla_fat);

}

