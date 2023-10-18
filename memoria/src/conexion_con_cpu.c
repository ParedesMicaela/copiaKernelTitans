#include "memoria.h"
int tiempo;


//================================================= Handshake =====================================================================
void enviar_paquete_handshake(int socket_cliente) {

	t_paquete* handshake=crear_paquete(HANDSHAKE);
	agregar_entero_a_paquete(handshake,config_get_int_value(config, "TAM_PAGINA"));

	enviar_paquete(handshake,socket_cliente);
	log_info(memoria_logger,"Handshake enviado :)\n");
	log_info(memoria_logger,"Se envio el tamaño de pagina %d bytes al CPU \n",config_get_int_value(config, "TAM_PAGINA"));

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
/*
char* leer_archivo_instrucciones(char* path_instrucciones) {

    FILE* instr_f = fopen(path_instrucciones, "r");
    char* una_cadena    = malloc(MAX_CHAR);
    char* cadena_completa  = NULL; //malloc(MAX_CHAR);

    if (instr_f == NULL) {
        perror("no se pudo abrir el archivo de instrucciones");
        exit(-1);
    }

    while (!feof(instr_f)) {
        fgets(una_cadena, MAX_CHAR, instr_f);
        string_append(&cadena_completa, una_cadena);
    }

    free(una_cadena);
    fclose(instr_f);

    return cadena_completa;
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

char* buscar_path_proceso(int pid)
{
    if (lista_procesos == NULL) {
        log_error(memoria_logger, "Error: lista_procesos esta vacia");
        return NULL;
    }

    for (int i = 0; i < list_size(lista_procesos); i++)
    {
        t_proceso_en_memoria* proceso = list_get(lista_procesos, i);

        if (proceso != NULL && proceso->pid == pid)
        {
            return strdup(proceso->path_proceso);
        }
    }

    log_warning(memoria_logger, "No se encontró un proceso con PID %d", pid);
    return NULL;
}

//============================================ Instrucciones de CPU =====================================================================

/// @brief Lectura y escritura del espacio de usuario ///
char* leer(int32_t direccion_fisica,int tamanio) {

    int tiempo = config_valores_memoria.retardo_respuesta;

	usleep(tiempo*1000); 

	char* puntero_direccion_fisica = espacio_usuario + direccion_fisica; 

	char* valor = malloc(sizeof(char)*tamanio); 
	
	memcpy(valor, puntero_direccion_fisica, tamanio);

	return valor; 

}

void escribir(char* valor, int32_t direccion_fisica, int tamanio){

    int tiempo = config_valores_memoria.retardo_respuesta;

	usleep(tiempo*1000); 

	char* puntero_a_direccion_fisica = espacio_usuario + direccion_fisica; 

	memcpy(puntero_a_direccion_fisica, valor, tamanio);

	free(valor); 
}
