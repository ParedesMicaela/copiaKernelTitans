#include "memoria.h"

//================================================= Funciones Internas ================================================
static t_list* buscar_paginas_MP();
static void reemplazar_con_FIFO(t_list* paginas_totales, t_pagina* pagina_reemplazante, t_proceso_en_memoria* proceso_en_memoria);
static void reemplazar_pagina(t_pagina* pagina_reemplazante, t_proceso_en_memoria* proceso_en_memoria);
static bool presente(t_pagina* pagina);
static void loggear_reemplazo(t_pagina* pagina_a_reemplazar, t_pagina* pagina_reemplazante);
static void reemplazar_con_LRU(t_list* paginas_totales, t_pagina* pagina_reemplazante, t_proceso_en_memoria* proceso_en_memoria);
static bool memoria_llena();
static int buscar_marco_libre();

//================================================= Reemplazo de paginas ====================================================
void escribir_en_memoria_principal(t_pagina* pagina_recibida){
    
    int pid = pagina_recibida->id;
    t_proceso_en_memoria* proceso_en_memoria = buscar_proceso_en_memoria(pid);
    
    if(memoria_llena()){
        reemplazar_pagina(pagina_recibida, proceso_en_memoria);
    }else{
        int marco = buscar_marco_libre();
        //memcpy?
        pagina_recibida->bit_de_presencia = 1;
        pagina_recibida->bit_modificado = 0;
        pagina_recibida->marco = marco;

        list_add(proceso_en_memoria->paginas_en_memoria, (void*)pagina_recibida);
    }
    
}

static bool memoria_llena() {
    bool estoy_full = false;
    int tamanio_memoria = config_valores_memoria.tam_memoria;
    int tam_pagina = config_valores_memoria.tam_pagina;
    int cantidad_maxima_de_paginas_en_memoria =  tamanio_memoria/tam_pagina;


    int cantidad_paginas_procesos;
    for (int i = 0; i < list_size(procesos_en_memoria); i++) {
    t_proceso_en_memoria* proceso = list_get(procesos_en_memoria, i);
    cantidad_paginas_procesos += proceso->cantidad_entradas;
    }

    if(cantidad_paginas_procesos >= cantidad_maxima_de_paginas_en_memoria) {
        estoy_full = true;
    }

    return estoy_full;
}

static void reemplazar_pagina(t_pagina* pagina_reemplazante, t_proceso_en_memoria* proceso_en_memoria){ 
	
	//BUSCO TODAS LAS PAGINAS QUE SE PUEDAN REEMPLAZAR
	t_list* paginas_en_MP = buscar_paginas_MP(); 
		
	if(list_is_empty(paginas_en_MP)){
		log_error(memoria_logger, "No hay paginas para sacar de la memoria");
	}
	
	if(string_equals_ignore_case(config_valores_memoria.algoritmo_reemplazo, "LRU")){
		reemplazar_con_LRU(paginas_en_MP, pagina_reemplazante, proceso_en_memoria);
	}
	else if(string_equals_ignore_case(config_valores_memoria.algoritmo_reemplazo, "FIFO")){
		reemplazar_con_FIFO(paginas_en_MP, pagina_reemplazante, proceso_en_memoria);
	}
	
	list_destroy(paginas_en_MP);
}
static bool presente(t_pagina* pagina)
	{
		return (pagina->bit_de_presencia == 1);
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
		escribir_en_swap(pagina_a_reemplazar);
	}
	else
	{
        //No está en MP
		pagina_a_reemplazar->bit_de_presencia = 0;
        //desocupar_frame(pagina_a_reemplazar->marco);
	}
    
    loggear_reemplazo(pagina_a_reemplazar, pagina_reemplazante);

    list_remove_element(proceso_en_memoria->paginas_en_memoria, (void*)pagina_a_reemplazar);
    list_add(proceso_en_memoria->paginas_en_memoria, (void*)pagina_reemplazante);

    free(pagina_a_reemplazar);
}

static void reemplazar_con_FIFO(t_list* paginas_totales, t_pagina* pagina_reemplazante, t_proceso_en_memoria* proceso_en_memoria) {

    //Ordeno por quien se creó primero
    list_sort(paginas_totales, (void*) mas_vieja); 

    //Agarro la más vieja
    t_pagina* pagina_a_reemplazar = list_get(paginas_totales, 0);

    //Si esta modificada la escribo en SWAP
	if(pagina_a_reemplazar->bit_modificado == 1)
	{
		escribir_en_swap(pagina_a_reemplazar);
	}
	else
	{
        //No está en MP
		pagina_a_reemplazar->bit_de_presencia = 0;
        //desocupar_frame(pagina_a_reemplazar->marco);
	}

    loggear_reemplazo(pagina_a_reemplazar, pagina_reemplazante);

    list_remove_element(proceso_en_memoria->paginas_en_memoria, (void*)pagina_a_reemplazar);
    list_add(proceso_en_memoria->paginas_en_memoria, (void*)pagina_reemplazante);

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

    for (int i = 0; i < list_size(procesos_en_memoria); i++) {
        t_proceso_en_memoria* proceso = list_get(procesos_en_memoria, i);

            // Filtramos por las paginas que tengan P=1
            t_list* paginas_presentes = list_filter(proceso->paginas_en_memoria, (void*)presente);

            //Lo copiamos en la lista que retornamos
            list_add_all(paginas_en_MP, paginas_presentes);

            list_destroy(paginas_presentes);
    }

    return paginas_en_MP;
}

static int buscar_marco_libre() {
    t_list* paginas_en_MV = list_create();

    for (int i = 0; i < list_size(procesos_en_memoria); i++) {
        t_proceso_en_memoria* proceso = list_get(procesos_en_memoria, i);

        // Filtramos por las paginas que tengan P=0
        paginas_en_MV = list_filter(proceso->paginas_en_memoria, !(void*)presente);
    }

    //obtenemos la primer pagina de esa lista
    t_pagina* pag_en_mv = list_get(paginas_en_MV, 0);

    //borramos la lista y obtenemos el primer marco
    list_destroy(paginas_en_MV);
    return pag_en_mv->marco;
}