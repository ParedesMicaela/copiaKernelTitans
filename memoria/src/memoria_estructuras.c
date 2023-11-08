#include "memoria.h"

//================================================= Variables Globales ================================================
void* espacio_usuario;
t_list* marcos_memoria;
t_bitarray* mapa_bits_principal;
t_bitarray* mapa_bits_swap;
t_list* procesos_en_memoria;
t_bitarray* status_tabla_paginas = NULL;

t_list* bloques_reservados;
//================================================= Funciones Internas ================================================
static void liberar_swap(t_pagina* paginas_a_liberar, int pid, int socket_fs);
static void recibir_listado_bloques_reservados(int socket_fs, int pid);
static t_proceso_en_memoria* buscar_proceso_en_memoria(int pid);
static void enviar_inicializar_swap_a_filesystem (int pid, int cantidad_paginas_proceso, int socket_fs);
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

//======================================================= INICIALIZACIONES =========================================================================================================

void crear_tablas_paginas_proceso(int pid, int cantidad_paginas_proceso, char* path_recibido){
    procesos_en_memoria = list_create();

    t_proceso_en_memoria* proceso_en_memoria = malloc(sizeof(t_proceso_en_memoria));
    proceso_en_memoria->pid = pid;
    proceso_en_memoria->bloques_reservados = list_create();

    // Leemos el path antes de guardarlo en el proceso en memoria
	char* instrucciones_leidas = leer_archivo_instrucciones(path_recibido);
    proceso_en_memoria->path_proceso = strdup(instrucciones_leidas);

    inicializar_la_tabla_de_paginas(proceso_en_memoria, cantidad_paginas_proceso);

    list_add(procesos_en_memoria, (void*)proceso_en_memoria);
    
    log_info(memoria_logger, "PID: %d - Tamaño: %d", pid, cantidad_paginas_proceso);
}

void inicializar_la_tabla_de_paginas(t_proceso_en_memoria* proceso, int cantidad_paginas_proceso) {
    proceso->paginas_en_memoria = (t_pagina*)malloc(sizeof(t_pagina));
    t_pagina* tp = proceso->paginas_en_memoria;
    tp->cantidad_entradas = cantidad_paginas_proceso;
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

void inicializar_swap_proceso(int pid, int cantidad_paginas_proceso, int socket_fs) {
    enviar_inicializar_swap_a_filesystem(pid, cantidad_paginas_proceso, socket_fs);
    recibir_listado_bloques_reservados(socket_fs, pid);
}

static void enviar_inicializar_swap_a_filesystem (int pid, int cantidad_paginas_proceso, int socket_fs) {
    t_paquete* paquete = crear_paquete(INICIALIZAR_SWAP); 
    agregar_entero_a_paquete(paquete, pid); 
    agregar_entero_a_paquete(paquete, cantidad_paginas_proceso);
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);
}

static void recibir_listado_bloques_reservados(int socket_fs, int pid)
{
    // Obtenemos proceso en memoria
    t_proceso_en_memoria* proceso_en_memoria = buscar_proceso_en_memoria(pid); 

    t_paquete* paquete = recibir_paquete(socket_fs);
    void* stream = paquete->buffer->stream;
    if (paquete->codigo_operacion == LISTA_BLOQUES_RESERVADOS)
    {

        proceso_en_memoria->bloques_reservados = sacar_lista_de_cadenas_de_paquete(&stream);
        log_info(memoria_logger, "Se ha recibido correctamente el listado de bloques");
    }
    else
    {
        log_error(memoria_logger,"No me enviaste el listado de bloques :( \n");
        abort();
    }
    eliminar_paquete(paquete);
}

//======================================================= BUSCAR_PAGINA =========================================================================================================

static t_proceso_en_memoria* buscar_proceso_en_memoria(int pid) {
    int i;
    for (i=0; i < list_size(procesos_en_memoria); i++){
        if ( ((t_proceso_en_memoria*) list_get(procesos_en_memoria, i)) -> pid == pid){
            break;            
        }
    }
    return list_get(procesos_en_memoria, i);
}

entrada_t_pagina* buscar_pagina(int pid, int num_pagina){

    // Obtenemos proceso en memoria
    t_proceso_en_memoria* proceso_en_memoria = buscar_proceso_en_memoria(pid); 

    // Iterar las entradas
    for(int i = 0; i < proceso_en_memoria->paginas_en_memoria->cantidad_entradas; i++)
    {
        // Si matchea el numero de pagina con la entrada, devuelvo la entrada
        if (proceso_en_memoria->paginas_en_memoria->entradas[i].numero_de_pagina == num_pagina)
        {
            return &(proceso_en_memoria->paginas_en_memoria->entradas[i]);
        }
    }

    // Si no consigue devuelvo NULL (Tratar después error)
    return NULL;
}

int buscar_marco(int pid, int num_pagina){

    entrada_t_pagina* pagina = buscar_pagina(pid, num_pagina);
    log_info(memoria_logger, "Se buscara marco en las tablas de paginas");

    if (pagina->bit_de_presencia == 0) {
        return -1; //Si el marco es -1, significa que hay page_fault 
    }   
    else {
        log_info(memoria_logger, "Acceso a tabla de paginas: PID: %d - Página: %d - Marco: %d", pid, num_pagina, pagina->marco); 
        return pagina->marco;
    }
}

