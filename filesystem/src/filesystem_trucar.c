
#include "filesystem.h"

void truncar_archivo(char *archivo, int tamanioTruncar) {
    
    char* path_fcb = ();
    int  tamanioBloque = tamanio_bloque();
    char* path_Archivo = string_from_format("%s/%s",path_fcb,archivo);
    char *string; //revisar
    
    t_config *fcb_archivo = iniciar_config(path_Archivo);// Abro el fcb del archivo
    free(path_Archivo);

    // Leo 
    int tamanio_Archivo = config_get_int_value(fcb_archivo,"TAMANIO_ARCHIVO");
    int bloque_inicial = config_get_int_value(fcb_archivo,"BLOQUE_INICIAL");

    if (tamanio_Archivo > tamanioTruncar){

        int bloquesASacar = ceil((tamanio_Archivo - tamanioTruncar) / (tamanioBloque));

        sacarBloques(bloquesASacar,bloque_inicial,archivo,tamanio_Archivo);

        string = string_from_format("%d", tamanioTruncar);
        config_set_value(fcb_archivo, "TAMANIO_ARCHIVO", string);
        free(string);
    }
    else{
        
        int bloquesAAgregar = ceil((tamanioTruncar - tamanio_Archivo) / (tamanioBloque));

        agregarBloques(bloquesAAgregar,&bloque_inicial,archivo);

        string = string_from_format("%d", tamanioTruncar);
        config_set_value(fcb_archivo, "TAMANIO_ARCHIVO", string);
        free(string);
        string = string_from_format("%d", bloque_inicial);
        config_set_value(fcb_archivo, "BLOQUE_INICIAL", string);
        free(string);

    }
    config_save(fcb_archivo);
    config_destroy(fcb_archivo);
    return;
}

void truncar_archivo(char *nombre, int tamanio_nuevo, int socket_kernel) {

    fcb* fcb_a_truncar = levantar_fcb(nombre);
	char* nombre_archivo = fcb_a_truncar->nombre_archivo;
	int tamanio_actual_archivo = fcb_a_truncar->tamanio_archivo;
	uint32_t bloque_inicial = fcb_a_truncar->bloque_inicial;
	free(fcb_a_truncar);
	free(nombre);

    log_info(filesystem_logger,"Truncando archivo: Nombre %s, tamanio %d, Bloque inicial %d \n",nombre_archivo, tamanio_actual_archivo, bloque_inicial);

    if(tamanio_actual_archivo < tamanio_nuevo) {

        ampliar_tamanio_archivo(tamanio_nuevo, tamanio_actual_archivo, bloque_inicial);

    } else if(tamanio_actual_archivo > tamanio_nuevo) {
        
        reducir_tamanio_archivo(tamanio_nuevo, tamanio_actual_archivo, bloque_inicial);

    } else{
		printf("Bobi no truncaste\n");
	}

    cargamos_cambios_a_fcb(tamanio_nuevo, nombre_archivo);

	free(nombre_archivo);

	//Confirmamos truncado
	int truncado = 1;
    send(socket_kernel, &truncado, sizeof(int), 0);

}

