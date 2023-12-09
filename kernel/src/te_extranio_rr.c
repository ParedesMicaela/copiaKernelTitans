/*static void atender_round_robin(t_pcb* proceso_seleccionado) {
  
        int quantum = config_valores_kernel.quantum;
        usleep(1000 * quantum); // Simula la ejecución y pausa el proceso en milisegundos
        quantum = 0;
        t_pcb *proceso_en_exec = NULL;

        printf("Comparamos si el proceso que nos dieron es el que se esta ejecutando\n");
        
        pthread_mutex_lock(&mutex_exec);
        int tam_cola_execute = list_size(cola_EXEC);
        pthread_mutex_unlock(&mutex_exec);

        if(tam_cola_execute > 0) {
        pthread_mutex_lock(&mutex_exec);
        t_pcb * hay_proceso_en_exec = list_get(dictionary_int_get(diccionario_colas, EXEC),0);
        proceso_en_exec = hay_proceso_en_exec;
        pthread_mutex_unlock(&mutex_exec);
        }
        pthread_mutex_unlock(&mutex_exec);

        //si el que agarramos es el mismo que enviamos, significa que se quiere seguir ejecutando dps del quantum (nao nao)
        if (proceso_en_exec != NULL && proceso_en_exec->pid == proceso_seleccionado->pid )
        {
            log_info(kernel_logger, "\nPID[%d] ha agotado su quantum de RR y se mueve a READY\n", proceso_seleccionado->pid);
            
             // Desalojamos
            pthread_mutex_lock(&mutex_exec);
            list_remove_element(dictionary_int_get(diccionario_colas, EXEC), proceso_seleccionado);
            pthread_mutex_unlock(&mutex_exec);

            // Debes enviar una señal de desalojo al proceso en CPU.
            t_paquete *paquete = crear_paquete(DESALOJO);
            agregar_entero_a_paquete(paquete, 1);
            enviar_paquete(paquete, socket_cpu_interrupt);
            eliminar_paquete(paquete);

            //proceso_en_exit_rr = proceso_seleccionado;
        }
}
*/

