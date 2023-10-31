#include "memoria.h"

//================================================= Variables Globales ================================================
void* espacio_usuario;
t_list* marcos_memoria;
t_bitarray* mapa_bits_principal;
t_bitarray* mapa_bits_swap;
t_list* procesos_en_memoria;

t_bitarray* status_tabla_paginas = NULL;
int numero_paginas;
int bytes_necesarios;

//================================================= Creacion Estructuras ====================================================
// Inicializamos el bit_array
void inicializar_bit_array() {
    numero_paginas = floor(config_valores_memoria.tam_memoria / config_valores_memoria.tam_pagina);
    bytes_necesarios = floor(numero_paginas / 8);
    status_tabla_paginas = bitarray_create_with_mode((char*)malloc(bytes_necesarios), bytes_necesarios, MSB_FIRST);
}

/// @brief Espacio Usuario ///
void creacion_espacio_usuario(){
    espacio_usuario = malloc (config_valores_memoria.tam_memoria); 
	if (espacio_usuario == NULL) {
        perror ("No se pudo alocar memoria al espacio de usuario.");
        //que pasa si no se puede, abort?
    }
	liberar_espacio_usuario(); //atexit? 
}

void liberar_espacio_usuario() {
	free (espacio_usuario);
}

/// @brief Memoria Swap ///
void crear_memoria_swap(int tam_swap_pid) {

    int cantidad_marcos = tam_swap_pid / config_valores_memoria.tam_pagina;
    log_info(memoria_logger, "Se crearan %d marcos para swap", cantidad_marcos);   

    int num_bytes = cantidad_marcos / 8;

	void* aux = malloc(num_bytes);
	mapa_bits_swap = bitarray_create_with_mode(aux, num_bytes, MSB_FIRST);

	for (int i=0; i < cantidad_marcos; i++)
	{
		bitarray_clean_bit(mapa_bits_swap, i);
	}
}

void crear_memoria_principal(){
    procesos_en_memoria = list_create();
    int tam_pagina = config_valores_memoria.tam_pagina;
    int tam_memoria = config_valores_memoria.tam_memoria;
    int ints_por_pagina = tam_pagina / 4; //ChAt: tam_pagina / sizeof(uint32_t);
    int cantidad_marcos = tam_memoria / tam_pagina;
    marcos_memoria = list_create();

    log_info(memoria_logger, "Se crearan %d marcos para la memoria principal", cantidad_marcos);   

    // CHat:  page_table = inicializar_la_tabla_de_paginas(num_pages);

    int num_bytes = cantidad_marcos / 8;
	void* aux = malloc(num_bytes);
	mapa_bits_principal = bitarray_create_with_mode(aux, num_bytes, MSB_FIRST);
    
    //cantidad de marcos de la memoria principal
	for (int i=0; i < cantidad_marcos; i++)
	{
		bitarray_clean_bit(mapa_bits_principal, i);
        t_list* lista_datos = list_create();

        //Se itera los ints por pagina para crear contenido del marco
        for (int j=0; j < ints_por_pagina; j++) {
            uint32_t* dato = malloc(sizeof(uint32_t));
            list_add(lista_datos, dato);
        }
        list_add(marcos_memoria, lista_datos);
	}

    return;
}

void crear_tablas_paginas_proceso(int pid, int tam_swap_pid){
    
    t_proceso_en_memoria* proceso_en_memoria = malloc(sizeof(t_proceso_en_memoria));
    proceso_en_memoria->pid = pid;
    proceso_en_memoria->tam_swap = tam_swap_pid;
    proceso_en_memoria->paginas_en_memoria = list_create();
    //proceso_en_memoria->path_proceso = strdup(instrucciones_leidas);

    inicializar_la_tabla_de_paginas();

    list_add(procesos_en_memoria, proceso_en_memoria);
    
}

// Inicializar la tabla de paginas
void inicializar_la_tabla_de_paginas() {
    t_pagina* tp = (t_pagina*)malloc(sizeof(t_pagina));
    tp->tamanio = numero_paginas;
    tp->entradas = (entrada_t_pagina*)malloc(sizeof(entrada_t_pagina) * numero_paginas);

    for (int i = 0; i < numero_paginas; i++) {
        tp->entradas[i].numero_de_pagina = i;
        tp->entradas[i].marco = i;
        tp->entradas[i].bit_de_presencia = 0;
        tp->entradas[i].bit_modificado = 0;
        tp->entradas[i].posicion_swap = -1; // No en memoria

        // Marco la pagina que no está en memoria
        bitarray_clean_bit(status_tabla_paginas, i);
    }
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
    return list_get(proceso_en_memoria->paginas_en_memoria, num_pagina);
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