#include "filesystem.h"

t_list* procesos_en_filesystem;
t_bitarray* mapa_bits_swap;

static t_proceso_en_filesystem* buscar_proceso_en_filesystem(int pid);

//==============================================================================================================
void inicializar_swap()
{
    char* path = config_valores_filesystem.path_bloques;
    FILE* fd = fopen(path, "w");

    fseek(fd, tamanio_swap , SEEK_SET);
    fputc('\0', fd);
    fclose(fd);

	for (int i=0; i < config_valores_filesystem.cant_bloques_swap ; i++)
	{
        bloque_libre_swap(i);
	}    
} 

//fs va a reservar 1 bloque para cada pagina del proceso, la memoria solo sabe la cant de paginas que tinee, no sabe las paginas
t_list* reservar_bloques(int pid, int cantidad_bloques)
{
    // Creo un proceso de fs
    t_proceso_en_filesystem* proceso_en_filesystem = malloc(sizeof(t_proceso_en_filesystem));
    proceso_en_filesystem->pid = pid;
    proceso_en_filesystem->bloques_reservados = list_create();

    int tam_bloque = config_valores_filesystem.tam_bloque;

    // Por cada pagina le asigno un nuevo bloque
    for (int index = 0; index < cantidad_bloques; index++) { 

        //voy a crear swap segun las paginas que tiene el proceso
        bloque_swap* nuevo_bloque = asignar_bloque_swap(tam_bloque, index, pid);
        
        //agrego el bloque/pagina al proceso 
        if (nuevo_bloque != NULL) {
            list_add(proceso_en_filesystem->bloques_reservados, (void*)nuevo_bloque);
        } else {
            // Si no se pudo reservar un bloque, liberamos los bloques previamente reservados
            liberar_bloques(proceso_en_filesystem->bloques_reservados);
            free(proceso_en_filesystem); 
            return NULL;
        }
    }
    list_add(procesos_en_filesystem, (void*)proceso_en_filesystem);
    return proceso_en_filesystem->bloques_reservados;
}

//el nro de pagina va a ser la posicion donde este, que va a ser la posicion del bloque porque cada bloque es una pagina
bloque_swap* asignar_bloque_swap (int tam_bloque, int index, int pid)
{
    bloque_swap* bloque = malloc(sizeof(bloque_swap));

    //busco la posicion donde estoy tomando en cuenta todos los bloques
    int posicion_en_swap = index % config_valores_filesystem.cant_bloques_swap;
    bloque->nro_pagina = index;
    bloque->bit_presencia_swap = 1;
    bloque->posicion_swap = posicion_en_swap; 
    bloque->pid = pid;

    FILE* fd = fopen(config_valores_filesystem.path_bloques, "wb+");
    if (fd == NULL) {
        perror("Error al abrir el archivo de bloques de SWAP\n");
        free(bloque);
        return NULL;
    }

    //seek end para que vaya al final y no sobreescriba datos
    fseek(fd, index * sizeof(*bloque), SEEK_SET);

    //escribo el bloque completo en el archivo de bloques
    fwrite(bloque, sizeof(*bloque), 1, fd);
    fclose(fd);

    return bloque;
}

void enviar_bloques_reservados(t_list* bloques_reservados) {
	t_paquete* paquete = crear_paquete (LISTA_BLOQUES_RESERVADOS);
	agregar_lista_de_cadenas_a_paquete(paquete, bloques_reservados);
	enviar_paquete(paquete, socket_memoria); 
	eliminar_paquete(paquete);
} 

//===================================================== BUSCAR ================================================

//tengo una lista de procesos en fs 
static t_proceso_en_filesystem* buscar_proceso_en_filesystem(int pid) {
    int i;
    for (i=0; i < list_size(procesos_en_filesystem); i++){
        if ( ((t_proceso_en_filesystem*) list_get(procesos_en_filesystem, i)) -> pid == pid){
            break;            
        }
    }
    return list_get(procesos_en_filesystem, i);
}


