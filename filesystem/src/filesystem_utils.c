#include "filesystem.h"

fcb config_valores_fcb;
//..................................CONFIGURACIONES.....................................................................

void cargar_configuracion(char* path) {

       config = config_create(path); //Leo el archivo de configuracion

      if (config == NULL) {
          perror("Archivo de configuracion de filesystem no encontrado \n");
          abort();
      }

      config_valores_filesystem.ip_filesystem = config_get_string_value(config, "IP_FILESYSTEM");
      config_valores_filesystem.puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");
      config_valores_filesystem.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
      config_valores_filesystem.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
      config_valores_filesystem.puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
      config_valores_filesystem.path_fat = config_get_string_value(config, "PATH_FAT");
      config_valores_filesystem.path_bloques = config_get_string_value(config, "PATH_BLOQUES");
      config_valores_filesystem.path_fcb = config_get_string_value(config, "PATH_FCB");
      config_valores_filesystem.cant_bloques_total = config_get_int_value(config, "CANT_BLOQUES_TOTAL");
      config_valores_filesystem.cant_bloques_swap = config_get_int_value(config, "CANT_BLOQUES_SWAP");
      config_valores_filesystem.tam_bloque = config_get_int_value(config, "TAM_BLOQUE");
      config_valores_filesystem.retardo_acceso_bloque = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");
      config_valores_filesystem.retardo_acceso_fat = config_get_int_value(config, "RETARDO_ACCESO_FAT");
}

// ATENDER CLIENTES //

void atender_clientes_filesystem(void* conexion) {
    int cliente_fd = *(int*)conexion;

    int cod_op = recibir_operacion(cliente_fd);
    log_info(filesystem_logger,"codigo op: %d", cod_op);
    switch (cod_op)
        {
        case -1:
            log_error(filesystem_logger, "Fallo la comunicacion. Abortando \n");
        default:
            log_warning(filesystem_logger, "Operacion desconocida \n");
        break;
        }
}


void levantar_fcb() {

    config = config_create(config_valores_filesystem.path_fcb); //Leo el archivo de configuracion

      if (config == NULL) {
          perror("Archivo de configuracion de fcb no encontrado \n");
          abort();
      }

    config_valores_fcb.nombre_archivo = config_get_string_value(config, "NOMBRE_ARCHIVO");
    config_valores_fcb.tamanio_archivo = config_get_int_value(config, "TAMANIO_ARCHIVO");
    config_valores_fcb.bloque_inicial = config_get_int_value(config, "BLOQUE_INCIAL");
}

void levantar_fat(size_t tamanio_fat) {
    char* path = config_valores_filesystem.path_fat;
    
    FILE* archivo_fat = fopen(path, "rb+");
    if (archivo_fat == NULL) {
        // Si no se encuentra el fat se inicializa con 0s.
        archivo_fat = fopen(path, "wb+");
        if (archivo_fat == NULL) {
            perror("No se puedo abrir o crear el archivo_fat \n");
            abort();
        }

        uint32_t valor_inicial = 0; //Revisar que no sea while(!UINT32_MAX)
        for (size_t i = 0; i < tamanio_fat / sizeof(uint32_t); i++) {
            fwrite(&valor_inicial, sizeof(uint32_t), 1, archivo_fat);
        }
        fflush(archivo_fat);
    }
/*
    // Escritura 
    uint32_t nuevo_valor = UINT32_MAX; 
    fseek(archivo_fat, block_number * sizeof(uint32_t), SEEK_SET);
    fwrite(&nuevo_valor, sizeof(uint32_t), 1, archivo_fat);
    fflush(archivo_fat);

    // Lectura
    uint32_t valor;
    fseek(archivo_fat, block_number * sizeof(uint32_t), SEEK_SET);
    fread(&valor, sizeof(uint32_t), 1, archivo_fat);
*/
    fclose(archivo_fat);
}

void levantar_archivo_bloque(size_t tamanio_swap, size_t tamanio_fat) {
    char* path = config_valores_filesystem.path_bloques;

    FILE* archivo_bloque = fopen(path, "rb"); // Leemos en modo binario, puede ser wb+
    if (archivo_bloque == NULL) {
    perror("No se pudo abrir el archivo de bloques \n");
    abort();
    }

    // Le asignamos un espacio de memoria al swap
    uint32_t* particion_swap = (uint32_t*)malloc(tamanio_swap);
    if (particion_swap == NULL) {
    perror("No se pudo alocar memoria para el swap \n");
    abort();
    }

    fread(particion_swap, 1, tamanio_swap, archivo_bloque);

    // Le asignamos un espacio de memoria al fat
    uint32_t* particion_fat = (uint32_t*)malloc(tamanio_fat);
    if (particion_fat == NULL) {
    perror("No se pudo alocar memoria para el fat \n");
    abort();
    }

    fread(particion_fat, 1, tamanio_fat, archivo_bloque);

    fclose(archivo_bloque);

}          

