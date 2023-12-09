#include "filesystem.h"

t_list* procesos_en_filesystem;
t_bitarray* mapa_bits_swap;

static t_proceso_en_filesystem* buscar_proceso_en_filesystem(int pid);

//==============================================================================================================
void crear_filesystem_swap(int cant_paginas_proceso) {

    char* path = config_valores_filesystem.path_bloques;
    FILE* fd = fopen(path, "w");

    fseek(fd, tamanio_swap , SEEK_SET);
    fputc('\0', fd);
    fclose(fd);

    int tam = cant_paginas_proceso / 8;

	void* aux = malloc(tam);
	mapa_bits_swap = bitarray_create_with_mode(aux, tam, MSB_FIRST);

    int cant_bloques_swap = config_valores_filesystem.cant_bloques_swap;

	for (int i=0; i < cant_bloques_swap; i++)
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

    //el index es dentro de la lista del proceso
    // Por cada pagina le asigno un nuevo bloque
    for (int index = 0; index < cantidad_bloques; index++) { 
        bloque_swap* nuevo_bloque = crear_bloque_swap(tam_bloque,index, pid);
        if (nuevo_bloque != NULL) {
            list_add(proceso_en_filesystem->bloques_reservados, nuevo_bloque);
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
bloque_swap* crear_bloque_swap(int tam_bloque, int index, int pid)
{
    bloque_swap* bloque = malloc(sizeof(bloque_swap));
    bloque->pagina_guardada = malloc(sizeof(t_pagina_fs));
    bloque->data = malloc(tam_bloque);

    int posicion_en_swap = index % config_valores_filesystem.cant_bloques_swap;
	bloque->data = "\0";
    //el nro pag es el indice del bloque en la lista del proceso
    bloque->pagina_guardada->nro_pagina = index;
    bloque->pagina_guardada->bit_presencia_swap = 1;
    bloque->pagina_guardada->posicion_swap = posicion_en_swap; //tiene que ser el bloque dentro de todoe l swap
    bloque->pagina_guardada->pid = pid;

    bitarray_set_bit(bitmap_archivo_bloques, index);
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
t_pagina_fs* buscar_pagina_swap(int nro_pagina, int pid)
{
    usleep(config_valores_filesystem.retardo_acceso_bloque);
    log_info(filesystem_logger, "Acceso SWAP: BLOQUE %d", nro_pagina);
    t_proceso_en_filesystem* proceso_en_fs = buscar_proceso_en_filesystem(pid); 
    
    //por cada bloque, busco la pagina que me piden
    for(int i = 0; i < proceso_en_fs->bloques_reservados; i++)
    {
        bloque_swap* bloque_actual = list_get(proceso_en_fs->bloques_reservados, i);
        // Si coincide el número de página, devolver la página
        if (bloque_actual->pagina_guardada->nro_pagina == nro_pagina) {
            return bloque_actual->pagina_guardada;
        }
    }
    return NULL;  
}

//===================================================== SWAP IN / SWAP OUT ================================================

void swap_in(int pid, int nro_pagina)
{
 
}


void swap_out(int pid, int nro_pag_pf, int posicion_swap, int marco)
{
    t_proceso_en_filesystem* proceso_en_fs = buscar_proceso_en_filesystem(pid); 

    //voy a buscar solamente el nro de pagina que va a ser igual a la posicion del bloque
    t_pagina_fs* pagina = buscar_pagina_swap(nro_pag_pf, pid);
    posicion_swap = pagina->posicion_swap;

    //le mando a memoria el nro de pagina, como el id total la memoria no sabe nada apenas empieza
    t_paquete *paquete = crear_paquete(PAGINA_PARA_ESCRITURA);
    agregar_entero_a_paquete(paquete, nro_pag_pf);
    agregar_entero_a_paquete(paquete, posicion_swap);
    agregar_entero_a_paquete(paquete, pid);
    //agregar_entero_a_paquete(paquete, marco);

    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
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
		bloque_a_liberar->data = " ";
        bloque_a_liberar->pagina_guardada->nro_pagina = 0;
        bloque_a_liberar->pagina_guardada->bit_presencia_swap = 10;
        bloque_a_liberar->pagina_guardada->posicion_swap = 0; //tiene que ser el bloque dentro de todoe l swap
        bloque_a_liberar->pagina_guardada->pid = 0;
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