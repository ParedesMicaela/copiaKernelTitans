#include "memoria.h"

void enviar_pedido_pagina_para_escritura(int pid, int pag_pf, int socket_filesystem){

    t_paquete* paquete = crear_paquete(PEDIR_PAGINA_PARA_ESCRITURA);
    agregar_entero_a_paquete(paquete,pid);
    agregar_entero_a_paquete(paquete,pag_pf);
    
    enviar_paquete(paquete, socket_filesystem);
    eliminar_paquete(paquete);
}

t_pagina* recibir_pagina_para_escritura(int socket_filesystem){
    
    t_paquete *paquete = recibir_paquete(socket_filesystem);
    void *stream = paquete->buffer->stream;

    if (paquete->codigo_operacion == PAGINA_PARA_ESCRITURA)
    {
        //t_pagina* pagina_recibida = sacar_pagina_de_paquete(&stream);
        //return pagina_recibida;
    }
    else
    {
        log_error(memoria_logger,"Falla al recibir las pagina solicitada\n");
        abort();
    }
    eliminar_paquete(paquete);
    return NULL;
}

void escribir_en_swap(t_pagina* pagina, int socket_filesystem){
    
    t_paquete* paquete = crear_paquete(ESCRIBIR_PAGINA_SWAP);
    //agregar_pagina_a_paquete(paquete,pagina);
    
    enviar_paquete(paquete, socket_filesystem);
    eliminar_paquete(paquete);
}