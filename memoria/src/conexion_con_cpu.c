#include "memoria.h"
int tiempo;


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
	char** lista_instrucciones = string_split(instrucciones, "\n");

	//a la cpu le mandamos SOLO la instruccion que me marca el prog_count
    char *instruccion = lista_instrucciones[inst_a_ejecutar];

    t_paquete* paquete = crear_paquete(INSTRUCCIONES); 

    agregar_cadena_a_paquete(paquete, instruccion); 

    enviar_paquete(paquete, socket_cpu);
	log_info(memoria_logger,"Instrucciones enviadas :)\n");

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

    log_warning(memoria_logger, "No se encontró un proceso con PID %d", pid);
    return NULL;
}

//============================================ Instrucciones de CPU =====================================================================
/// Buscar Marco Pedido ///
void enviar_respuesta_pedido_marco(int socket_cpu, uint32_t num_pagina, int pid) {
    int marco;

    marco = buscar_marco(pid, num_pagina);

    t_paquete* paquete = crear_paquete(NUMERO_MARCO);
    // El -1 lo vemos desde la cpu (Page Fault)
    agregar_entero_a_paquete(paquete, marco);
    enviar_paquete(paquete, socket_cpu);   
    eliminar_paquete(paquete);
}

/// @brief Lectura y escritura del espacio de usuario ///
void escribir(uint32_t* valor, uint32_t direccion_fisica, int socket_cpu){

	usleep(1000 * config_valores_memoria.retardo_respuesta); 

	char* puntero_a_direccion_fisica = espacio_usuario + direccion_fisica; 

	memcpy(puntero_a_direccion_fisica, valor, sizeof(uint32_t));

    int se_ha_escrito = 1;
    send(socket_cpu, &se_ha_escrito, sizeof(int), 0); 
}

uint32_t leer(uint32_t direccion_fisica) {

	usleep(1000 * config_valores_memoria.retardo_respuesta); 

	char* puntero_direccion_fisica = espacio_usuario + direccion_fisica; 

    uint32_t valor;
	memcpy(&valor, puntero_direccion_fisica, sizeof(uint32_t));

	return valor; 

}

void enviar_valor_de_lectura(uint32_t valor, int socket_cpu) {
    t_paquete* paquete = crear_paquete(VALOR_READ);
    agregar_entero_a_paquete(paquete, valor);
    enviar_paquete(paquete, socket_cpu);   
    eliminar_paquete(paquete);
}


