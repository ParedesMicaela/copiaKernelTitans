#include "memoria.h"

void enviar_pedido_pagina_para_escritura(int pid, int pag_pf){

    t_paquete* paquete = crear_paquete(PEDIR_PAGINA_PARA_ESCRITURA);
    agregar_entero_a_paquete(paquete,pid);
    agregar_entero_a_paquete(paquete,pag_pf);
    
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);
}


void escribir_en_swap(t_pagina* pagina_a_escribir){
    
    //CREO QUE DEBERIA SER UN SEND CON EL CONTENIDO, PERO POR EL MOMENTO LO DEJO ASÍ

    t_paquete* paquete = crear_paquete(ESCRIBIR_PAGINA_SWAP);
    agregar_entero_a_paquete(paquete, pagina_a_escribir->numero_de_pagina);
    agregar_entero_a_paquete(paquete, pagina_a_escribir->posicion_swap);
    agregar_entero_a_paquete(paquete, pagina_a_escribir->marco);
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);
}


void escribir_en_memoria(void* contenido, size_t tamanio_contenido, uint32_t direccion_fisica)
{
	usleep(1000 * config_valores_memoria.retardo_respuesta); 

    char* puntero_a_direccion_fisica = espacio_usuario + direccion_fisica; 

    memcpy(puntero_a_direccion_fisica, contenido, tamanio_contenido);
   
    int se_ha_escrito = 1;
    send(socket_fs, &se_ha_escrito, sizeof(int), 0 );

 }
