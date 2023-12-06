#include "filesystem.h"

t_list* bloques_reservados;
//t_list* bloques_a_liberar = NULL;
t_list* procesos_en_filesystem;

static t_proceso_en_filesystem* buscar_proceso_en_filesystem(int pid);

//==============================================================================================================
t_list* reservar_bloques(int pid, int cantidad_bloques)
{
    // Creo un proceso
    t_proceso_en_filesystem* proceso_en_filesystem = malloc(sizeof(t_proceso_en_filesystem));
    proceso_en_filesystem->pid = pid;
    proceso_en_filesystem->bloques_reservados = list_create();

    int tam_bloque = config_valores_filesystem.tam_bloque;

    // Por cada pagina le asigno un nuevo bloque
    for (int index = 0; index < cantidad_bloques; index++) { 
        bloque_swap* nuevo_bloque = crear_bloque_swap(tam_bloque,index);
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


bloque_swap* crear_bloque_swap(int tam_bloque, int index)
{
    bloque_swap* bloque = malloc(sizeof(tam_bloque));
	bloque->data = "\0";

    bitarray_set_bit(bitmap_archivo_bloques, index);
    return bloque;
}

void enviar_bloques_reservados(t_list* bloques_reservados) {
	t_paquete* paquete = crear_paquete (LISTA_BLOQUES_RESERVADOS);
	agregar_lista_de_cadenas_a_paquete(paquete, bloques_reservados);
	enviar_paquete(paquete, socket_memoria); 
	eliminar_paquete(paquete);
} 

void enviar_pagina_para_escritura() {
	t_paquete* paquete = crear_paquete(PAGINA_PARA_ESCRITURA);
	//agregar_pagina_a_paquete(paquete, pagina); //medio raro ponerle recibida pero es lo que recibe la memoria después de este paquete
	enviar_paquete(paquete, socket_memoria);
	eliminar_paquete(paquete);
}

//===================================================== BUSCAR ================================================

static t_proceso_en_filesystem* buscar_proceso_en_filesystem(int pid) {
    int i;
    for (i=0; i < list_size(procesos_en_filesystem); i++){
        if ( ((t_proceso_en_filesystem*) list_get(procesos_en_filesystem, i)) -> pid == pid){
            break;            
        }
    }
    return list_get(procesos_en_filesystem, i);
}



//=================================================== FINALIZACION =========================================

void liberar_bloques(int pid)
{
	//Busco el proceso y cuantos bloques tiene
	t_proceso_en_filesystem* proceso_en_filesystem = buscar_proceso_en_filesystem(pid);
	int cantidad_bloques_a_liberar = list_size(proceso_en_filesystem->bloques_reservados);

	//Liberamos los bloques del proceso (Ponemos en 0)
	for(int i =0; i <= cantidad_bloques_a_liberar; i++){
		bloque_swap* bloque_a_liberar = list_get(proceso_en_filesystem->bloques_reservados, i);
		bloque_a_liberar->data = "0";
		list_replace(proceso_en_filesystem->bloques_reservados, i, bloque_a_liberar);
        bitarray_clean_bit(bitmap_archivo_bloques, i);
        liberar_bloque_individual(bloque_a_liberar);
	}

    list_remove_element(procesos_en_filesystem, (void*)proceso_en_filesystem);
	free(proceso_en_filesystem);

	int ok_finalizacion_swap = 1;
    send(socket_memoria, &ok_finalizacion_swap, sizeof(int), 0);
}

void liberar_bloque_individual(bloque_swap* bloque) {
    if (bloque != NULL) {
        free(bloque);
    }
}