void solucionar_page_fault(int num_pagina, int socket_fs) {
    t_paquete* paquete = crear_paquete(SOLUCIONAR_PAGE_FAULT); 
    agregar_entero_a_paquete(paquete, num_pagina); 
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);
}

//======================================================= FINALIZAR_PROCESO =========================================================================================================

void finalizar_en_memoria(int pid, int socket_fs) {
    t_proceso_en_memoria* proceso_en_memoria = buscar_proceso_en_memoria(pid);
    liberar_swap(proceso_en_memoria->paginas_en_memoria, pid, socket_fs);
    //Hace falta un send o response?
    list_remove_element(procesos_en_memoria,proceso_en_memoria);
    free(procesos_en_memoria); 
}

static void liberar_swap(t_pagina* paginas_a_liberar, int pid, int socket_fs) {
    t_paquete* paquete = crear_paquete(LIBERAR_SWAP);
    //agregar_array_cadenas_a_paquete(paquete, paginas_a_liberar); TODO 
    agregar_entero_a_paquete(paquete, pid);
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);
}

//======================================================= Reemplazar Páginas =========================================================================================================

void escribir_en_memoria_principal(){

    /*intenta escribir en memoria y si esta llena, reemplaza
    if(memoria_llena){
        reemplazar_pagina(pagina_recibida);
    }
    
    int numero_pagina = sacar_entero_de_paquete(&stream);
    int marco = sacar_entero_de_paquete(&stream);
    int posicion_swap = sacar_entero_de_paquete(&stream);
    int pid = sacar_entero_de_paquete(&stream);
    */

    int numero_pagina = 0;
    int marco = 0;
    int posicion_swap = 16;
    int pid = 1;

    t_proceso_en_memoria* proceso = buscar_proceso_en_memoria(pid);

    t_pagina* tp = proceso->paginas_en_memoria;

    tp->entradas[0].numero_de_pagina = numero_pagina;
    tp->entradas[0].marco = marco;
    tp->entradas[0].bit_de_presencia = 1;
    tp->entradas[0].bit_modificado = 1;
    tp->entradas[0].posicion_swap = posicion_swap; 

}



/*
void reemplazarPagina(){
	
	//BUSCO TODAS LAS PAGINAS QUE SE PUEDAN REEMPLAZAR
	t_list* paginasEnMp = buscarPaginasMP(); 
	
	int noLock(t_pagina* pag){
        
		return (pag->lock == 0);
	}
	
	t_list* paginasSinLock = list_filter(paginasEnMp, (void*)noLock);//FILTRO LAS QUE ESTAN LOCKEADAS -> ESTAS LAS NECESITO //TODO QUE PASA SI ESTAN TODAS LOCKEADAS???
	
	if(list_is_empty(paginasSinLock)){
		log_error(logger, "No hay paginas para sacar de la memoria ppal");
	}
	
	if(string_equals_ignore_case(config_valores.algoritmo_reemplazo, "LRU")){
		reemplazarConLRU(paginasSinLock);
	}
	else if(string_equals_ignore_case(config_valores.algoritmo_reemplazo, "FIFO")){
		reemplazarConFIFO(paginasSinLock);
	}
	
	list_destroy(paginasEnMp);
	list_destroy(paginasSinLock);	
}


//LRU
void reemplazarConLRU(t_list* paginasEnMp){
	
	int masVieja(t_pagina* unaPag, t_pagina* otraPag){
		
		return (otraPag->tiempo_uso > unaPag->tiempo_uso); //LA QUE ESTA HACE MAS TIEMPO
	}
	
	list_sort(paginasEnMp, (void*) masVieja); //FILTRO Y PONGO A LA MAS VIEJA PRIMERO -> ORDENA DE MAS VIEJA A MAS NIEVA
	
	//COMO REEMPLAZO SEGUN LRU, ELIJO LA PRIMERA QUE ES LA MAS VIEJA
	t_pagina* paginaReemplazo = list_get(paginasEnMp, 0);
	log_info(logger, "Voy a reemplazar la pagina %d que estaba en el frame %d", paginaReemplazo->id, paginaReemplazo->frame_ppal);
	setLock(paginaReemplazo);
	
	//SI EL BIT DE MODIFICADO ES 1, LA GUARDO EM MV -> PORQUE TIENE CONTENIDO DIFERENTE A LO QUE ESTA EN MV
	if(paginaReemplazo->modificado == 1)
	{
		guardarMemoriaVirtual(paginaReemplazo);
	}
	else
	{
		//EN LA MP, DESOCUPO EL FRAME EN EL BITMAP
		desocuparFrame(paginaReemplazo->frame_ppal, MEM_PPAL);
        //AHORA ESTA PAGINA YA NO ESTA EN MP, CAMBIO EL BIT DE PRESENCIA
		paginaReemplazo->presencia = 0;
		cleanLock(paginaReemplazo);
	}
}
*/
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