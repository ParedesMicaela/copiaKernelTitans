#include "memoria.h"

//================================================= Variables Globales ================================================
void* espacio_usuario;
t_list* marcos_memoria;
t_bitarray* mapa_bits_principal;
t_bitarray* mapa_bits_swap;
t_list* procesos_en_memoria;
t_list* paginas_en_memoria;
t_bitarray* status_tabla_paginas = NULL;
int tiempo = 0;
int tiempo_carga = 0;
pthread_mutex_t mutex_tiempo;
t_memoria_principal memoria;
sem_t swap_finalizado;

t_list* bloques_reservados;
//================================================= Funciones Internas ================================================
static void liberar_swap(t_proceso_en_memoria* proceso);
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
}

void liberar_espacio_usuario() {
	free (espacio_usuario);
}

//======================================================= INICIALIZACIONES =========================================================================================================

void crear_tablas_paginas_proceso(int pid, int cantidad_bytes_proceso, char* path_recibido){

    int cantidad_paginas_proceso = cantidad_bytes_proceso / config_valores_memoria.tam_pagina;

    t_proceso_en_memoria* proceso_en_memoria = malloc(sizeof(t_proceso_en_memoria));
    proceso_en_memoria->pid = pid;
    proceso_en_memoria->bloques_reservados = list_create();
    proceso_en_memoria->paginas_en_memoria = list_create();
    proceso_en_memoria->cantidad_entradas = cantidad_paginas_proceso;

    // Leemos el path antes de guardarlo en el proceso en memoria
	char* instrucciones_leidas = leer_archivo_instrucciones(path_recibido);

    free(path_recibido);

    proceso_en_memoria->path_proceso = strdup(instrucciones_leidas);

    free(instrucciones_leidas);

    list_add(procesos_en_memoria, (void*)proceso_en_memoria);
    
    log_info(memoria_logger, "PID: %d - Tamaño: %d", pid, cantidad_paginas_proceso);
}

void inicializar_la_tabla_de_paginas(int tamanio_memoria, int tamanio_pagina) {
    
    int cantidad_maxima_de_paginas_en_memoria =  tamanio_memoria/tamanio_pagina;

    //voy a crear cantidad de marcos dependiendo del tamanio de mi memoria
    memoria.marcos = malloc(cantidad_maxima_de_paginas_en_memoria * sizeof(t_marco));
    memoria.cantidad_marcos = cantidad_maxima_de_paginas_en_memoria;

    for (int i = 0; i < cantidad_maxima_de_paginas_en_memoria; i++) {
        memoria.marcos[i].ocupado = false;
    }
}

int obtener_tiempo(){
	pthread_mutex_lock(&mutex_tiempo);
	tiempo++;
	pthread_mutex_unlock(&mutex_tiempo);
	return tiempo;
}

int obtener_tiempo_carga(){
	pthread_mutex_lock(&mutex_tiempo);
	tiempo_carga++;
	pthread_mutex_unlock(&mutex_tiempo);
	return tiempo_carga;
}

//======================================================= BUSCAR_PAGINA =========================================================================================================

t_proceso_en_memoria* buscar_proceso_en_memoria(int pid) { //Saque los mutex

    int i;
    for (i = 0; i < list_size(procesos_en_memoria); i++) {
        if (((t_proceso_en_memoria*)list_get(procesos_en_memoria, i))->pid == pid) {
            return list_get(procesos_en_memoria, i);
        }
    }

    return NULL;
}


t_pagina* buscar_pagina(int pid, int num_pagina) 
{
    t_proceso_en_memoria* proceso_en_memoria = buscar_proceso_en_memoria(pid); 

    if (proceso_en_memoria == NULL) {
        return NULL;
    }

    if(list_size(proceso_en_memoria->paginas_en_memoria) == 0)
    {
        return NULL;  
    }else{

        for (int i = 0; i < list_size(proceso_en_memoria->paginas_en_memoria); i++) {        
            t_pagina* pagina_actual = list_get(proceso_en_memoria->paginas_en_memoria, i);

            if (pagina_actual->numero_de_pagina == num_pagina) {
                return pagina_actual;
            }
        }
    }

    return NULL;
}

//Dentro de las paginas del proceso
int buscar_marco(int pid, int num_pagina){

    t_pagina* pagina = buscar_pagina(pid, num_pagina);

    if (pagina == NULL) {
        return -1; //Si el marco es -1, significa que hay page_fault 
    }else if(pagina->bit_de_presencia == 0)
    {
        return -1; //Si la pagina es -1, significa que hay page_fault 
    }   
    else {
        log_info(memoria_logger, "Acceso a tabla de paginas: PID: %d - Página: %d - Marco: %d", pid, num_pagina, pagina->marco); 
        return pagina->marco;
    }
}

void desocupar_marco(int nro_marco)
{
    for (int i = 0; i < memoria.cantidad_marcos; i++) {
        if (i == nro_marco) {
            memoria.marcos[i].ocupado = false;
        }
    }
}

//======================================================= FINALIZAR_PROCESO =========================================================================================================

void finalizar_en_memoria(int pid) {
    t_proceso_en_memoria* proceso_en_memoria = buscar_proceso_en_memoria(pid);

     if (proceso_en_memoria->path_proceso != NULL) {
    free(proceso_en_memoria->path_proceso);
    }
    
    liberar_swap(proceso_en_memoria);
    liberar_paginas(proceso_en_memoria);

    list_remove_element(procesos_en_memoria,proceso_en_memoria);

    free(proceso_en_memoria); 
}

static void liberar_paginas(t_proceso_en_memoria* proceso_en_memoria) {

    int cantidad_de_paginas_a_liberar = list_size(proceso_en_memoria->paginas_en_memoria);

    log_info(memoria_logger, "PID: %d - Tamaño: %d\n", proceso_en_memoria->pid, cantidad_de_paginas_a_liberar);

    liberar_tabla_de_paginas(proceso_en_memoria);
    free_list(proceso_en_memoria->bloques_reservados);
    list_destroy(proceso_en_memoria->bloques_reservados);

}

static void liberar_swap(t_proceso_en_memoria* proceso) {

    t_paquete* paquete = crear_paquete(LIBERAR_SWAP);
    agregar_entero_a_paquete(paquete, proceso->pid);
    agregar_lista_de_cadenas_a_paquete(paquete, proceso->bloques_reservados);
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);

	log_info(memoria_logger,"Enviando pedido de liberacion de bloques en swap\n");
}

static void liberar_tabla_de_paginas(t_proceso_en_memoria* proceso) {
   
    int cantidad_de_paginas = list_size(proceso->paginas_en_memoria);

    for (int i = 0; i < cantidad_de_paginas; i++) {

        t_pagina* pagina = list_get(proceso->paginas_en_memoria, i);

        if(pagina != NULL) {
            desocupar_marco(pagina->marco); 
            //free(pagina->marco);       
            free(pagina);
        }
        
    }

    list_destroy(proceso->paginas_en_memoria);
}