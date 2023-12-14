#include "kernel.h"

//=============================================== Variables Globales =========================================================================================================
int indice_pid = 0;

pthread_mutex_t mutex_new;

uint32_t AX;
uint32_t BX;
uint32_t CX;
uint32_t DX;

static void enviar_creacion_estructuras(t_pcb* pcb, int cant_paginas_proceso, char* path);

//============================================================================================================================================================================
t_pcb* crear_pcb(int prioridad, int cant_paginas_proceso, char* path) 
{
    //Nota Diego: Actualmente se puede hacer malloc de t_pcb pq todo es estatico
    t_pcb* pcb = malloc(sizeof(t_pcb)); 

    sem_wait(&(mutex_pid));
    indice_pid ++;
    sem_post(&(mutex_pid)); 
    pcb->pid = indice_pid;
    pcb->program_counter = 0;
    pcb->registros_cpu.AX = 0;
    pcb->registros_cpu.BX = 0;
    pcb->registros_cpu.CX = 0;
    pcb->registros_cpu.DX = 0;
    pcb->prioridad = prioridad;
    pcb->estado_pcb = NEW;
    pcb->quantum  = 2000;

    /*para inicializar el t_recurso le tengo que asignar memoria porque es un puntero a la estructura
    asi como hice para t_pcb*/
     pcb->recursos_asignados = malloc(tamanio_recursos * sizeof(t_recurso));

    // Inicialización de cada recurso
    for (int i = 0; i < tamanio_recursos; ++i) {
        strcpy(pcb->recursos_asignados[i].nombre_recurso, nombres_recursos[i]);  // Puedes asignar el nombre que desees
        pcb->recursos_asignados[i].instancias_recurso = 0;
    }

    pcb->recurso_pedido = NULL;
    pcb->sleep = 0;

    
    if (path != NULL) {
    pcb->path_proceso = strdup(path);
    if (pcb->path_proceso == NULL) {
        log_error(kernel_logger, "No se pudo alocar memoria\n");
        free(pcb);
        inicializar_consola_interactiva();
    }
    } else {
        log_error(kernel_logger, "No me enviaste un path correcto\n");
        free(pcb);
        inicializar_consola_interactiva();
    }

    pcb->nombre_archivo = NULL;
    pcb->modo_apertura = NULL;
    pcb->direccion_fisica_proceso = 0;
    pcb->tamanio_archivo = -1;
    pcb->puntero = 0;

    pcb->archivos_abiertos = list_create();

    log_info(kernel_logger, "Se crea el proceso %d en NEW \n", pcb->pid);

    pthread_mutex_lock(&mutex_new);
    meter_en_cola(pcb, NEW,cola_NEW);
    pthread_mutex_unlock(&mutex_new);

    mostrar_lista_pcb(cola_NEW,"NEW");
   
    enviar_creacion_estructuras(pcb,cant_paginas_proceso, path);

    sem_post (&hay_proceso_nuevo);

    return pcb;
}

static void enviar_creacion_estructuras(t_pcb* pcb, int cant_paginas_proceso, char* path) {
    t_paquete* paquete = crear_paquete(CREACION_ESTRUCTURAS_MEMORIA);
    agregar_entero_a_paquete(paquete,pcb-> pid);
    agregar_entero_a_paquete(paquete,cant_paginas_proceso);
    agregar_cadena_a_paquete(paquete,path);
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);

    int respuesta = 0;
    recv(socket_memoria, &respuesta,sizeof(int),0);

    if (respuesta != 1)
    {
        log_error(kernel_logger, "No se pudieron crear estructuras en memoria");
    }
}
void enviar_pcb_a_cpu(t_pcb* pcb_a_enviar)
{
    t_paquete *paquete = crear_paquete(PCB);

    agregar_entero_a_paquete(paquete, pcb_a_enviar->pid);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->program_counter);
    //log_info(kernel_logger, "Envio a cpu PID: %d PC:%d", pcb_a_enviar->pid, pcb_a_enviar->program_counter);
    agregar_entero_sin_signo_a_paquete(paquete, pcb_a_enviar->puntero);

    agregar_entero_sin_signo_a_paquete(paquete, pcb_a_enviar->registros_cpu.AX); 
    agregar_entero_sin_signo_a_paquete(paquete, pcb_a_enviar->registros_cpu.BX);
    agregar_entero_sin_signo_a_paquete(paquete, pcb_a_enviar->registros_cpu.CX);
    agregar_entero_sin_signo_a_paquete(paquete, pcb_a_enviar->registros_cpu.DX);
    agregar_entero_sin_signo_a_paquete(paquete, pcb_a_enviar->direccion_fisica_proceso);

    // Iterar sobre cada recurso y agregarlo al paquete
    for (int i = 0; i < tamanio_recursos; ++i) {
        agregar_cadena_a_paquete(paquete, pcb_a_enviar->recursos_asignados[i].nombre_recurso);
        agregar_entero_a_paquete(paquete, pcb_a_enviar->recursos_asignados[i].instancias_recurso);
    }

    //agregar_entero_a_paquete(paquete, pcb_a_enviar->archivosAbiertos); ///hay que ver como mandamos esto

    enviar_paquete(paquete, socket_cpu_dispatch);
    eliminar_paquete(paquete);
    return;
}

