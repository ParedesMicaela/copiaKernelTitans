#include "kernel.h"

//=============================================== Variables Globales =========================================================================================================
uint32_t indice_pid = 0;

//============================================================================================================================================================================
//cada vez que la consola interactiva nos dice de crear un pcb, nos va a pasar la prioridad, el pid lo podemos poner nosotros
t_pcb *crear_pcb(int prioridad) 
{
    //esto lo ponemos aca para no tener que hacerlo en la funcion iniciar_proceso si total lo vamos a hacer siempre
    t_pcb *pcb = malloc(sizeof(*pcb)); //nota de Martín: este malloc después se libera en cpu cuando termina el proceso junto al pcb (posible memory leak?)

    //el indice lo vamos a estar modificando cada vez que tengamos que crear un pcb entonces conviene ponerlo como variable global
    //cosa que todos sabemos cuanto vale y no repetimos pid
    indice_pid ++;
    pcb->pid = indice_pid;
    pcb->program_counter = 0;
    pcb->prioridad=prioridad;
    pcb->pid = pid;
    
    pcb->program_counter = 0;
    pcb->registros_cpu.AX = 0;
    pcb->registros_cpu.BX = 0;
    pcb->registros_cpu.CX = 0;
    pcb->registros_cpu.DX = 0;
    //pcb->tabla_archivos_abiertos = diccionario;
    
    log_info(kernel_logger, "Se crea el proceso %d en NEW", pcb->pid);

    //se pueden agregar mas cosas segun necesitemos

    return pcb;
}

void enviar_pcb_a_cpu(t_pcb* pcb_a_enviar)
{
    t_paquete *paquete = crear_paquete(PCB);

    agregar_entero_a_paquete(paquete, pcb_a_enviar->pid);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->program_counter);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->pid);

    // ojo al piojo, es un struct, tengo que mandar los registros por separado
    agregar_entero_a_paquete(paquete, pcb_a_enviar->program_counter);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->registros_cpu.AX); 
    agregar_entero_a_paquete(paquete, pcb_a_enviar->registros_cpu.BX);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->registros_cpu.CX);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->registros_cpu.DX);
    
    //agregar diccionario a paquete

    enviar_paquete(paquete, socket_cpu_dispatch);
    log_info(kernel_logger, "Se envio el PCB %d a la CPU", pcb_a_enviar->pid);

    return;
}
