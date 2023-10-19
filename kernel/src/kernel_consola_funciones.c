#include "kernel.h"

// INICIAR PROCESO //
void iniciar_proceso(char *path, int tam_proceso_swap, int prioridad)
{
    log_info(kernel_logger, "Iniciando proceso \n");

    //Creamos un PCB, que comienza en NEW
    crear_pcb(prioridad, tam_proceso_swap, path);
}

void consola_detener_planificacion() {
    printf("PAUSA DE PLANIFICACIÓN \n");

    if(corriendo){
    pthread_mutex_lock(&mutex_corriendo);
    corriendo = 0;  //Bandera en Pausa
    pthread_mutex_unlock(&mutex_corriendo);
    }
    else printf("Ya esta detenida flaco");
}

void consola_iniciar_planificacion() {

  if(!corriendo) {
       printf("INICIO DE PLANIFICACIÓN \n");
        pthread_mutex_lock(&mutex_corriendo);
        pthread_cond_broadcast(&cond_corriendo);  
        corriendo = 1;  // Bandera sigue
        pthread_mutex_unlock(&mutex_corriendo);
  }
  else {
    printf("No estaba pausada la planificacion -_- \n");
  }
  
}

void consola_modificar_multiprogramacion(int valor) {
    if (valor <= config_valores_kernel.grado_multiprogramacion_ini) {
        printf("NO se puede modificador el grado de multiprogramacion \n"); 
    }
    else {
        int grado_anterior = config_valores_kernel.grado_multiprogramacion_ini;
        config_valores_kernel.grado_multiprogramacion_ini = valor;
        printf("Grado Anterior: %d - Grado Actual: %d \n", grado_anterior, valor);
    }
}

