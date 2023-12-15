#include "memoria.h"

void inicializar_swap_proceso(int pid, int cantidad_paginas_proceso) {
    t_paquete* paquete = crear_paquete(INICIALIZAR_SWAP); 
    agregar_entero_a_paquete(paquete, pid); 
    agregar_entero_a_paquete(paquete, cantidad_paginas_proceso);
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);
}

void enviar_pedido_pagina_para_escritura(int pid, int pag_pf){

    t_paquete* paquete = crear_paquete(PAGINA_SWAP_OUT);
    agregar_entero_a_paquete(paquete,pid);
    agregar_entero_a_paquete(paquete,pag_pf);
    
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);
}


void escribir_en_swap(t_pagina* pagina_a_escribir, int pid){
    
    pagina_a_escribir->bit_de_presencia = 0;
    pagina_a_escribir->bit_modificado = 0;

    desocupar_marco(pagina_a_escribir->marco);

    log_info(memoria_logger, "SWAP OUT -  PID: %d - Marco: %d - Page In: %d - %d\n",pid,pagina_a_escribir->marco, pid, pagina_a_escribir->numero_de_pagina);

    t_paquete* paquete = crear_paquete(PAGINA_SWAP_IN);
    agregar_entero_a_paquete(paquete, pid);
    agregar_entero_a_paquete(paquete, pagina_a_escribir->numero_de_pagina);
    agregar_entero_a_paquete(paquete, pagina_a_escribir->posicion_swap);
    //agregar_entero_a_paquete(paquete, pagina_a_escribir->marco);
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);
}


void escribir_en_memoria(void* contenido, size_t tamanio_contenido, uint32_t direccion_fisica)
{
	usleep(1000 * config_valores_memoria.retardo_respuesta); 

    char* puntero_a_direccion_fisica = espacio_usuario + direccion_fisica; 

    memcpy(puntero_a_direccion_fisica, contenido, tamanio_contenido);

    log_info(memoria_logger, "Acción: %s - Dirección física: %d ", "ESCRIBIR EN MEMORIA", direccion_fisica);

    enviar_paquete_confirmacion_escritura();
 }

void* leer_en_memoria(size_t tamanio_contenido, uint32_t direccion_fisica) {

	usleep(1000 * config_valores_memoria.retardo_respuesta); 

	char* puntero_direccion_fisica = espacio_usuario + direccion_fisica; 
    void* contenido = malloc(tamanio_contenido);

	memcpy(contenido, puntero_direccion_fisica, tamanio_contenido);

    //log_info(memoria_logger, "Acción: %s - Dirección física: %d ", "LEER EN MEMORIA", direccion_fisica);

    log_info(memoria_logger, "Acción: %s - Dirección física: %d - Tamaño: %zu", "LEER EN MEMORIA", direccion_fisica, tamanio_contenido);

	mem_hexdump(contenido, sizeof(contenido));

	return contenido; 
}


void bloques_para_escribir(int tam_bloque, void* contenido, uint32_t puntero_archivo, char* nombre_archivo){

    t_paquete* paquete = crear_paquete(ESCRIBIR_EN_ARCHIVO_BLOQUES);
    agregar_bytes_a_paquete(paquete, contenido, tam_bloque);
	agregar_entero_sin_signo_a_paquete(paquete, puntero_archivo);
	agregar_cadena_a_paquete(paquete, nombre_archivo);
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);
}

void enviar_paquete_confirmacion_escritura() {
    t_paquete* paquete = crear_paquete(ESCRITURA_EN_MEMORIA_CONFIRMADA);
    agregar_entero_a_paquete(paquete, 1);
    enviar_paquete(paquete, socket_fs);
}