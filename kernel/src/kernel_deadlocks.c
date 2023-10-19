#include "kernel.h"

char* recurso_retenido = NULL;

//==========================================================================================================================================================

void deteccion_deadlock (t_pcb* proceso, char* recurso_pedido)
{
    //voy a ver si el proceso P1 que pidio un recurso, tiene al menos 1 recurso asignado, sino no cumple deadlock
    for (int i = 0; i < 3; ++i)
    {
        if (proceso->recursos_asignados[i].instancias_recurso > 0)
        {
            //si tiene al menos 1 recurso asignado entonces vemos si hay deadlock.
            log_info(kernel_logger, "EL PID [%d] esta reteniendo %s\n", proceso->pid, proceso->recursos_asignados[i].nombre_recurso);
            recurso_retenido = strdup(proceso->recursos_asignados[i].nombre_recurso);

            //necesito saber si el recurso que P1 esta reteniendo, tiene una cola de espera
            t_list* cola_recurso = (t_list *)list_get(lista_recursos, i);

            //si hay algo en la cola de espera, entonces hay un P2 esperando por ese recurso que P1 tiene
            if(cola_recurso != NULL && list_size(cola_recurso) > 0)
            {

                /*voy a buscar dentro de la cola de bloqueados del recurso, un P2 que este reteniendo 
                el recurso que P1 esta pidiendo, para ver si hay espera circular->deadlock*/
                for(int j = 0; j < list_size(cola_recurso); j++)
                {
                    t_pcb* proceso_involucrado = list_get(cola_recurso, j);
                    if (proceso_reteniendo_recurso(proceso_involucrado, recurso_pedido)) 
                    {                       
                        //P1 que esta reteniendo el que P2 necesita
                        mensaje_deadlock_detectado(proceso, recurso_pedido);
                    }else
                    {
                        log_info(kernel_logger,"Analisis de deteccion de deadlock completado: NO hay deadlock");
                    }
                }
            }
        }
    }
    //si no retiene recursos, entonces no puede estar en deadlock
    //sem_post(&analisis_deadlock_completo);
}

bool proceso_reteniendo_recurso(t_pcb* proceso_involucrado, char* recurso) {

    for (int i = 0; i < tamanio_recursos; ++i) {
        if (strcmp(proceso_involucrado->recursos_asignados[i].nombre_recurso, recurso) == 0) {

            //P2 que esta reteniendo el que P1 necesita
            mensaje_deadlock_detectado(proceso_involucrado, recurso_retenido);
            return true; 
        }
    }
    return false; 
}

void mensaje_deadlock_detectado(t_pcb* proceso, char* recurso_requerido)
{
    char *recursos_totales = string_new();

    for (int i = 0; i < tamanio_recursos; i++) {

        //voy a guardar en un string los nombres de los recursos que tenga asignados
        if (proceso->recursos_asignados[i].instancias_recurso > 0)
        {
            char *recurso = string_duplicate(proceso->recursos_asignados[i].nombre_recurso);   

            //el recurso que encuentro asignado lo engancho al string de recursos totales
            string_append(&recursos_totales, recurso);
            free(recurso);

            //separo los recursos con comas
            if (i < tamanio_recursos - 1)
            {
                string_append(&recursos_totales, ", ");
            }
        }
    }

    log_info(kernel_logger, "Deadlock detectado: PID[%d] - Recursos en posesión [%s] - Recurso requerido: [%s]\n", proceso->pid, recursos_totales, recurso_requerido);

    if (recursos_totales != NULL) {
        free(recursos_totales); 
    }
}



