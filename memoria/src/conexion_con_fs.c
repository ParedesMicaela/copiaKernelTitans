#include "memoria.h"

void enviar_pedido_pagina_para_escritura(int pid, int pag_pf)
{
    t_proceso_en_memoria* proceso = buscar_proceso_en_memoria(pid);
    t_pagina* pagina = proceso->paginas_en_memoria;
    t_paquete *paquete = crear_paquete(PEDIR_PAGINA_PARA_ESCRITURA);
    
    agregar_entero_a_paquete(paquete, pid);
    agregar_entero_a_paquete(paquete, pagina->entradas[pag_pf].marco);
    agregar_entero_a_paquete(paquete, pagina->entradas[pag_pf].numero_de_pagina);
    agregar_entero_a_paquete(paquete, pagina->entradas[pag_pf].posicion_swap);

    enviar_paquete(paquete, socket_filesystem);
    eliminar_paquete(paquete);
}

void recibir_pagina_para_escritura()
 {
    t_paquete* paquete = recibir_paquete(socket_filesystem);
    void* stream = paquete->buffer->stream;
    int numero_de_pagina;
    int marco; 
    int posicion_swap;     
    int pid;
    
	if(paquete->codigo_operacion == PAGINA_PARA_ESCRITURA)
	{
        numero_de_pagina = sacar_entero_de_paquete(&stream);
        marco = sacar_entero_de_paquete(&stream);
        posicion_swap = sacar_entero_de_paquete(&stream);
        pid = sacar_entero_de_paquete(&stream);
    }
    else{
        log_error(memoria_logger, "Falla al recibir página, se cierra la Memoria \n");
        abort();
    }

    
    eliminar_paquete(paquete);

    escribir_en_memoria_principal(numero_de_pagina, marco, posicion_swap, pid);

}

void escribir_en_swap(int pid, int cantidad_paginas_proceso){
    
    t_paquete* paquete = crear_paquete(ESCRIBIR_PAGINA_SWAP);

    t_proceso_en_memoria* proceso = buscar_proceso_en_memoria(pid);
    t_pagina* pagina = proceso->paginas_en_memoria;
    
    agregar_entero_a_paquete(paquete, pid);
    agregar_entero_a_paquete(paquete, cantidad_paginas_proceso);

     for (int i = 0; i < cantidad_paginas_proceso; ++i) {
    agregar_entero_a_paquete(paquete, pagina->entradas[i].marco);
    agregar_entero_a_paquete(paquete, pagina->entradas[i].numero_de_pagina);
    agregar_entero_a_paquete(paquete, pagina->entradas[i].posicion_swap);
    }
    
    enviar_paquete(paquete, socket_filesystem);
    eliminar_paquete(paquete);
}