void ampliar_tamanio_archivo(int tamanio_nuevo, int tamanio_actual_archivo, uint32_t bloque_inicial) {
    
    uint32_t bloques_a_agregar = ceil((tamanio_nuevo - tamanio_actual_archivo) / tam_bloque);

    const uint32_t eof = UINT32_MAX;
    uint32_t puntero_bloque = *puntero_bloque_inicial;
    uint32_t puntero_bloque_libre = 1;
    uint32_t proximo_bloque_a_agregar;

    // Busco el puntero al último bloque del archivo
    if (puntero_bloque != 0) {

        while (1) {
            memcpy(&proximo_bloque_a_agregar, tabla_fat_en_memoria + (puntero_bloque * sizeof(uint32_t)), sizeof(uint32_t));

            usleep(1000 * config_valores_filesystem.retardo_acceso_fat);

            log_info(filesystem_logger, "Acceso FAT - Entrada: %d - Valor: %d", puntero_bloque, proximo_bloque_a_agregar);

            // Si es el final del archivo
            if (proximo_bloque_a_agregar == eof) {
                break;
            }

            // Actualizar puntero_aux para seguir buscando
            puntero_bloque = proximo_bloque_a_agregar;
        }
    }

    // Asignar bloques al archivo
    while (bloques_a_agregar > 0) {

        // Buscamos el primer bloque
        while (1) {
            
            // Leemos el siguiente bloque en la secuencia de bloques libres
            memcpy(&proximo_bloque_a_agregar, tabla_fat_en_memoria + (puntero_bloque_libre * sizeof(uint32_t)), sizeof(uint32_t));
            log_info(filesystem_logger, "Acceso FAT - Entrada: %d - Valor: %d", puntero_bloque_libre, proximo_bloque_a_agregar);
            
            usleep(1000 * config_valores_filesystem.retardo_acceso_fat);

            // Salir si se encuentra un bloque libre
            if (proximo_bloque_a_agregar == 0) {
                break;
            }

            // Actualizar puntero_bloque_libre para seguir buscando
            puntero_bloque_libre++;
        }

        // Actualizamos la FAT y asignamos el bloque al archivo
        if (*puntero_bloque_inicial == 0) {

            // Primer bloque del archivo
            *puntero_bloque_inicial = puntero_bloque_libre;
            memcpy(bufferFAT + (*puntero_bloque_inicial * sizeof(uint32_t)), &eof, sizeof(uint32_t));
        } else {
            // Bloque subsiguiente en la secuencia del archivo
            memcpy(bufferFAT + (puntero_bloque * sizeof(uint32_t)), &puntero_bloque_libre, sizeof(uint32_t));
            memcpy(bufferFAT + (puntero_bloque_libre * sizeof(uint32_t)), &eof, sizeof(uint32_t));
            puntero_bloque = puntero_bloque_libre;
        }

        bloques_a_agregar--;
    }

    // Sincronizar la FAT con la memoria persistente
    msync(bufferFAT, tamanio_fat, MS_INVALIDATE);

}

// Función auxiliar para actualizar la FAT
void actualizarFAT(uint32_t bloque, uint32_t nuevo_valor, uint32_t retardo_bloque) {
    memcpy(bufferFAT + bloque * sizeof(uint32_t), &nuevo_valor, sizeof(uint32_t));
    usleep(1000 * config_valores_filesystem.retardo_acceso_fat);
    log_info(filesystem_logger, "Acceso FAT - Entrada: %d - Valor: %d", bloque, nuevo_valor);

}

void reducir_tamanio_archivo(tamanio_nuevo, tamanio_actual_archivo, bloque_inicial) {
    
    int bloques_a_sacar = ceil((tamanio_actual_archivo - tamanio_nuevo) / (tam_bloque));
    
    uint32_t cantidad_a_reducir = cantidad_bloques * sizeof(int);

    void *punteros_archivo = malloc(cantidad_a_reducir);
    get_array_punteros(punteros_archivo, bloque_inicial, tamanio_Archivo);
    int j = cantidad_bloques - 1;
    int eof = UINT32_MAX;
    int bloque_libre = 0;
    int bloques_Aux = bloquesASacar;
    int retardo_fat = retardo_acceso_FAT();
    int retardo_bloque = retardo_acceso_bloques();

    while (bloques_Aux > 0) {
        int desplazamiento;
        memcpy(&desplazamiento, punteros_archivo + (j * sizeof(int)), sizeof(int));

        // Marcar el bloque como libre en la FAT
        actualizarFAT(desplazamiento, bloque_libre, retardo_fat, retardo_bloque);

        j--;
        bloques_Aux--;

        // Si es el último bloque a eliminar, actualizar la FAT para marcar el final del archivo
        if (bloques_Aux == 0) {
            actualizarFAT(desplazamiento, endOfFile, retardo_fat, retardo_bloque);
            break;
        }
    }

    int tam_FAT = cant_bloques_total();
    msync(bufferFAT, tam_FAT * sizeof(int), MS_INVALIDATE);

    free(punteros_archivo);
    return;
}

    usleep(retardo_bloque * 1000);


