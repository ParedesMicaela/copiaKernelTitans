#include "filesystem.h"

fcb* levantar_fcb (char * nombre) {

    char * path = string_from_format ("%s/%s.fcb", config_valores_filesystem.path_fcb, nombre);

    t_config * archivo = config_create (path);

    fcb* archivo_FCB = malloc (sizeof (fcb)); 
    archivo_FCB->nombre_archivo = string_duplicate(config_get_string_value (archivo, "NOMBRE_ARCHIVO"));
    archivo_FCB->bloque_inicial = config_get_int_value(archivo, "BLOQUE_INICIAL");
    archivo_FCB->tamanio_archivo = config_get_int_value (archivo, "TAMANIO_ARCHIVO");

	config_destroy (archivo);
    free(path);
    return archivo_FCB;
}

void actualizar_fcb(fcb* nuevo_fcb)
{
	char * path = string_from_format ("%s/%s.fcb", config_valores_filesystem.path_fcb, nuevo_fcb->nombre_archivo);

    t_config * archivo = config_create (path);
	
	int tamaño_buffer_bloque = snprintf(NULL, 0, "%d", nuevo_fcb->bloque_inicial) + 1;
    char *bloque = (char *)malloc(tamaño_buffer_bloque * sizeof(char));
    if (bloque == NULL)perror("Error: no se pudo asignar memoria para el bloque.\n");
    sprintf(bloque, "%d", nuevo_fcb->bloque_inicial);
	log_info(filesystem_logger,"El bloque inical seria %s\n", bloque);

	int tamaño_buffer_tamanio = snprintf(NULL, 0, "%d", nuevo_fcb->tamanio_archivo) + 1;
    char *tamanio = (char *)malloc(tamaño_buffer_tamanio * sizeof(char));
    if (tamanio == NULL)perror("Error: no se pudo asignar memoria para el bloque.\n");
    sprintf(tamanio, "%d", nuevo_fcb->tamanio_archivo);
	log_info(filesystem_logger,"El tamanio seria %s\n", tamanio);
	
	config_set_value(archivo, "NOMBRE_ARCHIVO", nuevo_fcb->nombre_archivo);
	config_set_value(archivo, "BLOQUE_INICIAL", bloque);
	config_set_value(archivo, "TAMANIO_ARCHIVO", tamanio);
	
	free(bloque);
	free(tamanio);

	config_remove_key(archivo,"CANT_BLOQUES_SWAP");
	config_remove_key(archivo,"RETARDO_ACCESO_BLOQUE");
	config_remove_key(archivo,"CANT_BLOQUES_TOTAL");
	config_remove_key(archivo,"PATH_BLOQUES");
	config_remove_key(archivo,"PATH_FAT");
	config_remove_key(archivo,"RETARDO_ACCESO_FAT");
	config_remove_key(archivo,"TAM_BLOQUE");
	config_remove_key(archivo,"PUERTO_ESCUCHA");
	config_remove_key(archivo,"PATH_FCB");
	config_remove_key(archivo,"IP_MEMORIA");
	config_remove_key(archivo,"PUERTO_MEMORIA");
	
	config_save_in_file(archivo,path);

	free(path);
	config_destroy(archivo);
	log_info(filesystem_logger,"Actualizarmos bien el fcb\n");
}

void cargamos_cambios_a_fcb(int tamanio_nuevo, char* nombre) {

	fcb *nuevo_fcb = malloc(sizeof(fcb));
	nuevo_fcb->nombre_archivo = nombre;
	nuevo_fcb->tamanio_archivo = tamanio_nuevo;
	nuevo_fcb->bloque_inicial = calcular_bloque_inicial_archivo(tamanio_nuevo); 
	actualizar_fcb(nuevo_fcb);
	free(nuevo_fcb);
	log_info(filesystem_logger,"Terminamos y mandamos el fcb a actualizarse\n");
}

int proximo_bloque_inicial = 1;

int calcular_bloque_inicial_archivo(int tamanio)
{
	int devolver = proximo_bloque_inicial;
	proximo_bloque_inicial += (tamanio /tam_bloque);
	return devolver;
}