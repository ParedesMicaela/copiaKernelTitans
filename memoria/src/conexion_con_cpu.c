#include "memoria.h"

//================================================= Handshake =====================================================================
void enviar_paquete_handshake(int socket_cliente) {

	t_paquete* handshake=crear_paquete(HANDSHAKE);
	agregar_entero_a_paquete(handshake,config_valores_memoria.tam_pagina);

	enviar_paquete(handshake,socket_cliente);
	log_info(memoria_logger,"Handshake enviado :)\n");
	log_info(memoria_logger,"Se envio el tamaño de pagina %d bytes al CPU \n",config_valores_memoria.tam_pagina);

	eliminar_paquete(handshake);
}

//============================================ Instrucciones a CPU =====================================================================
void enviar_paquete_instrucciones(int socket_cpu, char* instrucciones, int inst_a_ejecutar)
{
	//armamos una lista de instrucciones con la cadena de instrucciones que lei, pero ahora las separo
    pthread_mutex_lock(&mutex_lista_instrucciones);
	char** lista_instrucciones = string_split(instrucciones, "\n");
    pthread_mutex_unlock(&mutex_lista_instrucciones);

	//a la cpu le mandamos SOLO la instruccion que me marca el prog_count
    pthread_mutex_lock(&mutex_instrucciones);
    char *instruccion = lista_instrucciones[inst_a_ejecutar];
    pthread_mutex_unlock(&mutex_instrucciones);

    t_paquete* paquete = crear_paquete(INSTRUCCIONES); 

    agregar_cadena_a_paquete(paquete, instruccion); 

    enviar_paquete(paquete, socket_cpu);

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
void escribir(uint32_t* valor, uint32_t direccion_fisica, uint32_t direccion_logica, int pid, int socket_cpu){

	usleep(1000 * config_valores_memoria.retardo_respuesta); 

	char* puntero_a_direccion_fisica = espacio_usuario + direccion_fisica; 

	memcpy(puntero_a_direccion_fisica, valor, sizeof(uint32_t));

    int nro_pagina =  floor(direccion_logica / config_valores_memoria.tam_pagina);

    t_pagina* pagina = buscar_pagina(pid, nro_pagina);

    t_proceso_en_memoria* proceso = buscar_proceso_en_memoria(pid);

    if(valor == -1)
    {
        //si me dan para escribir -1 es que la pagina no esta en memoria
        pagina->bit_de_presencia = 0;
    }else{
        pagina->bit_modificado = 1;
        pagina->tiempo_uso = obtener_tiempo(); 
    }

    printf("\n cantidad de paginas que tioene el proceso %d  en mem: %d \n",pid ,list_size(proceso->paginas_en_memoria));

    int se_ha_escrito = 1;
    send(socket_cpu, &se_ha_escrito, sizeof(int), 0); 
}

uint32_t leer(uint32_t direccion_fisica, uint32_t direccion_logica, int pid) {

	usleep(1000 * config_valores_memoria.retardo_respuesta); 

	char* puntero_direccion_fisica = espacio_usuario + direccion_fisica; 

    uint32_t valor;
	memcpy(&valor, puntero_direccion_fisica, sizeof(uint32_t));
    
    int nro_pagina =  floor(direccion_logica / config_valores_memoria.tam_pagina);

    t_pagina* pagina = buscar_pagina(pid,nro_pagina);

    pagina->tiempo_uso = obtener_tiempo(); 

	return valor; 
}

void enviar_valor_de_lectura(uint32_t valor, int socket_cpu) {
    t_paquete* paquete = crear_paquete(VALOR_READ);
    agregar_entero_a_paquete(paquete, valor);
    enviar_paquete(paquete, socket_cpu);   
    eliminar_paquete(paquete);
}


