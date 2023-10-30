#include "cpu_utils.h"

//======================= Funciones Internas ==============================================================================
static uint32_t traducir_pagina_a_marco(uint32_t numero_pagina);
static void pedir_numero_frame(uint32_t numero_pagina);
static uint32_t numero_marco_pagina();
static void enviar_paquete_READ(uint32_t direccion_fisica);
static uint32_t recibir_valor_a_insertar(int socket_cliente_memoria);
static void enviar_paquete_WRITE(uint32_t direccion_fisica, uint32_t registro);
/// MMU ///

uint32_t traducir_de_logica_a_fisica(uint32_t direccion_logica) {

    // Calculamos numero_pagina y offset
    uint32_t numero_pagina = direccion_logica / tam_pagina;
    uint32_t offset = direccion_logica - (numero_pagina  * tam_pagina);

    //Llamos a la  Memoria, para conseguir el número de marco correspondiente a la página 
    uint32_t numero_marco = traducir_pagina_a_marco(numero_pagina);

    // Calculamos la direcion fisica
    uint32_t direccion_fisica = (numero_marco * tam_pagina) + offset;

    return direccion_fisica;
}

static uint32_t traducir_pagina_a_marco(uint32_t numero_pagina) {
    pedir_numero_frame(numero_pagina);
    log_info(cpu_logger, "Página enviada a memoria \n");
    uint32_t numero_marco = numero_marco_pagina();

    return numero_marco;
}

static void pedir_numero_frame(uint32_t numero_pagina) {
    t_paquete *paquete = crear_paquete(TRADUCIR_PAGINA_A_MARCO);
    agregar_entero_sin_signo_a_paquete(paquete, numero_pagina);
    enviar_paquete(paquete, socket_cliente_memoria);
    eliminar_paquete(paquete);
}

static uint32_t numero_marco_pagina() {
    uint32_t numero_marco = 0;

    t_paquete* paquete = recibir_paquete(socket_cliente_memoria);
    void* stream = paquete->buffer->stream;
    if (paquete->codigo_operacion == NUMERO_MARCO)
    {
        numero_marco = sacar_entero_sin_signo_de_paquete(&stream);
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

void mov_in(uint32_t registro, uint32_t direccion_logica) {

    uint32_t direccion_fisica = UINT32_MAX;

    direccion_fisica = traducir_de_logica_a_fisica(direccion_logica);

    if(direccion_fisica!=UINT32_MAX){
     enviar_paquete_READ(direccion_fisica);

    registro = recibir_valor_a_insertar(socket_cliente_memoria);
    
    log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %s \n", contexto_ejecucion->pid, "LEER", direccion_fisica, registro);
    }
}

static void enviar_paquete_READ(uint32_t direccion_fisica) {
    t_paquete *paquete = crear_paquete(READ);
     agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
     agregar_entero_sin_signo_a_paquete(paquete,direccion_fisica);
     enviar_paquete(paquete, socket_cliente_memoria);    
     eliminar_paquete (paquete);
}

static uint32_t recibir_valor_a_insertar(int socket_cliente_memoria) {
    uint32_t registro = 0;

    t_paquete* paquete = recibir_paquete(socket_cliente_memoria);
    void* stream = paquete->buffer->stream;
    if (paquete->codigo_operacion == VALOR_READ)
    {
        registro = sacar_entero_sin_signo_de_paquete(&stream);
        return registro;
    }
    else
    {
        log_error(cpu_logger,"No me enviaste el valor a leer:( \n");
        abort();
    }
    eliminar_paquete(paquete);
}

void mov_out(uint32_t direccion_logica, uint32_t registro) { 

    uint32_t direccion_fisica = UINT32_MAX;

    direccion_fisica = traducir_de_logica_a_fisica(direccion_logica);

    if(direccion_fisica != UINT32_MAX){    
     enviar_paquete_WRITE(direccion_fisica, registro);

    // NO se si tengo que hacer una devolucion de la memoria y el log_info lo hago del valor escrito

    log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %s \n", contexto_ejecucion->pid, "ESCRIBIR", direccion_fisica, registro);
    }
}

static void enviar_paquete_WRITE(uint32_t direccion_fisica, uint32_t registro) {
    t_paquete *paquete = crear_paquete(WRITE);
     agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
     agregar_entero_sin_signo_a_paquete(paquete,direccion_fisica);
     agregar_entero_sin_signo_a_paquete(paquete, registro);
     enviar_paquete(paquete, socket_cliente_memoria);    
     eliminar_paquete (paquete);
}