#include "memoria.h"

//================================================= Handshake =====================================================================
void enviar_paquete_handshake(int socket_cliente) {

	t_paquete* handshake=crear_paquete(HANDSHAKE);
	agregar_entero_a_paquete(handshake,config_valores_memoria.tam_pagina);

	enviar_paquete(handshake,socket_cliente);

	eliminar_paquete(handshake);
}

//============================================ Instrucciones a CPU =====================================================================
void enviar_paquete_instrucciones(int socket_cpu, char* instrucciones, int inst_a_ejecutar)
{
    //pthread_mutex_lock(&mutex_lista_instrucciones);

	//Separamos las instrucciones en diferentes líneas
	char** lista_instrucciones = string_split(instrucciones, "\n");

	//Mandamos la instrucción que nos dijo la CPU (PC)
    char *instruccion = lista_instrucciones[inst_a_ejecutar];

    t_paquete* paquete = crear_paquete(INSTRUCCIONES); 

    agregar_cadena_a_paquete(paquete, instruccion); 

    enviar_paquete(paquete, socket_cpu);

    //pthread_mutex_lock(&mutex_lista_instrucciones);

    free_array(lista_instrucciones);
    free(instrucciones);
	eliminar_paquete(paquete);
}

char* leer_archivo_instrucciones(char* path_instrucciones) {
    FILE* instr_f = fopen(path_instrucciones, "r");
    if (instr_f == NULL) {
        perror("No se pudo abrir el archivo de instrucciones");
        abort();
    }

    char* cadena_completa = NULL;
    char* linea = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&linea, &len, instr_f)) != -1) {
        if (cadena_completa == NULL) {
            cadena_completa = strdup(linea);
        } else {
            char* temp = malloc(strlen(cadena_completa) + strlen(linea) + 1);
            strcpy(temp, cadena_completa);
            strcat(temp, linea);
            free(cadena_completa);
            cadena_completa = temp;
        }
    }
   
    free(linea);
    fclose(instr_f);

    return cadena_completa;
}

char* buscar_path_proceso(int pid)
{
    if (procesos_en_memoria == NULL) {
        log_error(memoria_logger, "Error: lista_procesos esta vacia");
        return NULL;
    }

    for (int i = 0; i < list_size(procesos_en_memoria); i++)
    {
        t_proceso_en_memoria* proceso = list_get(procesos_en_memoria, i);

        if (proceso != NULL && proceso->pid == pid)
        {
            return strdup(proceso->path_proceso);
        }
    }

    log_error(memoria_logger, "No se encontró un proceso con PID %d", pid);
    return NULL;
}

//============================================ Instrucciones de CPU =====================================================================
/// Buscar Marco Pedido ///
void enviar_respuesta_pedido_marco(int socket_cpu, uint32_t num_pagina, int pid) {
    int marco;
    
    //log_info(memoria_logger, "Se buscara marco en las tablas de paginas");
    marco = buscar_marco(pid, num_pagina);
    t_paquete* paquete = crear_paquete(NUMERO_MARCO);
    // El -1 lo vemos desde la cpu (Page Fault)
    agregar_entero_a_paquete(paquete, marco);
    enviar_paquete(paquete, socket_cpu);   
    eliminar_paquete(paquete);
}

/// @brief Lectura y escritura del espacio de usuario ///
void escribir(int* valor, uint32_t direccion_fisica, uint32_t direccion_logica, int pid, int socket_cpu){

    int id_temp = 0;
    int numero_de_pagina_temp = 0;
    int marco_temp = 0;
    int posicion_swap_temp = 0;
    bool ocupado_temp = false;
    int tiempo_temp = 0;

	usleep(1000 * config_valores_memoria.retardo_respuesta); 

	char* puntero_a_direccion_fisica = espacio_usuario + direccion_fisica; 

	memcpy(puntero_a_direccion_fisica, valor, sizeof(uint32_t));

    int nro_pagina =  floor(direccion_logica / config_valores_memoria.tam_pagina);

    t_pagina* pagina = buscar_pagina(pid, nro_pagina);

    //guardo los datos de la pagina en variables temporales
    id_temp = pagina->id;
    numero_de_pagina_temp = pagina->numero_de_pagina;
    marco_temp = pagina->marco;
    posicion_swap_temp = pagina->posicion_swap;
    ocupado_temp = pagina->ocupado;
    tiempo_temp = pagina->tiempo_de_carga;    

    t_proceso_en_memoria* proceso = buscar_proceso_en_memoria(pid);

    //la borro de la lista porque la voy a modificar
    list_remove_element(proceso->paginas_en_memoria, (void*)pagina);
    
    //creo una nueva pagina para modificar
    t_pagina* pagina_modificada = malloc(sizeof(t_pagina));

    /*si el valor al que apunta es -1, la pagina no esta en memoria. Necesito pf
    if(*valor != 0) 
    {
        //si no es -1, modifico la pagina. SI es -1 la saco de la lista para que tire pf    
    }else{
        //si es -1, no cargo la pagina en la lista de paginas del proceso y desocupo el marco, porque no esta presente
        desocupar_marco(marco_temp);
    }*/

    pagina_modificada->bit_modificado = 1;
    pagina_modificada->tiempo_uso = obtener_tiempo(); 

    pagina_modificada->bit_de_presencia = 1;
    pagina_modificada->id = id_temp;
    pagina_modificada->numero_de_pagina = numero_de_pagina_temp;
    pagina_modificada->marco = marco_temp;
    pagina_modificada->posicion_swap = posicion_swap_temp;
    pagina_modificada->ocupado = ocupado_temp;
    pagina_modificada->tiempo_de_carga = tiempo_temp; 

    //agrego la pagina modificada
    list_add(proceso->paginas_en_memoria, (void*)pagina_modificada);   

    int se_ha_escrito = 1;
    send(socket_cpu, &se_ha_escrito, sizeof(int), 0);      
    
}

uint32_t leer(uint32_t direccion_fisica, uint32_t direccion_logica, int pid) {

	usleep(1000 * config_valores_memoria.retardo_respuesta); 

	char* puntero_direccion_fisica = espacio_usuario + direccion_fisica; 

    uint32_t valor;
	memcpy(&valor, puntero_direccion_fisica, sizeof(uint32_t));
    
    int nro_pagina =  floor(direccion_logica / config_valores_memoria.tam_pagina);
    
    t_proceso_en_memoria* proceso = buscar_proceso_en_memoria(pid);

    t_pagina* pagina = buscar_pagina(pid,nro_pagina);
    
    //list_remove_element(proceso->paginas_en_memoria, (void*)pagina);

    pagina->tiempo_uso = obtener_tiempo();
    
    //list_add(proceso->paginas_en_memoria, (void*)pagina);

	return valor; 
}

void enviar_valor_de_lectura(uint32_t valor, int socket_cpu) {
    t_paquete* paquete = crear_paquete(VALOR_READ);
    agregar_entero_sin_signo_a_paquete(paquete, valor);
    enviar_paquete(paquete, socket_cpu);   
    eliminar_paquete(paquete);
}


