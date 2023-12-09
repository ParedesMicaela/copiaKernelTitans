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

void consola_finalizar_proceso(int pid) {

    printf("Finalizamos proceso el proceso %d \n", pid);

    t_pcb* pcb_asociado = NULL;  
    int estado = -1;  

    // Recorremos cola de Blocked
    pthread_mutex_lock(&mutex_blocked);
    if (list_size(cola_BLOCKED) > 0) {
        for (int i = 0; i < list_size(cola_BLOCKED); i++) {
            t_pcb* pcb = list_get(cola_BLOCKED, i);
            if (pcb->pid == pid) {
                pcb_asociado = pcb;
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

        eliminar_pcb(pcb_asociado);
        } 
        else {
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

        int fin_ok = 0;
        recv(socket_memoria, &fin_ok, sizeof(int), 0);

        if (fin_ok != 1)
        {
        printf("No se pudieron eliminar estructuras en memoria del proceso PID[%d]\n", pcb_asociado->pid);
        }
    
        // si la respuesta que conseguimos de memoria es que se finalice la memoria, le avisamos a la consola que ya finaliza el proceso
        printf("Respuesta memoria de estructuras liberadas del proceso recibida \n");

        eliminar_pcb(pcb_asociado);
        sem_post(&grado_multiprogramacion);
    }
    } else {
        printf("Proceso no encontrado. Intente nuevamente.\n");
    }
}