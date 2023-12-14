#include "memoria.h"

//================================================= Funciones Internas ================================================
static t_list* buscar_paginas_MP();
static void reemplazar_con_FIFO(t_list* paginas_totales, int nro_pagina_reemplazante, int posicion_swap, t_proceso_en_memoria* proceso_en_memoria);
static void reemplazar_pagina(int nro_pagina_reemplazante, int posicion_swap, t_proceso_en_memoria* proceso_en_memoria);
static bool presente(t_pagina* pagina);
static bool no_presente(t_pagina* pagina);
static void loggear_reemplazo(t_pagina* pagina_a_reemplazar, t_pagina* pagina_reemplazante);
static void reemplazar_con_LRU(t_list* paginas_totales, int nro_pagina_reemplazante, int posicion_swap, t_proceso_en_memoria* proceso_en_memoria);
static bool memoria_llena();
static int buscar_marco_libre();

//================================================= Reemplazo de paginas ====================================================
void escribir_en_memoria_principal(int nro_pagina, int posicion_swap, int pid){
    
    int marco = 0;
    t_proceso_en_memoria* proceso_en_memoria = buscar_proceso_en_memoria(pid);

    //si no esta en memoria esto me da -1, no puedo mandarla a reemplazar pagina de una
    t_pagina* pagina_recibida = buscar_pagina(pid, nro_pagina);

    marco = buscar_marco_libre();

    //esto es lo mismo pero para estar seguros
    if(memoria_llena() || marco == -1){
        reemplazar_pagina(nro_pagina,posicion_swap, proceso_en_memoria);
    }else{
        
        //cargamos una nueva pagina en memoria
        crear_nueva_pagina(proceso_en_memoria, nro_pagina, posicion_swap, marco);      
    }
}

//creamos una nueva pagina en la lista de paginas del proceso cada vez que hago swap in
void crear_nueva_pagina(t_proceso_en_memoria* proceso_en_memoria, int nro_pagina, int posicion_swap, int marco)
{
    t_pagina* pagina_nueva = malloc(sizeof(t_pagina));

    pagina_nueva->id = proceso_en_memoria->pid;
    pagina_nueva->numero_de_pagina = nro_pagina;
    pagina_nueva->bit_de_presencia = 1;
    pagina_nueva->bit_modificado = 0;
    pagina_nueva->marco = marco;
    pagina_nueva->ocupado = true;
    pagina_nueva->posicion_swap = posicion_swap;
    pagina_nueva->tiempo_uso = obtener_tiempo();
    pagina_nueva->tiempo_de_carga = obtener_tiempo_carga();

    //guardo las paginas presentes en memoria
    list_add(proceso_en_memoria->paginas_en_memoria, (void*)pagina_nueva);

    log_info(memoria_logger, "SWAP IN -  PID: %d - Marco: %d - Page In: %d - %d",proceso_en_memoria->pid, pagina_nueva->marco, proceso_en_memoria->pid, nro_pagina);
}

static bool memoria_llena() {

    bool estoy_full = false;
    int tamanio_memoria = config_valores_memoria.tam_memoria;
    int tam_pagina = config_valores_memoria.tam_pagina;
    int cantidad_maxima_de_paginas_en_memoria =  tamanio_memoria/tam_pagina;

    int paginas_presentes_en_memoria = 0;

    for (int i = 0; i < list_size(procesos_en_memoria); i++) {

        //agarro cada proceso en memoria
        t_proceso_en_memoria* proceso = list_get(procesos_en_memoria, i);

        //cuando cuantas paginas tiene presente en memoria
        paginas_presentes_en_memoria += list_size(proceso->paginas_en_memoria);
    }

    if(paginas_presentes_en_memoria == cantidad_maxima_de_paginas_en_memoria)
    {
        estoy_full = true;
    }

    return estoy_full;
}

static void reemplazar_pagina(int nro_pagina_reemplazante, int posicion_swap, t_proceso_en_memoria* proceso_en_memoria){ 
	
	//BUSCO TODAS LAS PAGINAS QUE SE PUEDAN REEMPLAZAR
	t_list* paginas_en_MP = buscar_paginas_MP(); 
		
	if(list_is_empty(paginas_en_MP)){
		log_error(memoria_logger, "No hay paginas para sacar de la memoria\n");
	}
	
	if(string_equals_ignore_case(config_valores_memoria.algoritmo_reemplazo, "LRU")){
		reemplazar_con_LRU(paginas_en_MP, nro_pagina_reemplazante, posicion_swap, proceso_en_memoria);
	}
	else if(string_equals_ignore_case(config_valores_memoria.algoritmo_reemplazo, "FIFO")){
		reemplazar_con_FIFO(paginas_en_MP, nro_pagina_reemplazante, posicion_swap, proceso_en_memoria);
	}
	
	list_destroy(paginas_en_MP);
}