char* recibir_contexto(t_pcb* proceso)
 {
    t_paquete* paquete = recibir_paquete(socket_cpu_dispatch);
    void* stream = paquete->buffer->stream;

	if(paquete->codigo_operacion == PCB)
	{
        proceso->program_counter = sacar_entero_de_paquete(&stream);
        //log_info(kernel_logger, "Recibo de cpu PID: %d PC:%d", proceso->pid, proceso->program_counter);
        AX = sacar_entero_sin_signo_de_paquete(&stream);
        BX = sacar_entero_sin_signo_de_paquete(&stream);
        CX = sacar_entero_sin_signo_de_paquete(&stream);
        DX = sacar_entero_sin_signo_de_paquete(&stream);
        proceso->motivo_bloqueo= sacar_cadena_de_paquete(&stream);
        proceso->recurso_pedido = sacar_cadena_de_paquete(&stream); 
        proceso->sleep = sacar_entero_de_paquete(&stream);
        proceso->pagina_pedida = sacar_entero_de_paquete(&stream);
        proceso->nombre_archivo = sacar_cadena_de_paquete(&stream);
        proceso->modo_apertura = sacar_cadena_de_paquete(&stream);
        proceso->puntero = sacar_entero_sin_signo_de_paquete(&stream);
        proceso->direccion_fisica_proceso = sacar_entero_sin_signo_de_paquete(&stream);
        proceso->tamanio_archivo = sacar_entero_de_paquete(&stream);
    }
    else{
        log_error(kernel_logger, "Falla al recibir PCB, se cierra el Kernel \n");
        abort();
    }    

    if(string_equals_ignore_case(proceso->recurso_pedido, "basura")) {
        free(proceso->recurso_pedido);
        proceso->recurso_pedido = NULL;
    }

    if(string_equals_ignore_case(proceso->nombre_archivo, "basura")) {
        free(proceso->nombre_archivo);
        proceso->nombre_archivo = NULL;
    }

    if(string_equals_ignore_case(proceso->modo_apertura, "basura")) {
        free(proceso->modo_apertura); 
        proceso->modo_apertura = NULL;
    }
    log_info(kernel_logger, "Recibi el PCB %d de la cpu por motivo de: %s\n", proceso->pid, proceso->motivo_bloqueo);

    eliminar_paquete(paquete);
    return proceso->motivo_bloqueo;
}

void eliminar_pcb(t_pcb* proceso)
{
    eliminar_recursos_asignados(proceso);
    if (proceso->path_proceso != NULL) {
        free(proceso->path_proceso);
    }
     if (proceso->recurso_pedido != NULL) {
        free(proceso->recurso_pedido);
    }

    list_destroy(proceso->archivos_abiertos);
    free(proceso);
    proceso = NULL;
}

void eliminar_recursos_asignados(t_pcb* proceso) {

    liberar_todos_recurso(proceso);
    free(proceso->recursos_asignados);
}

void liberar_todos_recurso(t_pcb* proceso)
{
    //por cada recurso asignado que tiene el proceso, tengo que liberarlo y ver si ese recurso tiene procesos esperando
    //int tamanio_asignados = string_array_size(proceso->recursos_asignados->nombre_recurso);
    int instancias = 0;
    char* recurso_asig = NULL;

    for (int i = 0; i < tamanio_recursos; i++)
    {
        if(proceso->recursos_asignados[i].instancias_recurso > 0)
        {
            //agarro uno por uno cada recurso que tiene el proceso
            recurso_asig = proceso->recursos_asignados[i].nombre_recurso;
            int indice_pedido = indice_recurso(recurso_asig);

            //le sumo una instancia a cada recurso que se esta liberando
            instancias = instancias_del_recurso[indice_pedido];
            instancias++;
            instancias_del_recurso[indice_pedido] = instancias;
            
            //buscamos la lista del recurso que se libero, dentro de la lista de recursos
            t_list *cola_bloqueados_recurso = (t_list *)list_get(lista_recursos, indice_pedido);

            //si hay algun proceso esperando por el recurso liberado
            if(!list_is_empty(cola_bloqueados_recurso))
            {
                list_remove_element(cola_bloqueados_recurso, (void *)proceso);

                //agarramos el primer proceso que esta bloqueado dentro de esa lista
                t_pcb *pcb_desbloqueado = obtener_bloqueado_por_recurso(cola_bloqueados_recurso);

                log_info(kernel_logger, "Se libero el recurso: [%s] y se desbloquea el PID [%d]", recurso_asig, pcb_desbloqueado->pid);

                //antes de ver su hay deadlock tengo que asignar el recurso liberado al proceso que estaba esperando
                strcpy(pcb_desbloqueado->recursos_asignados[indice_pedido].nombre_recurso, recurso_asig);
                pcb_desbloqueado->recursos_asignados[indice_pedido].instancias_recurso++;
                pcb_desbloqueado->recurso_pedido = NULL;

                instancias = instancias_del_recurso[indice_pedido];
                instancias--;
                instancias_del_recurso[indice_pedido] = instancias;

                for (int i = 0; i < tamanio_recursos; ++i) {
                    log_info(kernel_logger, "Recursos Asignados: %s - Cantidad: %d",pcb_desbloqueado->recursos_asignados[i].nombre_recurso, pcb_desbloqueado->recursos_asignados[i].instancias_recurso);
                }

                obtener_siguiente_blocked(pcb_desbloqueado);
            }            
        }

        //busco el proceso que finaliza en cada una de las colas de recursos
        t_list *cola_recurso = (t_list *)list_get(lista_recursos, i);
        if(list_remove_element(cola_recurso, (void *)proceso)){
            instancias = instancias_del_recurso[i];
            instancias++;
            instancias_del_recurso[i] = instancias;
        }
    }
}



void eliminar_archivos_abiertos(t_dictionary *archivosAbiertos)
{
    //esto hay que revisarlo porque no se si esta bien, pero a rezar que lo ultimo que se pierde es la esperanza
    dictionary_destroy_and_destroy_elements(archivosAbiertos, dictionary_elements(archivosAbiertos));
}
