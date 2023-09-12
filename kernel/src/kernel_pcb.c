#include "kernel.h"


t_pcb *crear_pcb(uint32_t pid) 
{
    t_pcb *pcb = malloc(sizeof(*pcb)); //nota de Martín: este malloc después se libera en cpu cuando termina el proceso junto al pcb (posible memory leak?)
    
    pcb->pid = pid;
    pcb->program_counter = 0;
    pcb->registros_cpu.AX = 0;
    pcb->registros_cpu.BX = 0;
    pcb->registros_cpu.CX = 0;
    pcb->registros_cpu.DX = 0;
    //pcb->tabla_archivos_abiertos = diccionario;
    
    //se pueden agregar mas cosas segun necesitemos

    return pcb;
}

void enviar_pcb_a_cpu(t_pcb* pcb_a_enviar)
{
    t_paquete *paquete = crear_paquete(PCB);

    agregar_entero_a_paquete(paquete, pcb_a_enviar->pid);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->program_counter);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->registros_cpu.AX); // ojo al piojo, es un struct, tengo que mandar los registros por separado
    agregar_entero_a_paquete(paquete, pcb_a_enviar->registros_cpu.BX);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->registros_cpu.CX);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->registros_cpu.DX);
    //agregar diccionario a paquete

    enviar_paquete(paquete, socket_cpu_dispatch);

    return;
}
