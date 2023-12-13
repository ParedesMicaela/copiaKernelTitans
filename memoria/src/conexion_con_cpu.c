#include "memoria.h"

//static size_t string_array_length(char** array);
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
    //pthread_mutex_lock(&mutex_lista_instrucciones);
    char** lista_instrucciones = string_split(instrucciones, "\n");

    free(instrucciones);

    // Acceder a la instrucción corregida
    char *instruccion = lista_instrucciones[inst_a_ejecutar];

    t_paquete* paquete = crear_paquete(INSTRUCCIONES);
    agregar_cadena_a_paquete(paquete, instruccion);
    enviar_paquete(paquete, socket_cpu);
    free_array(lista_instrucciones);

    //pthread_mutex_unlock(&mutex_lista_instrucciones);

    eliminar_paquete(paquete);
}

/*
static size_t string_array_length(char** array) {
    if (array == NULL) {
        return 0;
    }

    size_t count = 0;
    while (array[count] != NULL) {
        count++;
    }

    return count;
}

 int cantidad_instrucciones = string_array_length(lista_instrucciones);
    if (inst_a_ejecutar > cantidad_instrucciones) {
        inst_a_ejecutar = 0;  
    }
*/

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

char* buscar_path_proceso(int pid) {
    pthread_mutex_lock(&mutex_procesos);

    int cantidad_procesos_en_memoria = list_size(procesos_en_memoria);
    char* path_encontrado = NULL;

    for (int i = 0; i < cantidad_procesos_en_memoria; i++) {
        t_proceso_en_memoria* proceso = list_get(procesos_en_memoria, i);

        if (proceso != NULL && proceso->pid == pid) {
            path_encontrado = strdup(proceso->path_proceso);
            break;  
        }
    }

    pthread_mutex_unlock(&mutex_procesos);

    return path_encontrado;
}

//============================================ Instrucciones de CPU =====================================================================
/// Buscar Marco Pedido ///
void enviar_respuesta_pedido_marco(int socket_cpu, uint32_t num_pagina, int pid) {

    // Si encuentra -1, CPU tira PF
    int marco = buscar_marco(num_pagina, pid);
    t_paquete* paquete = crear_paquete(NUMERO_MARCO);
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