int menos_usada(t_pagina* una_pag, t_pagina* otra_pag)
{
    //las paginas con numero mas grande van primero (u=1 se uso hace mas tiempo que u=8)
    return (otra_pag->tiempo_uso > una_pag->tiempo_uso); 
}

static void reemplazar_con_LRU(t_list* paginas_totales, int nro_pagina_reemplazante, int posicion_swap, t_proceso_en_memoria* proceso_en_memoria){
	
    //ordeno la lista para que queden las mas vieja como primera en la lista
	list_sort(paginas_totales, (void*) menos_usada); 
	
	t_pagina* pagina_a_reemplazar = list_get(paginas_totales, 0);
	
    //Si esta modificada la escribo en SWAP
	if(pagina_a_reemplazar->bit_modificado == 1)
	{
        pagina_a_reemplazar->bit_de_presencia = 0;
        pagina_a_reemplazar->bit_modificado = 0;
		escribir_en_swap(pagina_a_reemplazar, proceso_en_memoria->pid);
	}
    
    t_proceso_en_memoria* proceso_robado = buscar_proceso_en_memoria(pagina_a_reemplazar->id);
    list_remove_element(proceso_robado->paginas_en_memoria, (void*)pagina_a_reemplazar);

    //creo la nueva pagina con el marco que me robe
    crear_nueva_pagina(proceso_en_memoria, nro_pagina_reemplazante, posicion_swap, pagina_a_reemplazar->marco);

    t_pagina* pagina_reemplazante = buscar_pagina(proceso_en_memoria->pid, nro_pagina_reemplazante);

    loggear_reemplazo(pagina_a_reemplazar, pagina_reemplazante);
    
}

static void reemplazar_con_FIFO(t_list* paginas_totales, int nro_pagina_reemplazante, int posicion_swap, t_proceso_en_memoria* proceso_en_memoria) {

    //ordeno por quien se creó primero
    list_sort(paginas_totales, (void*) mas_vieja); 

    //Agarro la más vieja
    t_pagina* pagina_a_reemplazar = list_get(paginas_totales, 0);

    //Si esta modificada la escribo en SWAP
	if(pagina_a_reemplazar->bit_modificado == 1)
	{
        pagina_a_reemplazar->bit_de_presencia = 0;
        pagina_a_reemplazar->bit_modificado = 0;
		escribir_en_swap(pagina_a_reemplazar, proceso_en_memoria->pid);
	}

    //le saco la pagina al proceso al que le robe el marco
    t_proceso_en_memoria* proceso_robado = buscar_proceso_en_memoria(pagina_a_reemplazar->id);
    list_remove_element(proceso_robado->paginas_en_memoria, (void*)pagina_a_reemplazar);

    //creo la nueva pagina con el marco que me robe
    crear_nueva_pagina(proceso_en_memoria, nro_pagina_reemplazante, posicion_swap, pagina_a_reemplazar->marco);

    t_pagina* pagina_reemplazante = buscar_pagina(proceso_en_memoria->pid, nro_pagina_reemplazante);

    loggear_reemplazo(pagina_a_reemplazar, pagina_reemplazante);
}

int mas_vieja(t_pagina* una_pag, t_pagina* otra_pag)
{
    //las paginas con numero mas chico van primero (t=1 se cargo antes que t=8)
    return (otra_pag->tiempo_de_carga > una_pag->tiempo_de_carga); 
}

static void loggear_reemplazo(t_pagina* pagina_a_reemplazar, t_pagina* pagina_reemplazante) {
    log_info(memoria_logger, "REEMPLAZO - Marco: %d - Page Out: %d - %d - Page In: %d - %d\n", pagina_a_reemplazar->marco, pagina_a_reemplazar->id, pagina_a_reemplazar->numero_de_pagina, pagina_reemplazante->id, pagina_reemplazante->marco);
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

static bool presente(t_pagina* pagina)
	{
		return (pagina->bit_de_presencia == 1);
	}

static bool no_presente(t_pagina* pagina)
	{
		return (pagina->bit_de_presencia == 0);
	}

static int buscar_marco_libre()
{
    for (int i = 0; i < memoria.cantidad_marcos; i++) {
        if (memoria.marcos[i].ocupado == false) {
            memoria.marcos[i].ocupado = true;
            return i; 
        }
    }

    return -1; // Devuelve -1 si no hay marcos libres
}
