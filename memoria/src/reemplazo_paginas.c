#include "memoria.h"

//================================================= Funciones Internas ================================================
static t_list* buscar_paginas_MP();
static void reemplazar_con_FIFO(t_list* paginas_totales, t_pagina* pagina_reemplazante, t_proceso_en_memoria* proceso_en_memoria);
static void reemplazar_pagina(t_pagina* pagina_reemplazante, t_proceso_en_memoria* proceso_en_memoria);
static bool presente(t_pagina* pagina);
static bool no_presente(t_pagina* pagina);
static void loggear_reemplazo(t_pagina* pagina_a_reemplazar, t_pagina* pagina_reemplazante);
static void reemplazar_con_LRU(t_list* paginas_totales, t_pagina* pagina_reemplazante, t_proceso_en_memoria* proceso_en_memoria);
static bool memoria_llena();
static int buscar_marco_libre();

//================================================= Reemplazo de paginas ====================================================
void escribir_en_memoria_principal(int nro_pagina, int posicion_swap, int pid){
    
    t_proceso_en_memoria* proceso_en_memoria = buscar_proceso_en_memoria(pid);

    t_pagina* pagina_recibida = buscar_pagina(nro_pagina);

    if(memoria_llena()){
        printf("\n entro a reemplazo\n");
        reemplazar_pagina(pagina_recibida, proceso_en_memoria);
    }else{
        
        //memcpy?
        int marco = buscar_marco_libre();
        pagina_recibida->id = pid;
        pagina_recibida->bit_de_presencia = 1;
        pagina_recibida->bit_modificado = 0;
        pagina_recibida->marco = marco;
        pagina_recibida->posicion_swap = posicion_swap;
        pagina_recibida->tiempo_uso = obtener_tiempo();
        pagina_recibida->tiempo_de_carga = obtener_tiempo_carga();

        pthread_mutex_lock(&mutex_tabla_de_paginas);
        list_add(tabla_de_paginas, (void*)pagina_recibida);
        pthread_mutex_unlock(&mutex_tabla_de_paginas);

        list_add(proceso_en_memoria->paginas_asignadas, (void*)pagina_recibida);
    }

    log_info(memoria_logger, "SWAP IN -  PID: %d - Marco: %d - Page In: %d - %d",pid, pagina_recibida->marco, pid, nro_pagina);
}

static bool memoria_llena() {
    bool estoy_full = false;
    int tamanio_memoria = config_valores_memoria.tam_memoria;
    int tam_pagina = config_valores_memoria.tam_pagina;
    int cantidad_maxima_de_paginas_en_memoria = tamanio_memoria / tam_pagina;
    int cantidad_paginas_procesos = 0;
    int cantidad_paginas_en_memoria = 0;

    pthread_mutex_lock(&mutex_tabla_de_paginas);
    int tamanio_tabla_paginas = list_size(tabla_de_paginas);
    pthread_mutex_unlock(&mutex_tabla_de_paginas);

    pthread_mutex_lock(&mutex_procesos);
    int cantidad_procesos_en_memoria = list_size(procesos_en_memoria);
    pthread_mutex_unlock(&mutex_procesos);

    // De cada proceso en memoria obtenemos sus paginas en memoria
    for (int i = 0; i < cantidad_procesos_en_memoria; i++) {

        pthread_mutex_lock(&mutex_procesos);
        t_proceso_en_memoria* proceso = list_get(procesos_en_memoria, i);
        pthread_mutex_unlock(&mutex_procesos);

        // Cantidad total de paginas asignadas a procesos
        pthread_mutex_lock(&mutex_tabla_de_paginas);
        cantidad_paginas_en_memoria += list_size(proceso->paginas_asignadas);
        pthread_mutex_unlock(&mutex_tabla_de_paginas);
    }
    
    if (cantidad_paginas_en_memoria >= tamanio_tabla_paginas || buscar_marco_libre() == -1) {
        estoy_full = true;
    }

    return estoy_full;
}

static void reemplazar_pagina(t_pagina* pagina_reemplazante, t_proceso_en_memoria* proceso_en_memoria){ 
	
	//BUSCO TODAS LAS PAGINAS QUE SE PUEDAN REEMPLAZAR
    t_list* paginas_en_MP = list_create();
	paginas_en_MP = buscar_paginas_MP(); 
		
    printf("\n estoy en  reemplazo\n");

	if(list_is_empty(paginas_en_MP)){
		log_error(memoria_logger, "No hay paginas para sacar de la memoria\n");

	}else if (string_equals_ignore_case(config_valores_memoria.algoritmo_reemplazo, "LRU")){
		reemplazar_con_LRU(paginas_en_MP, pagina_reemplazante, proceso_en_memoria);
	}
	else if(string_equals_ignore_case(config_valores_memoria.algoritmo_reemplazo, "FIFO")){
		reemplazar_con_FIFO(paginas_en_MP, pagina_reemplazante, proceso_en_memoria);
	}
	
	list_destroy(paginas_en_MP);
}

int menos_usada(t_pagina* una_pag, t_pagina* otra_pag)
{
    return (otra_pag->tiempo_uso > una_pag->tiempo_uso); 
}

