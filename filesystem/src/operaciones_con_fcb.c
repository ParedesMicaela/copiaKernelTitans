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

void cargamos_cambios_a_fcb_ampliar(int tamanio_nuevo, uint32_t bloque_inicial, char* nombre_archivo) {

	char* puntero_auxiliar = NULL;

	char* path = string_from_format ("%s/%s.fcb", config_valores_filesystem.path_fcb, nombre_archivo);
    t_config * archivo = config_create (path);

    puntero_auxiliar = string_from_format("%d", bloque_inicial);
	config_set_value(archivo, "BLOQUE_INICIAL", puntero_auxiliar);
	free(puntero_auxiliar);
	
	puntero_auxiliar = string_from_format("%d", tamanio_nuevo);
    config_set_value(archivo, "TAMANIO_ARCHIVO", puntero_auxiliar);
	free(puntero_auxiliar);

    config_save_in_file(archivo, path);

    free(path);
    config_destroy(archivo);
}

void cargamos_cambios_a_fcb_reducir(int tamanio_nuevo, char* nombre_archivo) {

	char* puntero_auxiliar = NULL;

	char* path = string_from_format ("%s/%s.fcb", config_valores_filesystem.path_fcb, nombre_archivo);
    t_config * archivo = config_create (path);
	
	puntero_auxiliar = string_from_format("%d", tamanio_nuevo);
    config_set_value(archivo, "TAMANIO_ARCHIVO", puntero_auxiliar);
	free(puntero_auxiliar);

    config_save_in_file(archivo, path);

    free(path);
    config_destroy(archivo);
}