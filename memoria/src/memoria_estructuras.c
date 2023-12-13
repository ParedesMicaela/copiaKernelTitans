#include "memoria.h"

//================================================= Variables Globales ================================================
void* espacio_usuario;
t_list* marcos_memoria;
t_bitarray* mapa_bits_principal;
t_bitarray* mapa_bits_swap;
t_list* procesos_en_memoria;
t_list* paginas_totales_en_memoria;
t_bitarray* status_tabla_paginas = NULL;
int tiempo = 0;
int tiempo_carga = 0;
pthread_mutex_t mutex_tiempo;
int cantidad_de_paginas_totales = 0;
pthread_mutex_t contador_paginas;
pthread_mutex_t mutex_tabla_de_paginas;


t_list* bloques_reservados;
//================================================= Funciones Internas ================================================
static void liberar_swap(int pid);
static void liberar_paginas(t_proceso_en_memoria* proceso_en_memoria);
static void liberar_tabla_de_paginas(t_proceso_en_memoria* proceso);
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

void crear_tablas_paginas_proceso(int pid, int cantidad_bytes_proceso, char* path_recibido){

    int tamanio_pagina = config_valores_memoria.tam_pagina;
    int cantidad_paginas_de_proceso = cantidad_bytes_proceso / tamanio_pagina;

    t_proceso_en_memoria* proceso_en_memoria = malloc(sizeof(t_proceso_en_memoria));
    proceso_en_memoria->pid = pid;
    proceso_en_memoria->bloques_reservados = list_create(); //ERROR ACA
    proceso_en_memoria->cantidad_paginas = cantidad_paginas_de_proceso;
    proceso_en_memoria->paginas_asignadas = list_create();

    // Leemos el path antes de guardarlo en el proceso en memoria
	char* instrucciones_leidas = leer_archivo_instrucciones(path_recibido);
    proceso_en_memoria->path_proceso = string_duplicate(instrucciones_leidas); 

    free(instrucciones_leidas);

    inicializar_la_tabla_de_paginas(cantidad_paginas_de_proceso, pid);
    
    pthread_mutex_lock(&mutex_procesos);
    list_add(procesos_en_memoria, (void*)proceso_en_memoria); 
    pthread_mutex_unlock(&mutex_procesos);
    
    log_info(memoria_logger, "PID: %d - Tamaño: %d", pid, cantidad_paginas_de_proceso);
}

void inicializar_la_tabla_de_paginas(int cantidad_paginas_de_proceso, int pid) {
    
    int cantidad_paginas_a_crear = cantidad_paginas_de_proceso - 1;

    for (int i = 0; i <= cantidad_paginas_a_crear; i++) {

        //creo una pagina por cada iteracion
        t_pagina* tp = malloc(sizeof(t_pagina)); 

        tp->id = pid;
        tp->numero_de_pagina = i;
        tp->marco = i;
        tp->bit_de_presencia = 0;
        tp->bit_modificado = 0;
        tp->posicion_swap = 0; 
        tp->tiempo_uso = 0;
        tp->tiempo_de_carga = 0;

        pthread_mutex_lock(&mutex_tabla_de_paginas);
        list_add(tabla_de_paginas, (t_pagina*)tp);
        pthread_mutex_unlock(&mutex_tabla_de_paginas);
    }
}

int obtener_tiempo(){
	pthread_mutex_lock(&mutex_tiempo);
	int t = tiempo;
	tiempo++;
	pthread_mutex_unlock(&mutex_tiempo);
	return t;
}

int obtener_tiempo_carga(){
	pthread_mutex_lock(&mutex_tiempo);
	int t = tiempo_carga;
	tiempo++;
	pthread_mutex_unlock(&mutex_tiempo);
	return t;
}

//======================================================= BUSCAR_PAGINA =========================================================================================================

t_proceso_en_memoria* buscar_proceso_en_memoria(int pid) {
    int i;
    t_proceso_en_memoria* proceso_en_memoria = NULL;

    pthread_mutex_lock(&mutex_procesos);
    int cantidad_procesos_en_memoria = list_size(procesos_en_memoria);

    for (i = 0; i < cantidad_procesos_en_memoria; i++) {
        t_proceso_en_memoria* proceso = (t_proceso_en_memoria*)list_get(procesos_en_memoria, i);
        if (proceso->pid == pid) {
            proceso_en_memoria = proceso;
            break;
        }
    }

    pthread_mutex_unlock(&mutex_procesos);

    return proceso_en_memoria;
}

t_pagina* buscar_pagina(int num_pagina) {
    pthread_mutex_lock(&mutex_tabla_de_paginas);

    int tamanio = list_size(tabla_de_paginas);
    t_pagina* pagina_encontrada = NULL;

    for (int i = 0; i < tamanio; i++) {
        t_pagina* pagina_actual = list_get(tabla_de_paginas, i);

        if (pagina_actual == NULL) {
            break;  
        }

        if (pagina_actual->numero_de_pagina == num_pagina) {
            pagina_actual->tiempo_uso = obtener_tiempo();
            pagina_encontrada = pagina_actual;
            break;  
        }
    }

    pthread_mutex_unlock(&mutex_tabla_de_paginas);

    return pagina_encontrada;
}



int buscar_marco(int num_pagina, int pid){

    t_pagina* pagina = buscar_pagina(num_pagina);
    log_info(memoria_logger, "Se buscara marco en las tablas de paginas del proceso");

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

    pthread_mutex_lock(&mutex_procesos);
    list_remove_element(procesos_en_memoria,proceso_en_memoria);
    pthread_mutex_unlock(&mutex_procesos);
    free(proceso_en_memoria); 
}

static void liberar_paginas(t_proceso_en_memoria* proceso_en_memoria) {

    if (proceso_en_memoria->path_proceso != NULL) {
    free(proceso_en_memoria->path_proceso);
    }

    int cantidad_de_paginas_a_liberar = proceso_en_memoria->cantidad_paginas;

    liberar_tabla_de_paginas(proceso_en_memoria);
    free_list(proceso_en_memoria->bloques_reservados);
    list_destroy(proceso_en_memoria->bloques_reservados);

    log_info(memoria_logger, "PID: %d - Paginas a liberar: %d\n", proceso_en_memoria->pid, cantidad_de_paginas_a_liberar);

}

static void liberar_swap(int pid) {
    t_paquete* paquete = crear_paquete(LIBERAR_SWAP);
    agregar_entero_a_paquete(paquete, pid);
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);

	log_info(memoria_logger,"Enviando pedido de liberacion de bloques en swap\n");
}

static void liberar_tabla_de_paginas(t_proceso_en_memoria* proceso) {
   
    int cantidad_de_paginas = list_size(proceso->paginas_asignadas);

    for (int i = 0; i < cantidad_de_paginas; i++) {

        t_pagina* pagina = list_get(proceso->paginas_asignadas, i);

        if(pagina != NULL) {
            free(pagina);
        }
        
    }

    list_destroy(proceso->paginas_asignadas);
}
