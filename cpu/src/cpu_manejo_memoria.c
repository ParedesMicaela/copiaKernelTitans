#include "cpu_utils.h"

int page_fault = -1;
bool hay_page_fault = false;

//======================= Funciones Internas ==============================================================================
static int traducir_pagina_a_marco(uint32_t numero_pagina, t_contexto_ejecucion* contexto_ejecucion);
static void pedir_numero_frame(uint32_t numero_pagina, t_contexto_ejecucion* contexto_ejecucion);
static int numero_marco_pagina();
static void enviar_paquete_READ(uint32_t direccion_fisica, t_contexto_ejecucion* contexto_ejecucion, uint32_t direccion_logica_aux);
static uint32_t recibir_valor_a_insertar(int socket_cliente_memoria);
static void enviar_paquete_WRITE(uint32_t direccion_fisica, int registro, t_contexto_ejecucion* contexto_ejecucion, uint32_t direccion_logica_aux);
/// MMU ///

uint32_t traducir_de_logica_a_fisica(uint32_t direccion_logica, t_contexto_ejecucion* contexto_ejecucion) {
    uint32_t numero_pagina = 0;
    uint32_t offset = 0 ;
    int numero_marco = 0;
    uint32_t direccion_fisica = 0;

    // Calculamos numero_pagina y offset
    numero_pagina = floor(direccion_logica / tam_pagina);
    offset = direccion_logica - (numero_pagina  * tam_pagina);

    //Llamos a la  Memoria, para conseguir el número de marco correspondiente a la página 
    numero_marco = traducir_pagina_a_marco(numero_pagina, contexto_ejecucion); 

    //Le avisa a Kernel que hay Page_fault(-1)
    if(numero_marco == page_fault) {
        hay_page_fault = true;
        return UINT32_MAX;
    }

    // Calculamos la direcion fisica
    direccion_fisica = numero_marco * tam_pagina  + offset;

    return direccion_fisica;
}

static int traducir_pagina_a_marco(uint32_t numero_pagina, t_contexto_ejecucion* contexto_ejecucion) {
    pedir_numero_frame(numero_pagina, contexto_ejecucion);
    log_info(cpu_logger, "Pagina enviada a memoria \n");
    int numero_marco = numero_marco_pagina();
    log_info(cpu_logger, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d \n", contexto_ejecucion->pid, numero_pagina, numero_marco);
    return numero_marco;
}

static void pedir_numero_frame(uint32_t numero_pagina, t_contexto_ejecucion* contexto_ejecucion) {
    t_paquete *paquete = crear_paquete(TRADUCIR_PAGINA_A_MARCO);
    agregar_entero_sin_signo_a_paquete(paquete, numero_pagina);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    enviar_paquete(paquete, socket_cliente_memoria);
    eliminar_paquete(paquete);
}

static int numero_marco_pagina() {
    int numero_marco = 0;

    t_paquete* paquete = recibir_paquete(socket_cliente_memoria);
    void* stream = paquete->buffer->stream;

    if (paquete->codigo_operacion == NUMERO_MARCO) 
    {
        numero_marco = sacar_entero_de_paquete(&stream);

        eliminar_paquete(paquete);
        
        return numero_marco;
    }
    else
    {
        log_error(cpu_logger,"No me enviaste el numero de marco :( \n");
        abort();
    }
    eliminar_paquete(paquete);
}

/// FUNCIONES DE CPU Y KERNEL ///

void mov_in(char* registro, uint32_t direccion_fisica, t_contexto_ejecucion* contexto_ejecucion, uint32_t direccion_logica_aux) {

    if(direccion_fisica!=UINT32_MAX){
     enviar_paquete_READ(direccion_fisica, contexto_ejecucion, direccion_logica_aux);

    int valor = recibir_valor_a_insertar(socket_cliente_memoria);

    setear_registro(registro,valor);
    
    log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %d \n", contexto_ejecucion->pid, "LEER", direccion_fisica, valor);
    }
}

static void enviar_paquete_READ(uint32_t direccion_fisica, t_contexto_ejecucion* contexto_ejecucion, uint32_t direccion_logica_aux) {
    t_paquete *paquete = crear_paquete(READ);
     agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
     agregar_entero_sin_signo_a_paquete(paquete,direccion_fisica);
     agregar_entero_sin_signo_a_paquete(paquete, direccion_logica_aux);
     enviar_paquete(paquete, socket_cliente_memoria);    
     eliminar_paquete (paquete);
}

static uint32_t recibir_valor_a_insertar() {
    uint32_t valor_registro = 0;

    t_paquete* paquete = recibir_paquete(socket_cliente_memoria);
    void* stream = paquete->buffer->stream;
    if (paquete->codigo_operacion == VALOR_READ)
    {
        valor_registro = sacar_entero_sin_signo_de_paquete(&stream);
        return valor_registro;
    }
    else
    {
        log_error(cpu_logger,"No me enviaste el valor a leer:( \n");
        abort();
    }
    eliminar_paquete(paquete);
}

void mov_out(uint32_t direccion_fisica, char* registro, t_contexto_ejecucion* contexto_ejecucion, uint32_t direccion_logica_aux) { 

    int valor = buscar_registro(registro);

    if(direccion_fisica != UINT32_MAX){    
     enviar_paquete_WRITE(direccion_fisica, valor, contexto_ejecucion, direccion_logica_aux);

    int se_ha_escrito = 1;
    recv(socket_cliente_memoria, &se_ha_escrito, sizeof(int), 0);

    log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %d \n", contexto_ejecucion->pid, "ESCRIBIR", direccion_fisica, valor);
    }
}

static void enviar_paquete_WRITE(uint32_t direccion_fisica, int registro, t_contexto_ejecucion* contexto_ejecucion, uint32_t direccion_logica_aux) {
    t_paquete *paquete = crear_paquete(WRITE);
     agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
     agregar_entero_sin_signo_a_paquete(paquete,direccion_fisica);
     agregar_entero_sin_signo_a_paquete(paquete, direccion_logica_aux);
     agregar_entero_a_paquete(paquete, registro);
     enviar_paquete(paquete, socket_cliente_memoria);    
     eliminar_paquete (paquete);
}