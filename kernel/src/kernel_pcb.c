#include "kernel.h"


t_pcb *crear_pcb(int pid)
{
    t_pcb *pcb = malloc(sizeof(*pcb));
    
    pcb->pid = pid;
    pcb->program_counter = 0;
    //cb->registros_cpu = lista o estructura;
    //pcb->tabla_archivos_abiertos = diccionario;
    
    //se pueden agregar mas cosas segun necesitemos

    return pcb;
}

void enviar_pcb_a_cpu(t_pcb* pcb_a_enviar)
{
    t_paquete *paquete = crear_paquete(PCB);

    agregar_entero_a_paquete(paquete, pcb_a_enviar->pid);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->program_counter);
    //agregar registro a paquete
    //agregar diccionario a paquete

    enviar_paquete(paquete, socket_cpu_dispatch);

    return;
}