void consola_proceso_estado() {
    mostrar_lista_pcb(cola_NEW, "NEW");
    mostrar_lista_pcb(cola_READY, "READY");
    mostrar_lista_pcb(cola_BLOCKED, "BLOCKED");
    mostrar_lista_pcb(cola_EXEC, "EXEC");
    mostrar_lista_pcb(cola_EXIT, "EXIT");
}
/*
void consola_finalizar_proceso(int pid){

    printf("Entramos a finalizar proceso \n");

|   //Obtenemos el tamanio de cada cola
    pthread_mutex_lock(&mutex_blocked);
    int tam_cola_BLOCKED = list_size(cola_BLOCKED);
    pthread_mutex_unlock(&mutex_blocked);

    pthread_mutex_lock(&mutex_exec);
    int tam_cola_EXEC= list_size(cola_EXEC);
    pthread_mutex_unlock(&mutex_exec);

    pthread_mutex_lock(&mutex_new);
    int tam_cola_NEW = list_size(cola_NEW);
    pthread_mutex_unlock(&mutex_new);

    pthread_mutex_lock(&mutex_ready);
    int tam_cola_NEW = list_size(cola_READY);
    pthread_mutex_unlock(&mutex_ready);

    //BUSQUEDA DE PCB

    //Recorremos cola de bloqueados
    for(int i=0;i<tam_cola_BLOCKED ;i++)
        {
            //encontramos el pcb con el pid que mandamos
            if(pid == ((t_pcb*) list_get(tam_cola_BLOCKED, i))->pid)
            {
                t_pcb* pcb = ((t_pcb*) list_get(cola_BLOCKED, i)); // hacemos esto para llamarlo directamente pcb

                pthread_mutex_lock(&mutex_blocked);
                pcb = list_remove(dictionary_int_get(diccionario_colas, BLOCKED), 0);
                pthread_mutex_unlock(&mutex_blocked);

                liberar_pid(pid, EXIT, cola_EXIT);
                printf("Encontramos pcb en bloqueado\n");            
            }
        }

    //Recorremos cola de Execute
    for(int i=0;i<tam_cola_EXEC ;i++)
        {
            //encontramos el pcb con el pid que mandamos
            if(pid == ((t_pcb*) list_get(tam_cola_EXEC, i))->pid)
            {
                t_pcb* pcb = ((t_pcb*) list_get(cola_EXEC, i)); // hacemos esto para llamarlo directamente pcb

                pthread_mutex_lock(&mutex_exec);
                pcb = list_remove(dictionary_int_get(diccionario_colas, EXEC), 0);
                pthread_mutex_unlock(&mutex_exec);

                liberar_pid(pid, EXIT, cola_EXIT);
                printf("Encontramos pcb ejecutandose \n");            
            }
        }

    //Recorremos cola de NEW
    for(int i=0;i<tam_cola_NEW ;i++)
        {
            //encontramos el pcb con el pid que mandamos
            if(pid == ((t_pcb*) list_get(tam_cola_NEW, i))->pid)
            {
                t_pcb* pcb = ((t_pcb*) list_get(cola_NEW, i)); // hacemos esto para llamarlo directamente pcb

                pthread_mutex_lock(&mutex_new);
                pcb = list_remove(dictionary_int_get(diccionario_colas, NEW), 0);
                pthread_mutex_unlock(&mutex_new);

                liberar_pid(pid, EXIT, cola_EXIT);
                printf("Encontramos pcb ejecutandose \n");            
            }
        }

     //Recorremos cola de READY
    for(int i=0;i<tam_cola_READY ;i++)
        {
            //encontramos el pcb con el pid que mandamos
            if(pid == ((t_pcb*) list_get(tam_cola_READY, i))->pid)
            {
                t_pcb* pcb = ((t_pcb*) list_get(cola_READY, i)); // hacemos esto para llamarlo directamente pcb

                pthread_mutex_lock(&mutex_ready);
                pcb = list_remove(dictionary_int_get(diccionario_colas, READY), 0);
                pthread_mutex_unlock(&mutex_ready);

                liberar_pid(pid, EXIT, cola_EXIT);
                printf("Encontramos pcb ejecutandose \n");            
            }
        }
    
    //Si no lo encuentra en ninguna
    printf("Proceso equivocado, no se encontro. Intente nuevamente"); 
}


void liberar_pid(int pid, estado ESTADO, t_list *cola)
{
    // recorremos la cola y buscamos el pid del pcb
    for (int i = 0; i < list_size(cola); i++)
    {
        if (pid == ((t_pcb *)list_get(cola, i))->pid)
        {
            pthread_mutex_lock(&mutex_colas);
            list_remove(cola, i); // yendome del indice maximo de la lista
            pthread_mutex_unlock(&mutex_colas);
        }
    }

    // nuestro nuevo estado va a ser el estado al cual queremos cambiarlo
    pcb->estado_pcb = ESTADO;

    // finalmente lo agregamos a la cola de nuestro nuevo estado
    pthread_mutex_lock(&mutex_colas);
    list_add(cola, pcb);
    printf("PID: %d  es desalojado \n", pid);
    pthread_mutex_unlock(&mutex_colas);
}
*/
void consola_finalizar_proceso(int pid) {

    printf("Finalizamos proceso el proceso %d \n", pid);

    t_pcb* pcb_asociado = NULL;  
    //t_list* cola_con_proceso = NULL;
    int estado = -1;  

    // Recorremos cola de Blocked
    pthread_mutex_lock(&mutex_blocked);
    if (list_size(cola_BLOCKED) > 0) {
        for (int i = 0; i < list_size(cola_BLOCKED); i++) {
            t_pcb* pcb = list_get(cola_BLOCKED, i);
            if (pcb->pid == pid) {
                pcb_asociado = pcb;
                //cola_con_proceso = cola_BLOCKED;
                estado = BLOCKED;
                break;
            }
        }
    }
    pthread_mutex_unlock(&mutex_blocked);

    // Recorremos cola de Execute
    if (pcb_asociado == NULL) {
        pthread_mutex_lock(&mutex_exec);
        if (list_size(cola_EXEC) > 0) {
            for (int i = 0; i < list_size(cola_EXEC); i++) {
                t_pcb* pcb = list_get(cola_EXEC, i);
                if (pcb->pid == pid) {
                    pcb_asociado = pcb;
                    //cola_con_proceso = cola_EXEC;
                    estado = EXEC;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&mutex_exec);
    }

   // Recorremos cola de NEW
    if (pcb_asociado == NULL) {
        pthread_mutex_lock(&mutex_new);
        if (list_size(cola_NEW) > 0) {
            for (int i = 0; i < list_size(cola_NEW); i++) {
                t_pcb* pcb = list_get(cola_NEW, i);
                if (pcb->pid == pid) {
                    pcb_asociado = pcb;
                    //cola_con_proceso = cola_NEW;
                    estado = NEW;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&mutex_new);
    }

    // Recorremos cola de READY
    if (pcb_asociado == NULL) {
        pthread_mutex_lock(&mutex_ready);
        if (list_size(cola_READY) > 0) {
            for (int i = 0; i < list_size(cola_READY); i++) {
                t_pcb* pcb = list_get(cola_READY, i);
                if (pcb->pid == pid) {
                    pcb_asociado = pcb;
                    //cola_con_proceso = cola_READY;
                    estado = READY;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&mutex_ready);
    }

    //Si encuentra el pcb
    if (pcb_asociado != NULL) {

        //Lo desalojo
        if(estado == EXEC) {
        t_paquete *paquete_fin = crear_paquete(FINALIZACION_PROCESO);
        int interrupcion_exit = 2;
        agregar_entero_a_paquete(paquete_fin, interrupcion_exit);
        enviar_paquete(paquete_fin, socket_cpu_interrupt);
        eliminar_paquete(paquete_fin);
        }

        /*
        // Remuevo el pcb del diccionario
        pthread_mutex_lock(&mutex_colas);
        list_remove_element(dictionary_int_get(diccionario_colas, estado), pcb_asociado);
        pthread_mutex_unlock(&mutex_colas);

        // Lo meto en Exit
        pthread_mutex_lock(&mutex_exit);
        meter_en_cola(pcb_asociado, EXIT, cola_EXIT);
        pthread_mutex_unlock(&mutex_exit);

        // sacamos el proceso de la lista de exit
        pthread_mutex_lock(&mutex_exit);
        list_remove_element(dictionary_int_get(diccionario_colas, EXIT), pcb_asociado);
        pthread_mutex_unlock(&mutex_exit);

        printf("Finaliza el  PCB de ID: %d\n", pcb_asociado->pid);

        // le mandamos esto a memoria para que destruya las estructuras
        enviar_pcb_a_memoria(pcb_asociado, socket_memoria, FINALIZAR_EN_MEMORIA);
        printf("Enviando a memoria liberar estructuras del proceso \n");

        int fin_ok;
        recv(socket_memoria, &fin_ok, sizeof(int), 0);

        if (fin_ok != 1)
        {
        printf("No se pudieron eliminar estructuras en memoria del proceso PID[%d]\n", pcb_asociado->pid);
        }
    
        // si la respuesta que conseguimos de memoria es que se finalice la memoria, le avisamos a la consola que ya finaliza el proceso
        printf("Respuesta memoria de estructuras liberadas del proceso recibida \n");

        eliminar_pcb(pcb_asociado);
        sem_post(&grado_multiprogramacion);
        */
    } else {
        printf("Proceso no encontrado. Intente nuevamente.\n");
    }
}

/*
  // Remove el pcb de la cola
        pthread_mutex_lock(&mutex_colas);
        list_remove_element(target_queue, list_index_of(target_queue, pcb_to_remove));
        pthread_mutex_unlock(&mutex_colas);

        // Cambiamos estado a exit
        pcb_asociado->estado_pcb = EXIT;

        // Añadimos el PCB a exit
        pthread_mutex_lock(&mutex_colas);
        list_add(cola_EXIT, pcb_asociado);
        printf("PID: %d is desalojado\n", pid);
        pthread_mutex_unlock(&mutex_colas);
    */
