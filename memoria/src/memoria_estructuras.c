#include "memoria.h"

//================================================= Variables Globales ================================================
void* espacio_usuario;
t_list* marcos_memoria;
t_bitarray* mapa_bits_principal;
t_bitarray* mapa_bits_swap;
t_list* procesos_en_memoria;

t_bitarray* status_tabla_paginas = NULL;
// numero_paginas = floor(config_valores_memoria.tam_memoria / config_valores_memoria.tam_pagina);

//================================================= Funciones Internas ================================================
static void liberar_swap(t_list* paginas_a_liberar, int pid, int socket_fs);
//================================================= Creacion Estructuras ====================================================

/// @brief Espacio Usuario ///
void creacion_espacio_usuario() {
    espacio_usuario = malloc (config_valores_memoria.tam_memoria); 
	if (espacio_usuario == NULL) {
        perror ("No se pudo alocar memoria al espacio de usuario.");
        abort();
    }
	liberar_espacio_usuario(); //atexit? 
}

void liberar_espacio_usuario() {
	free (espacio_usuario);
}

void crear_tablas_paginas_proceso(int pid, int cantidad_paginas_proceso, char* path_recibido){
    procesos_en_memoria = list_create();

    t_proceso_en_memoria* proceso_en_memoria = malloc(sizeof(t_proceso_en_memoria));
    proceso_en_memoria->pid = pid;
    proceso_en_memoria->cantidad_paginas_proceso = cantidad_paginas_proceso;
    proceso_en_memoria->paginas_en_memoria = list_create();

    // Leemos el path antes de guardarlo en el proceso en memoria
	char* instrucciones_leidas = leer_archivo_instrucciones(path_recibido);
    proceso_en_memoria->path_proceso = strdup(instrucciones_leidas);

    inicializar_la_tabla_de_paginas(cantidad_paginas_proceso);

    list_add(procesos_en_memoria, (void*)proceso_en_memoria);
    
    log_info(memoria_logger, "PID: %d - Tamaño: %d", pid, cantidad_paginas_proceso);
}

// Inicializar la tabla de paginas
void inicializar_la_tabla_de_paginas(int cantidad_paginas_proceso) {
    t_pagina* tp = (t_pagina*)malloc(sizeof(t_pagina));
    tp->tamanio = cantidad_paginas_proceso;
    tp->entradas = (entrada_t_pagina*)malloc(sizeof(entrada_t_pagina) * cantidad_paginas_proceso);

    for (int i = 0; i < cantidad_paginas_proceso; i++) {
        tp->entradas[i].numero_de_pagina = i;
        tp->entradas[i].marco = i;
        tp->entradas[i].bit_de_presencia = 0;
        tp->entradas[i].bit_modificado = 0;
        tp->entradas[i].posicion_swap = -1; // No en memoria

        // Marco la pagina que no está en memoria
        //bitarray_clean_bit(status_tabla_paginas, i);
    }
}

t_proceso_en_memoria* buscar_proceso_en_memoria(int pid){
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

    // página de la tabla del proceso
    return list_get(proceso_en_memoria->paginas_en_memoria->entradas->numero_de_pagina, num_pagina);
}

int buscar_marco(int pid, int num_pagina){

    t_pagina* pagina = buscar_pagina(pid, num_pagina);
    log_info(memoria_logger, "Se buscara marco en las tablas de paginas");

    if (pagina->entradas->bit_de_presencia == 0) {
        return -1; //Si el marco es -1, significa que hay page_fault 
    }   
    else {
        log_info(memoria_logger, "Acceso a tabla de paginas: PID: %d - Página: %d - Marco: %d", pid, num_pagina, pagina->entradas->marco); 
        return pagina->entradas->marco;
    }
}

void solucionar_page_fault(int num_pagina, int socket_fs) {
    t_paquete* paquete = crear_paquete(SOLUCIONAR_PAGE_FAULT); 
    agregar_entero_a_paquete(paquete, num_pagina); 
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);
}

void finalizar_en_memoria(int pid, int socket_fs) {
    t_proceso_en_memoria* proceso_en_memoria = buscar_proceso_en_memoria(pid);
    liberar_swap(proceso_en_memoria->paginas_en_memoria, pid, socket_fs);
    //Hace falta un send o response?
    list_remove_element(procesos_en_memoria,proceso_en_memoria);
    free(procesos_en_memoria); 
}

static void liberar_swap(t_list* paginas_a_liberar, int pid, int socket_fs) {
    t_paquete* paquete = crear_paquete(LIBERAR_SWAP);
    //agregar_array_cadenas_a_paquete(paquete, paginas_a_liberar); TODO 
    agregar_entero_a_paquete(paquete, pid);
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);
}


/*
// Inicializamos el bit_array
void inicializar_bit_array(int cantidad_paginas_proceso) {
    bytes_necesarios = floor(cantidad_paginas_proceso / 8);
    status_tabla_paginas = bitarray_create_with_mode((char*)malloc(bytes_necesarios), bytes_necesarios, MSB_FIRST);
}

int acceso_a_tabla_de_paginas(int numero_de_pagina, int pid) {
// Checkeo
if (bitarray_test_bit(status_tabla_paginas, numero_de_pagina)) {
    int numero_de_marco = buscar_marco(pid, numero_de_pagina);
    return numero_de_marco;
} else {
    // Page_Fault
    }
}
*/