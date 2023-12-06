#include "filesystem.h"

void inicializar_bitarray() {
    // Calcular el tamaño necesario para el bitarray (en bits)
    int tamanio_bitarray = tamanio_fat + (tamanio_swap * sizeof(bloque_swap) * 8);

    // Calcular el tamaño necesario para el char* data
    int tamanio_data = tamanio_swap * sizeof(bloque_swap);

    // Crear un buffer para el char* data
    char* buffer_data = malloc(tamanio_data);
    memset(buffer_data, 0, tamanio_data); // Inicializar con ceros

    // Crear el bitarray con el buffer y el tamaño
    bitmap_archivo_bloques = bitarray_create_with_mode(buffer_data, tamanio_bitarray, LSB_FIRST);

    // Liberar el buffer, ya que el bitarray lo manejará internamente
    free(buffer_data);
}
