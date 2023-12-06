#include "memoria.h"

//================================================= Variables Globales ================================================
void* espacio_usuario;
t_list* marcos_memoria;
t_bitarray* mapa_bits_principal;
t_bitarray* mapa_bits_swap;
t_list* procesos_en_memoria;
t_list* paginas_en_memoria;
t_bitarray* status_tabla_paginas = NULL;
double tiempo;
//pthread_mutex_t mutex_tiempo;

t_list* bloques_reservados;
//================================================= Funciones Internas ================================================
static void liberar_swap(int pid);
static void enviar_inicializar_swap_a_filesystem (int pid, int cantidad_paginas_proceso);
static double obtener_tiempo();
static void liberar_paginas(t_proceso_en_memoria* proceso_en_memoria);

//================================================= Creacion Estructuras ====================================================

/// @brief Espacio Usuario ///
void creacion_espacio_usuario() {
    espacio_usuario = malloc (config_valores_memoria.tam_memoria); 
	if (espacio_usuario == NULL) {
        perror ("No se pudo alocar memoria al espacio de usuario.");
        abort();
    }
	//liberar_espacio_usuario(); atexit? 
}

void liberar_espacio_usuario() {
	free (espacio_usuario);
}

//======================================================= INICIALIZACIONES =========================================================================================================

void crear_tablas_paginas_proceso(int pid, int cantidad_paginas_proceso, char* path_recibido){

    t_proceso_en_memoria* proceso_en_memoria = malloc(sizeof(t_proceso_en_memoria));
    proceso_en_memoria->pid = pid;
    proceso_en_memoria->bloques_reservados = list_create();
    proceso_en_memoria->paginas_en_memoria = list_create();
    proceso_en_memoria->cantidad_entradas = cantidad_paginas_proceso;

    // Leemos el path antes de guardarlo en el proceso en memoria
	char* instrucciones_leidas = leer_archivo_instrucciones(path_recibido);
    proceso_en_memoria->path_proceso = strdup(instrucciones_leidas);

    inicializar_la_tabla_de_paginas(proceso_en_memoria, cantidad_paginas_proceso);

    list_add(procesos_en_memoria, (void*)proceso_en_memoria);
    
    log_info(memoria_logger, "PID: %d - Tamaño: %d", pid, cantidad_paginas_proceso);
}

void inicializar_la_tabla_de_paginas(t_proceso_en_memoria* proceso, int cantidad_paginas_proceso) {
    
    t_pagina* tp = (t_pagina*)malloc(sizeof(t_pagina));

    for (int i = 0; i < cantidad_paginas_proceso; i++) {
        tp[i].id = proceso->pid;
        tp[i].numero_de_pagina = i;
        tp[i].marco = i;
        tp[i].bit_de_presencia = 0;
        tp[i].bit_modificado = 0;
        tp[i].posicion_swap = -1; // No en memoria
        tp[i].tiempo_uso = obtener_tiempo();
        tp[i].tiempo_de_carga = i;

        //paso la direccion de la estructura
        list_add(proceso->paginas_en_memoria, (t_pagina*)&tp[i]);
    }
}

static double obtener_tiempo(){
	//pthread_mutex_lock(&mutex_tiempo);
	double t = tiempo;
	tiempo++;
	//pthread_mutex_unlock(&mutext_tiempo);
	return t;
}

void inicializar_swap_proceso(int pid, int cantidad_paginas_proceso) {
    enviar_inicializar_swap_a_filesystem(pid, cantidad_paginas_proceso);
}

static void enviar_inicializar_swap_a_filesystem (int pid, int cantidad_paginas_proceso) {
    t_paquete* paquete = crear_paquete(INICIALIZAR_SWAP); 
    agregar_entero_a_paquete(paquete, pid); 
    agregar_entero_a_paquete(paquete, cantidad_paginas_proceso);
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);
}

//======================================================= BUSCAR_PAGINA =========================================================================================================

t_proceso_en_memoria* buscar_proceso_en_memoria(int pid) {
    int i;
    for (i=0; i < list_size(procesos_en_memoria); i++){
        if ( ((t_proceso_en_memoria*) list_get(procesos_en_memoria, i)) -> pid == pid){
            break;            
        }
    }
    return list_get(procesos_en_memoria, i);
}

t_pagina* buscar_pagina(int pid, int num_pagina){

    // Obtenemos proceso en memoria
    t_proceso_en_memoria* proceso_en_memoria = buscar_proceso_en_memoria(pid); 

    // Iterar las entradas
    for(int i = 0; i < proceso_en_memoria->cantidad_entradas; i++)
    {
        t_pagina* pagina_actual = list_get(proceso_en_memoria->paginas_en_memoria, i);

        // Si coincide el número de página, devolver la página
        if (pagina_actual->numero_de_pagina == num_pagina) {
            return pagina_actual;
        }
    }

    // Si no consigue devuelvo NULL (Tratar después error)
    return NULL;
}

int buscar_marco(int pid, int num_pagina){

    t_pagina* pagina = buscar_pagina(pid, num_pagina);
    log_info(memoria_logger, "Se buscara marco en las tablas de paginas");

    if (pagina->bit_de_presencia == 0) {
        return -1; //Si el marco es -1, significa que hay page_fault 
    }   
    else {
        log_info(memoria_logger, "Acceso a tabla de paginas: PID: %d - Página: %d - Marco: %d", pid, num_pagina, pagina->marco); 
        return pagina->marco;
    }
}


//======================================================= FINALIZAR_PROCESO =========================================================================================================

void finalizar_en_memoria(int pid) {
    t_proceso_en_memoria* proceso_en_memoria = buscar_proceso_en_memoria(pid);
    liberar_paginas(proceso_en_memoria);
    liberar_swap(pid);
    list_remove_element(procesos_en_memoria,proceso_en_memoria);
    free(proceso_en_memoria); 
}

static void liberar_paginas(t_proceso_en_memoria* proceso_en_memoria) {

    int cantidad_de_paginas_a_liberar = proceso_en_memoria->cantidad_entradas;
    list_destroy(proceso_en_memoria->paginas_en_memoria);

    log_info(memoria_logger, "PID: %d - Tamaño: %d", proceso_en_memoria->pid, cantidad_de_paginas_a_liberar);

}

static void liberar_swap(int pid) {
    t_paquete* paquete = crear_paquete(LIBERAR_SWAP);
    agregar_entero_a_paquete(paquete, pid);
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);

    int ok_finalizacion_swap;
    recv(socket_fs, &ok_finalizacion_swap, sizeof(int), 0);
	log_info(memoria_logger,"Bloques reservados\n");

    if (ok_finalizacion_swap != 1)
    {
        log_error(memoria_logger, "No se pudieron crear reservar los bloques");
    }
}



