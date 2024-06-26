#include "filesystem.h"

t_list* procesos_en_filesystem;
t_bitarray* mapa_bits_swap;
FILE* archivo_de_bloques;

static void avisar_a_memoria_bloques_liberados();
//==============================================================================================================
void inicializar_swap()
{
    archivo_de_bloques = levantar_archivo_bloque();

    fseek(archivo_de_bloques, 0 , SEEK_SET);
    
    for (int i = 0; i < tamanio_swap; ++i) {
        fputc('\0', archivo_de_bloques);
    }

    fclose(archivo_de_bloques);
 
} 

//fs va a reservar 1 bloque para cada pagina del proceso, la memoria solo sabe la cant de paginas que tinee, no sabe las paginas
t_list* reservar_bloques(int pid, int cantidad_bloques)
{
    // Creo un proceso de fs
    t_proceso_en_filesystem* proceso_en_filesystem = malloc(sizeof(t_proceso_en_filesystem));
    proceso_en_filesystem->pid = pid;
    proceso_en_filesystem->bloques_reservados = list_create();

    int bloques_necesarios = (cantidad_bloques / tam_bloque);

    // Por cada pagina le asigno un nuevo bloque
    for (int i = 0; i < bloques_necesarios; i++){

        //voy a crear swap segun las paginas que tiene el proceso
        bloque_swap* nuevo_bloque = asignar_bloque_swap(tam_bloque, i, pid);
        
        //agrego el bloque/pagina al proceso 
        if (nuevo_bloque != NULL) {
            list_add(proceso_en_filesystem->bloques_reservados, (void*)nuevo_bloque);
        } else {
            // Si no se pudo reservar un bloque, liberamos los bloques previamente reservados
            liberar_bloques(proceso_en_filesystem->bloques_reservados, proceso_en_filesystem);
            return NULL;
        }
    }
    list_add(procesos_en_filesystem, (void*)proceso_en_filesystem);
    return proceso_en_filesystem->bloques_reservados;
}

//el nro de pagina va a ser la posicion donde este, que va a ser la posicion del bloque porque cada bloque es una pagina
/*bloque_swap* asignar_bloque_swap(int tam_bloque, int posicion_actual, int pid)
{
    archivo_de_bloques = levantar_archivo_bloque();
    bloque_swap* bloque = malloc(sizeof(bloque_swap));

    int posicion_en_swap = posicion_actual % config_valores_filesystem.cant_bloques_swap;
    int intentos = 0;

    do
    {
        // Nos posicionamos al comienzo del bloque
        fseek(archivo_de_bloques, posicion_en_swap * sizeof(*bloque), SEEK_SET);

        // Leemos el contenido del bloque
        fread(bloque, sizeof(*bloque), 1, archivo_de_bloques);

        // Si el bloque está libre
        if (bloque->bit_presencia_swap == 0)
        {
            bloque->nro_pagina = posicion_actual;
            bloque->bit_presencia_swap = 1;
            bloque->posicion_swap = posicion_en_swap;
            bloque->pid = pid;

            // Nos posicionamos al comienzo del bloque
            fseek(archivo_de_bloques, posicion_en_swap * sizeof(*bloque), SEEK_SET);

            // Escribimos el bloque en el archivo
            fwrite(bloque, sizeof(*bloque), 1, archivo_de_bloques);

            fclose(archivo_de_bloques);

            return bloque;
        }

        // Nos movemos al próximo bloque
        posicion_en_swap = (posicion_en_swap + 1) % config_valores_filesystem.cant_bloques_swap;
        intentos++;

    } while (intentos < config_valores_filesystem.cant_bloques_swap);

    // SI no hay bloques libres
    fclose(archivo_de_bloques);
    free(bloque);
    return NULL;
}*/



bloque_swap* asignar_bloque_swap (int tam_bloque, int index, int pid)
{
    bloque_swap* bloque = malloc(sizeof(bloque_swap));

    //busco la posicion donde estoy tomando en cuenta todos los bloques
    int posicion_en_swap = index % config_valores_filesystem.cant_bloques_swap;
    bloque->nro_pagina = index;
    bloque->bit_presencia_swap = 1;
    bloque->posicion_swap = posicion_en_swap; 
    bloque->pid = pid;

    archivo_de_bloques = levantar_archivo_bloque();
    if (archivo_de_bloques == NULL) {
        perror("Error al abrir el archivo de bloques de SWAP para asignar\n");
        free(bloque);
        return NULL;
    }

    //seek end para que vaya al final y no sobreescriba datos
    fseek(archivo_de_bloques, index * sizeof(*bloque), SEEK_SET);

    //escribo el bloque completo en el archivo de bloques
    fwrite(bloque, sizeof(*bloque), 1, archivo_de_bloques);
    fclose(archivo_de_bloques);

    return bloque;
}



void enviar_bloques_reservados(t_list* bloques_reservados, int pid) {
	t_paquete* paquete = crear_paquete (LISTA_BLOQUES_RESERVADOS);
    agregar_entero_a_paquete(paquete, pid);
	agregar_lista_de_cadenas_a_paquete(paquete, bloques_reservados);
	enviar_paquete(paquete, socket_memoria); 
	eliminar_paquete(paquete);
} 

//===================================================== BUSCAR ================================================

//tengo una lista de procesos en fs 
t_proceso_en_filesystem* buscar_proceso_en_filesystem(int pid) 
{
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
    
    if (list_size(proceso_en_fs->bloques_reservados) == 0) {
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
    archivo_de_bloques = levantar_archivo_bloque();

    bloque_swap* bloque_a_leer = buscar_bloque_swap(nro_pagina, pid);

    while (fread(bloque_a_leer, sizeof(*bloque_a_leer), 1, archivo_de_bloques) == 1) {
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

    fclose(archivo_de_bloques);
}

void swap_in(int pid, int nro_pag, int posicion_swap) {
    
    archivo_de_bloques = levantar_archivo_bloque();

    bloque_swap* bloque_a_escribir = buscar_bloque_swap(nro_pag, pid);

    bloque_a_escribir->bit_presencia_swap = 1;
    bloque_a_escribir->posicion_swap = posicion_swap;

    if (fwrite(bloque_a_escribir, sizeof(bloque_swap), 1, archivo_de_bloques) != 1) {
        perror("Error al escribir en SWAP\n");
        abort();
    }

    free(bloque_a_escribir);  
}

//=================================================== FINALIZACION =========================================

void liberar_bloques(t_list* lista_bloques_swap, t_proceso_en_filesystem* proceso)
{
	//Busco el proceso y cuantos bloques tiene
	int cantidad_bloques_a_liberar = list_size(lista_bloques_swap);
    log_info(filesystem_logger, "Liberando bloques de swap del PID: %d - Cantidad bloques a liberar: %d\n",proceso->pid, cantidad_bloques_a_liberar);

	//Liberamos los bloques del proceso (Ponemos en 0)
	for(int i = 0; i < cantidad_bloques_a_liberar; i++){

		bloque_swap* bloque_a_liberar = list_get(lista_bloques_swap, i);
		
        if (bloque_a_liberar != NULL) 
        free(bloque_a_liberar);
	}

    list_destroy(proceso->bloques_reservados);
    list_remove_element(procesos_en_filesystem, (void*)proceso);
	free(proceso);

    avisar_a_memoria_bloques_liberados();

    log_info(filesystem_logger, "Bloques de swap liberados exitosamente\n");	
}


static void avisar_a_memoria_bloques_liberados() {
    t_paquete* paquete = crear_paquete(FILESYSTEM_LIBERA_BLOQUES);
    agregar_entero_a_paquete(paquete, 1);
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
}