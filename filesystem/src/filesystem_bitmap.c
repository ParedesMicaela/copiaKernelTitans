#include "filesystem.h"

void inicializar_bitarray()
{
    // Calcular el tamaño necesario para el bitarray (en bits)
    int tamanio_bitarray = tamanio_fat + (tamanio_swap * sizeof(bloque_swap) * 8);

    // Calcular el tamaño necesario para el char* data
    int tamanio_data = (tamanio_bitarray + 7) / 8; // Ajustar al número entero superior

    // Crear un buffer para el char* data
    char *buffer_data = malloc(tamanio_data);
    memset(buffer_data, 0, tamanio_data); // Inicializar con ceros

    // Crear el bitarray con el buffer y el tamaño
    bitmap_archivo_bloques = bitarray_create_with_mode(buffer_data, tamanio_bitarray, LSB_FIRST);
}


/*
void leer_FAT() {
    FILE *archivo_fat = fopen(path, "wb+");

    for (size_t i = 0; i < tam_entrada; i++) {
        uint32_t entrada_fat;
        fread(&entrada_fat, sizeof(uint32_t), 1, archivo_fat);
    }

    fclose(archivo_fat);
}
*/