static void reemplazar_con_LRU(t_list* paginas_totales, t_pagina* pagina_reemplazante, t_proceso_en_memoria* proceso_en_memoria){
	
    //ordeno la lista para que queden las mas vieja como primera en la lista
	list_sort(paginas_totales, (void*) menos_usada); 
	
	t_pagina* pagina_a_reemplazar = list_get(paginas_totales, 0);
	
    //Si esta modificada la escribo en SWAP
	if(pagina_a_reemplazar->bit_modificado == 1)
	{
		escribir_en_swap(pagina_a_reemplazar, proceso_en_memoria->pid);
	}
	/*else
	{
        //No está en MP
		pagina_a_reemplazar->bit_de_presencia = 0;
        //desocupar_frame(pagina_a_reemplazar->marco);
	}*/
	pagina_a_reemplazar->bit_de_presencia = 0;
    pagina_reemplazante->marco = pagina_a_reemplazar->marco;
    
    loggear_reemplazo(pagina_a_reemplazar, pagina_reemplazante);

    pthread_mutex_lock(&mutex_tabla_de_paginas);
    list_remove_element(tabla_de_paginas, (void*)pagina_a_reemplazar);
    list_add(tabla_de_paginas, (void*)pagina_reemplazante);
    pthread_mutex_unlock(&mutex_tabla_de_paginas);

    free(pagina_a_reemplazar);

}

static void reemplazar_con_FIFO(t_list* paginas_totales, t_pagina* pagina_reemplazante, t_proceso_en_memoria* proceso_en_memoria) {

    //Ordeno por quien se creó primero
    list_sort(paginas_totales, (void*) mas_vieja);

    if(list_size(paginas_totales) == 0){
        printf("lista esta vacia de pagina");
    } 

    //Agarro la más vieja
    t_pagina* pagina_a_reemplazar = list_get(paginas_totales, 0);

    //Si esta modificada la escribo en SWAP
	if(pagina_a_reemplazar->bit_modificado == 1)
	{
		escribir_en_swap(pagina_a_reemplazar, proceso_en_memoria->pid);
	}/*
	else
	{
        //No está en MP
		pagina_a_reemplazar->bit_de_presencia = 0;
        //desocupar_frame(pagina_a_reemplazar->marco);
	}*/

	pagina_a_reemplazar->bit_de_presencia = 0;
    pagina_reemplazante->marco = pagina_a_reemplazar->marco;

    loggear_reemplazo(pagina_a_reemplazar, pagina_reemplazante);

    pthread_mutex_lock(&mutex_tabla_de_paginas);
    list_remove_element(tabla_de_paginas, (void*)pagina_a_reemplazar);
    list_add(tabla_de_paginas, (void*)pagina_reemplazante);
    pthread_mutex_unlock(&mutex_tabla_de_paginas);

    free(pagina_a_reemplazar);

}

int mas_vieja(t_pagina* una_pag, t_pagina* otra_pag)
{
    return (otra_pag->tiempo_de_carga > una_pag->tiempo_de_carga); 
}

static void loggear_reemplazo(t_pagina* pagina_a_reemplazar, t_pagina* pagina_reemplazante) {
    log_info(memoria_logger, "REEMPLAZO - Marco: %d - Page Out: %d-%d - Page In: %d-%d", pagina_a_reemplazar->marco, pagina_a_reemplazar->id, pagina_a_reemplazar->numero_de_pagina, pagina_reemplazante->id, pagina_reemplazante->marco);
}

//================================================ BUSCAR =======================================================
static t_list* buscar_paginas_MP() {
    t_list* paginas_en_MP = list_create();

    pthread_mutex_lock(&mutex_tabla_de_paginas);
    int tamanio_tabla = list_size(tabla_de_paginas);
    pthread_mutex_unlock(&mutex_tabla_de_paginas);

    for (int i = 0; i < tamanio_tabla; i++) {

        // Filtramos por las paginas que tengan P=1
        pthread_mutex_lock(&mutex_tabla_de_paginas);
        t_list* paginas_presentes = list_filter(tabla_de_paginas, (void*)presente);
        pthread_mutex_unlock(&mutex_tabla_de_paginas);

        //Lo copiamos en la lista que retornamos
        list_add_all(paginas_en_MP, paginas_presentes);

        list_destroy(paginas_presentes);
    }

    return paginas_en_MP;
}

static bool presente(t_pagina* pagina)
	{
		return (pagina->bit_de_presencia == 1);
	}

static bool no_presente(t_pagina* pagina)
	{
		return (pagina->bit_de_presencia == 0);
	}

static int buscar_marco_libre() {
    t_list* paginas_en_MV = list_create();

    pthread_mutex_lock(&mutex_tabla_de_paginas);
    int tamanio_tabla = list_size(tabla_de_paginas);
    pthread_mutex_unlock(&mutex_tabla_de_paginas);

    for (int i = 0; i < tamanio_tabla; i++) {

        // Filtramos por las páginas que tengan P=0
        t_list* paginas_sin_presencia = list_filter(tabla_de_paginas, (void*)no_presente);

        // Concatenamos la lista filtrada a la lista principal
        list_add_all(paginas_en_MV, paginas_sin_presencia);

        // Liberamos la lista filtrada
        list_destroy(paginas_sin_presencia);
    }

    //si todos los marcos estan llenos
    if(list_size(paginas_en_MV) == 0)
    {
        return -1;
    }

    // Obtenemos la primer página de esa lista
    t_pagina* pag_en_mv = list_get(paginas_en_MV, 0);

    // Borramos la lista y obtenemos el primer marco
    list_destroy(paginas_en_MV);
    return pag_en_mv->marco;
}