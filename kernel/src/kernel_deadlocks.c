#include "kernel.h"

void deteccion_deadlock (t_pcb* proceso)
{
    //voy a ver si el proceso P1 que pidio un recurso, tiene al menos 1 recurso asignado, sino no cumple deadlock
    for (int i = 0; i < 3; ++i)
    {
        if (proceso->recursos_asignados[i].instancias_recurso > 0)
        {
            //si tiene al menos 1 recurso asignado entonces vemos si hay deadlock.
            log_info(kernel_logger, "EL PID [%d] esta reteniendo %s", proceso->pid, proceso->recursos_asignados[i].nombre_recurso);
            char* recurso_involucrado = proceso->recursos_asignados[i].nombre_recurso;

            //necesito saber si el recurso que P1 esta reteniendo, tiene una cola de espera
            t_list* cola_recurso = (t_list *)list_get(lista_recursos, i);

            //si hay algo en la cola de espera, entonces hay un P2 esperando por ese recurso que P1 tiene
            if(cola_recurso != NULL && list_size(cola_recurso) > 0)
            {
                /*voy a buscar dentro de la cola de bloqueados del recurso, un P2 que este reteniendo 
                el recurso que P1 esta pidiendo, para ver si hay espera circular->deadlock*/
                for(int j = 0; j < list_size(cola_recurso); j++)
                {
                    if (proceso_reteniendo_recurso((t_pcb*)list_get(cola_recurso, j), recurso_involucrado)) 
                    {
                        t_pcb* proceso_involucrado = (t_pcb*)list_get(cola_recurso, j);
                        log_info(kernel_logger, "EL PID [%d] esta reteniendo %s", proceso_involucrado->pid, recurso_involucrado);
                    }
                }
            }
        }
    }
}

bool proceso_reteniendo_recurso(t_pcb* proceso,char* recurso) {

    for (int i = 0; i < 3; ++i) {
        if (strcmp(proceso->recursos_asignados[i].nombre_recurso, recurso) == 0) {
            return true;  // El recurso está siendo retenido por el proceso
        }
    }
    return false;  // El proceso no está reteniendo el recurso
}

/*
va a haber deadlock cuendo un proceso pida recursos mientras este reteniendo recursos que necesita otro proceso
para ejecutar

ni idea, todavia no creo que sea de importancia esto

Detección y resolución de Deadlocks
Frente a acciones donde los procesos lleguen al estado Block (por recursos o archivos)
se debe realizar una detección automática de deadlock (proceso bloqueados entre sí).
En caso que se detecte afirmativamente un deadlock se debe informar el mismo por consola.
En dicho mensaje se debe informar cuales son los procesos (PID) que se encuentran en deadlocks,
cuales son los recursos (o archivos) tomados y cuales son los recursos (o archivos) requeridos
(por los que fueron bloqueados).
La resolución de deadlocks se realizará de forma manual por la consola del Kernel utilizando el
mensaje “Finalizar proceso” donde el proceso finalizado deberá liberar los recursos (o archivos) tomados.
Una vez realizada esta acción se debe volver a realizar una nueva detección de deadlock.


*/
