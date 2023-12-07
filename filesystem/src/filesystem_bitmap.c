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

void ocupar_bloque(int i) {
    bitarray_set_bit(bitmap_archivo_bloques, i);
}



/*
void levantar_archivo_bloque() {
    char* path_bloques = config_valores_filesystem.path_bloques;

    int espacio_de_swap = tamanio_swap - 1;
    int espacio_de_FAT = tamanio_archivo_bloques - espacio_de_swap;


    // Abro el archivo (o lo creo)
    int fd_bloques = open(path_bloques, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd_bloques == -1) {
        perror("Error al abrir el archivo de bloques");
        abort();
    }

    // Mapea la partición de SWAP
    char* swap_mapeado = mmap(0, espacio_de_swap, PROT_WRITE, MAP_SHARED, fd_bloques, 0);
    if (swap_mapeado == MAP_FAILED) {
        perror("Error al mapear la partición de SWAP");
        close(fd_bloques);
        abort();
    }

    for(int i = 0; i < espacio_de_swap; i++) {
         swap_mapeado[i] = '0';
    }
    // Mapea la parte del sistema FAT después de la partición de SWAP
    uint32_t* fat_mapeado = mmap(0, espacio_de_FAT, PROT_WRITE, MAP_SHARED, fd_bloques, espacio_de_swap);
    if (fat_mapeado == MAP_FAILED) {
        perror("Error al mapear la parte del sistema FAT");
        munmap(swap_mapeado, espacio_de_swap);
        close(fd_bloques);
        abort();
    }

    // Inicializo la memoria mapeada de la partición de FAT 
    for (int i = 0; i < espacio_de_FAT; i++) {
        fat_mapeado[i] = 0;
    }

    // Desmapea las particiones
    if (munmap(swap_mapeado, espacio_de_swap) == -1) {
        perror("Error al desmapear la partición de SWAP");
        close(fd_bloques);
        exit(EXIT_FAILURE);
    }

    if (munmap(fat_mapeado, espacio_de_FAT) == -1) {
        perror("Error al desmapear la parte del sistema FAT");
        close(fd_bloques);
        exit(EXIT_FAILURE);
    }

    close(fd_bloques);
}
*/