//cada bloque va a tener la pagina que guarda y la posicion del bloque es el nro de pagina
bloque_swap* buscar_bloque_swap(int nro_pagina, int pid)
{
    usleep(config_valores_filesystem.retardo_acceso_bloque);
    log_info(filesystem_logger, "Acceso SWAP: BLOQUE %d", nro_pagina);
    t_proceso_en_filesystem* proceso_en_fs = buscar_proceso_en_filesystem(pid); 
    
    if (list_size(proceso_en_fs->bloques_reservados) == NULL || list_size(proceso_en_fs->bloques_reservados) == 0) {
        return NULL;
    }

    //por cada bloque, busco la pagina que me piden
    for(int i = 0; i < list_size(proceso_en_fs->bloques_reservados); i++)
    {
        bloque_swap* bloque_actual = list_get(proceso_en_fs->bloques_reservados, i);
        // Si coincide el número de página, devolver la página
        if (bloque_actual->nro_pagina == nro_pagina) {
            return bloque_actual;
        }
    }
    return NULL;  
}

//===================================================== SWAP IN / SWAP OUT ================================================

void swap_out(int pid, int nro_pagina)
{
    //no va a pasar que el bloque no exista
    FILE* fd = fopen(config_valores_filesystem.path_bloques, "rb+");
    if (fd == NULL) {
        perror("Error al abrir el archivo de bloques\n");
        return;
    }

    bloque_swap* bloque_a_leer = buscar_bloque_swap(nro_pagina, pid);

    while (fread(bloque_a_leer, sizeof(*bloque_a_leer), 1, fd) == 1) {
        if (bloque_a_leer->nro_pagina == nro_pagina) {
            t_paquete* paquete = crear_paquete(PAGINA_PARA_ESCRITURA);
            agregar_entero_a_paquete(paquete, bloque_a_leer->nro_pagina);
            agregar_entero_a_paquete(paquete, bloque_a_leer->posicion_swap);
            agregar_entero_a_paquete(paquete, pid);

            enviar_paquete(paquete, socket_memoria);
            eliminar_paquete(paquete);
            break;
        }
    }

    fclose(fd);
}

void swap_in(int pid, int nro_pag_pf, int posicion_swap, int marco)
{


}

//=================================================== FINALIZACION =========================================

void liberar_bloques(int pid)
{
	//Busco el proceso y cuantos bloques tiene
	t_proceso_en_filesystem* proceso_en_filesystem = buscar_proceso_en_filesystem(pid);
	int cantidad_bloques_a_liberar = list_size(proceso_en_filesystem->bloques_reservados);
    log_info(filesystem_logger, "Liberando bloques de swap del PID: %d - Cantidad bloques a liberar: %d\n", pid, cantidad_bloques_a_liberar);

	//Liberamos los bloques del proceso (Ponemos en 0)
	for(int i = 0; i < cantidad_bloques_a_liberar; i++){

		bloque_swap* bloque_a_liberar = list_get(proceso_en_filesystem->bloques_reservados, i);
        bloque_a_liberar->nro_pagina = 0;
        bloque_a_liberar->bit_presencia_swap = 10;
        bloque_a_liberar->posicion_swap = 0; //tiene que ser el bloque dentro de todoe l swap
        bloque_a_liberar->pid = 0;
		list_replace(proceso_en_filesystem->bloques_reservados, i, bloque_a_liberar);
        bitarray_clean_bit(bitmap_archivo_bloques, i);
        liberar_bloque_individual(bloque_a_liberar);
	}

    list_remove_element(procesos_en_filesystem, (void*)proceso_en_filesystem);
	free(proceso_en_filesystem);

    log_info(filesystem_logger, "Bloques de swap liberados exitosamente\n");

	int ok_finalizacion_swap = 1;
    send(socket_memoria, &ok_finalizacion_swap, sizeof(int), 0);
}

void liberar_bloque_individual(bloque_swap* bloque) {
    if (bloque != NULL) {
        free(bloque);
    }
}