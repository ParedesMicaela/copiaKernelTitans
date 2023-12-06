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
    pcb->posicion = -1;
    pcb->direccion_fisica_proceso = 0;
    pcb->tamanio_archivo = -1;

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
    log_info(kernel_logger, "Se manda mensaje a memoria para inicializar estructuras del proceso \n");
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
    agregar_entero_a_paquete(paquete, pcb_a_enviar->prioridad);

    agregar_entero_sin_signo_a_paquete(paquete, pcb_a_enviar->registros_cpu.AX); 
    agregar_entero_sin_signo_a_paquete(paquete, pcb_a_enviar->registros_cpu.BX);
    agregar_entero_sin_signo_a_paquete(paquete, pcb_a_enviar->registros_cpu.CX);
    agregar_entero_sin_signo_a_paquete(paquete, pcb_a_enviar->registros_cpu.DX);

    // Iterar sobre cada recurso y agregarlo al paquete
    for (int i = 0; i < tamanio_recursos; ++i) {
        agregar_cadena_a_paquete(paquete, pcb_a_enviar->recursos_asignados[i].nombre_recurso); //Problema Signal
        agregar_entero_a_paquete(paquete, pcb_a_enviar->recursos_asignados[i].instancias_recurso);
    }

    //agregar_entero_a_paquete(paquete, pcb_a_enviar->archivosAbiertos); ///hay que ver como mandamos esto

    enviar_paquete(paquete, socket_cpu_dispatch);
    eliminar_paquete(paquete);
    return;
}

char* recibir_contexto(t_pcb* proceso)
 {
    //esta funcion lo que hace es recibir un paquete, si ese paquete es un pcb lo abre y nos dice el motivo por el cual se devolvio
    char* motivo_de_devolucion = NULL;
    t_paquete* paquete = recibir_paquete(socket_cpu_dispatch);
    void* stream = paquete->buffer->stream;
    int program_counter =-1;
    char* recurso_pedido = NULL;
    int sleep_pedido = 0;
    int pagina_pedida = -1;
    
	if(paquete->codigo_operacion == PCB)
	{
		//nosotros solamente vamos a sacar el contexto y el motivo, que es lo que mas nos importa
        program_counter = sacar_entero_de_paquete(&stream);
        AX = sacar_entero_sin_signo_de_paquete(&stream);
        BX = sacar_entero_sin_signo_de_paquete(&stream);
        CX = sacar_entero_sin_signo_de_paquete(&stream);
        DX = sacar_entero_sin_signo_de_paquete(&stream);
        motivo_de_devolucion = sacar_cadena_de_paquete(&stream);
        recurso_pedido = sacar_cadena_de_paquete(&stream); 
        sleep_pedido = sacar_entero_de_paquete(&stream);
        pagina_pedida = sacar_entero_de_paquete(&stream);
        proceso->nombre_archivo = sacar_cadena_de_paquete(&stream);
        proceso->modo_apertura = sacar_cadena_de_paquete(&stream);
        proceso->posicion = sacar_entero_de_paquete(&stream);
        proceso->direccion_fisica_proceso = sacar_entero_sin_signo_de_paquete(&stream);
        proceso->tamanio_archivo = sacar_entero_de_paquete(&stream);
    }
    else{
        log_error(kernel_logger, "Falla al recibir PCB, se cierra el Kernel \n");
        abort();
    }

	proceso->program_counter = program_counter;
    proceso->recurso_pedido = recurso_pedido;
    proceso->sleep = sleep_pedido;
    proceso->motivo_bloqueo = motivo_de_devolucion;
    proceso->pagina_pedida = pagina_pedida;

    log_info(kernel_logger, "Recibi el PCB %d de la cpu por motivo de: %s\n", proceso->pid, motivo_de_devolucion);

    eliminar_paquete(paquete);
    return motivo_de_devolucion;
}

void eliminar_pcb(t_pcb* proceso)
{
    eliminar_recursos_asignados(proceso);
    if (proceso->path_proceso != NULL) {
        free(proceso->path_proceso);
    }
     if (proceso->recurso_pedido != NULL) {
       // free(proceso->recurso_pedido);
    }
}

void eliminar_recursos_asignados(t_pcb* proceso) {

    liberar_todos_recurso(proceso);
    free(proceso->recursos_asignados);
}

void liberar_todos_recurso(t_pcb* proceso)
{
    //por cada recurso asignado que tiene el proceso, tengo que liberarlo y ver si ese recurso tiene procesos esperando
    int tamanio_asignados = string_array_size(proceso->recursos_asignados->nombre_recurso);
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
            printf("cantidad instancias ahora: %d", instancias);
            if (instancias <= 0)
            {
                //buscamos la lista del recurso que se libero, dentro de la lista de recursos
                t_list *cola_bloqueados_recurso = (t_list *)list_get(lista_recursos, indice_pedido);

                list_remove_element(cola_bloqueados_recurso, (void *)proceso);
                //list_remove_and_destroy_element
                //agarramos el primer proceso que esta bloqueado dentro de esa lista
                t_pcb *pcb_desbloqueado = obtener_bloqueado_por_recurso(cola_bloqueados_recurso);

                log_info(kernel_logger, "Se libero el recurso: [%s] y se desbloquea el PID [%d]", recurso_asig, pcb_desbloqueado->pid);

                //antes de ver su hay deadlock tengo que asignar el recurso liberado al proceso que estaba esperando
                strcpy(pcb_desbloqueado->recursos_asignados[indice_pedido].nombre_recurso, recurso_asig);
                pcb_desbloqueado->recursos_asignados[indice_pedido].instancias_recurso++;
                pcb_desbloqueado->recurso_pedido = NULL;           
                //obtener_siguiente_blocked(pcb_desbloqueado);

                deteccion_deadlock(pcb_desbloqueado, recurso_asig);
                if(!hay_deadlock){
                    obtener_siguiente_blocked(pcb_desbloqueado);
                }
            }
        }
    }
}

void eliminar_registros_pcb (t_registros_cpu registros_cpu)
{
    free(registros_cpu.AX);
    free(registros_cpu.AX);
    free(registros_cpu.AX);
    free(registros_cpu.AX);
}
void eliminar_archivos_abiertos(t_dictionary *archivosAbiertos)
{
    //esto hay que revisarlo porque no se si esta bien, pero a rezar que lo ultimo que se pierde es la esperanza
    dictionary_destroy_and_destroy_elements(archivosAbiertos, dictionary_elements(archivosAbiertos));
}
/*
void eliminar_mutex(pthread_mutex_t *mutex)
{
    pthread_mutex_destroy(mutex);
}	*/
