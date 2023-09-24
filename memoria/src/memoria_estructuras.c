#include "memoria.h"

//================================================= Variables Globales ================================================
void* espacio_usuario;
t_list* marcos_memoria;
t_bitarray* mapa_bits_principal;
t_bitarray* mapa_bits_swap;
t_list* procesos_en_memoria;
//================================================= Creacion Estructuras ====================================================

/// @brief Espacio Usuario ///
void creacion_espacio_usuario(){
    espacio_usuario = malloc (config_valores_memoria.tam_memoria); 
	if (espacio_usuario == NULL) {
        perror ("No se pudo alocar memoria al espacio de usuario.");
        //que pasa si no se puede, abort?
    }
	liberar_espacio_usuario(); //atexit? 
}

void liberar_espacio_usuario() {
	free (espacio_usuario);
}

/// @brief Memoria Swap ///
void crear_memoria_swap(int tam_swap_pid) {

    int cantidad_marcos = tam_swap_pid / config_valores_memoria.tam_pagina;
    log_info(memoria_logger, "Se crearan %d marcos para swap", cantidad_marcos);   

    int num_bytes = cantidad_marcos / 8;

	void* aux = malloc(num_bytes);
	mapa_bits_swap = bitarray_create_with_mode(aux, num_bytes, MSB_FIRST);

	for (int i=0; i < cantidad_marcos; i++)
	{
		bitarray_clean_bit(mapa_bits_swap, i);
	}
}


void crear_memoria_principal(){
    procesos_en_memoria = list_create();
    int tam_pagina = config_valores_memoria.tam_pagina;
    int tam_memoria = config_valores_memoria.tam_memoria;
    int ints_por_pagina = tam_pagina / 4;
    int cantidad_marcos = tam_memoria / tam_pagina;
    marcos_memoria = list_create();

    log_info(memoria_logger, "Se crearan %d marcos para la memoria principal", cantidad_marcos);   

    //cantidad_marcos += (8 - cantidad_marcos % 8);    
    int num_bytes = cantidad_marcos / 8;

	void* aux = malloc(num_bytes);
	mapa_bits_principal = bitarray_create_with_mode(aux, num_bytes, MSB_FIRST);
    
    //cantidad de marcos de la memoria principal
	for (int i=0; i < cantidad_marcos; i++)
	{
		bitarray_clean_bit(mapa_bits_principal, i);
        t_list* lista_datos = list_create();

        //Se itera los ints por pagina para crear contenido del marco
        for (int j=0; j < ints_por_pagina; j++) {
            uint32_t* dato = malloc(sizeof(uint32_t));
            list_add(lista_datos, dato);
        }
        list_add(marcos_memoria, lista_datos);
	}

    return;
}

void crear_tablas_paginas_proceso(int pid_proceso,int tam_swap_pid){



}
