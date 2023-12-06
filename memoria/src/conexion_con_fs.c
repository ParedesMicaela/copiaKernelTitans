#include "memoria.h"

void enviar_pedido_pagina_para_escritura(int pid, int pag_pf){

    t_paquete* paquete = crear_paquete(PEDIR_PAGINA_PARA_ESCRITURA);
    agregar_entero_a_paquete(paquete,pid);
    agregar_entero_a_paquete(paquete,pag_pf);
    
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);
}

void solucionar_page_fault(int num_pagina) {
    t_paquete* paquete = crear_paquete(SOLUCIONAR_PAGE_FAULT_FILESYSTEM); 
    agregar_entero_a_paquete(paquete, num_pagina); 
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

//bien se viene el toqueteo sensual que nos dijo Dami (Fachini a.k.a el fachas en mis libros)

//en vez de escribir_en_memoria(char* contenido.... why not 
void escribir_en_memoria(void* contenido, size_t tamanio_contenido, uint32_t direccion_fisica)
{
	usleep(1000 * config_valores_memoria.retardo_respuesta); 

    if (espacio_usuario == NULL) {
        printf("Error: espacio_usuario is NULL\n");
        return;
    }else{
        memcpy(direccion_fisica, contenido, tamanio_contenido);

        //le mandamos el fokin send a fs
        int se_ha_escrito = 1;
        send(socket_fs, &se_ha_escrito, sizeof(int), 0 );

        //como en monster inc, que tal si esto lo mandamos a volarrrrrrr
        /*size_t contenido_length = strlen(contenido) + 1;
        memcpy(puntero_a_direccion_fisica, contenido, contenido_length);

        int se_ha_escrito = 1;
        send(socket_filesystem, &se_ha_escrito, sizeof(int), 0); */

        /*char* puntero_a_direccion_fisica = espacio_usuario + direccion_fisica; 

        size_t contenido_length = strlen(contenido) + 1;
        memcpy(puntero_a_direccion_fisica, contenido, contenido_length);

        int se_ha_escrito = 1;
        send(socket_filesystem, &se_ha_escrito, sizeof(int), 0); */
    }
}