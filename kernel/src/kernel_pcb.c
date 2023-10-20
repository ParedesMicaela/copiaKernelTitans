#include "kernel.h"

//=============================================== Variables Globales =========================================================================================================
int indice_pid = 0;

pthread_mutex_t mutex_new;

uint32_t AX;
uint32_t BX;
uint32_t CX;
uint32_t DX;


//============================================================================================================================================================================
//cada vez que la consola interactiva nos dice de crear un pcb, nos va a pasar la prioridad, el pid lo podemos poner nosotros
t_pcb* crear_pcb(int prioridad, int tam_swap, char* path) 
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


    //pcb->tabla_archivos_abiertos = diccionario;
    //pcb->archivosAbiertos = dictionary_create();
    /*
    pthread_mutex_t *mutex = malloc(sizeof(*(pcb->mutex))); 
    pthread_mutex_init (mutex, NULL);
    pcb->mutex = mutex; */

    log_info(kernel_logger, "Se crea el proceso %d en NEW \n", pcb->pid);

    pthread_mutex_lock(&mutex_new);
    meter_en_cola(pcb, NEW,cola_NEW);
    pthread_mutex_unlock(&mutex_new);

    mostrar_lista_pcb(cola_NEW,"NEW");
   
    /*cada vez que creamos un proceso le tenemos que avisar a memoria que debe crear la estructura
    en memoria del proceso*/
    t_paquete* paquete = crear_paquete(CREACION_ESTRUCTURAS_MEMORIA);

    //a la memoria solamente le pasamos el pid y el tamanio que va a ocupar en swap, despues se encarga ella
    agregar_entero_a_paquete(paquete,pcb-> pid);
    agregar_entero_a_paquete(paquete,tam_swap);
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

    sem_post (&hay_proceso_nuevo);

    return pcb;
}

void enviar_pcb_a_cpu(t_pcb* pcb_a_enviar)
{
    t_paquete *paquete = crear_paquete(PCB);

    agregar_entero_a_paquete(paquete, pcb_a_enviar->pid);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->program_counter);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->prioridad);

    // ojo al piojo, es un struct, tengo que mandar los registros por separado
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
    log_info(kernel_logger, "\nSe envio el PCB %d a la CPU \n", pcb_a_enviar->pid);
    eliminar_paquete(paquete);
    return;
}

//hay que ver bien esto porque le faltan cosas y me tiene que poder devolver varias
char* recibir_contexto(t_pcb* proceso)
 {
    //esta funcion lo que hace es recibir un paquete, si ese paquete es un pcb lo abre y nos dice el motivo por el cual se devolvio
    char* motivo_de_devolucion = NULL;
    t_paquete* paquete = recibir_paquete(socket_cpu_dispatch);
    void* stream = paquete->buffer->stream;
    int program_counter =-1;
    char* recurso_pedido = NULL;
    int sleep_pedido = 0;
    
    //si lo que recibimos es en efecto un pcb, lo abrimos
	if(paquete->codigo_operacion == PCB)
	{
		//nosotros solamente vamos a sacar el contexto y el motivo, que es lo que mas nos importa
        program_counter = sacar_entero_de_paquete(&stream);
        AX = sacar_entero_sin_signo_de_paquete(&stream);
        BX = sacar_entero_sin_signo_de_paquete(&stream);
        CX = sacar_entero_sin_signo_de_paquete(&stream);
        DX = sacar_entero_sin_signo_de_paquete(&stream);
        motivo_de_devolucion = sacar_cadena_de_paquete(&stream);
        recurso_pedido = sacar_cadena_de_paquete(&stream); ////Problema Wait
        sleep_pedido = sacar_entero_de_paquete(&stream);

    }
    else{
        log_error(kernel_logger, "\nFalla al recibir PCB, se cierra el Kernel \n");
        abort();
    }

	//actualizamos el pc y los registros
	proceso->program_counter = program_counter;
    proceso->recurso_pedido = recurso_pedido;
    proceso->sleep = sleep_pedido;
    proceso->motivo_bloqueo = motivo_de_devolucion;

	//proceso->registros_cpu = registros;

    log_info(kernel_logger, "\nRecibi el PCB %d de la cpu por motivo de: %s\n", proceso->pid, motivo_de_devolucion);

    /*la cpu nos va a devolver el contexto si hace exit, algo de recursos o sleep. Si en recurso_pedido
    no me manda nada, es porque esta ejecutando exit o sleep. Entonces lo que voy a hacer aca es ver si
    me mando algun pedido de recursos y en vez de retornar el motivo unicamente, ir a otra funcion que 
    me diga el recurso y lo que quiere hacer el proceso con ese recurso.*/

    //si no me piden hacer algo con recursos, solamente retorno el motivo de devolucion
    //free(recurso_pedido);
    eliminar_paquete(paquete);
    return motivo_de_devolucion;
}

//eliminamos el pcb, sus estructuras, y lo de adentro de esas estructuras
void eliminar_pcb(t_pcb* proceso)
{
    eliminar_recursos_asignados(proceso);
    if (proceso->path_proceso != NULL) {
        free(proceso->path_proceso);
    }
    free(proceso->recurso_pedido);

}

void eliminar_recursos_asignados(t_pcb* proceso) {

    liberar_todos_recurso(proceso);
    free(proceso->recursos_asignados);
    //proceso->recursos_asignados = NULL;
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
            if (instancias <= 0)
            {
                //buscamos la lista del recurso que se libero, dentro de la lista de recursos
                t_list *cola_bloqueados_recurso = (t_list *)list_get(lista_recursos, indice_pedido);

                //agarramos el primer proceso que esta bloqueado dentro de esa lista
                t_pcb *pcb_desbloqueado = obtener_bloqueado_por_recurso(cola_bloqueados_recurso);

                //metemos el proceso que desbloqueamos a la cola de ready y el plani se fija despues cuando ejecutarlo
                pthread_mutex_lock(&mutex_ready);
                meter_en_cola(pcb_desbloqueado, READY, cola_READY);
                pthread_mutex_unlock(&mutex_ready);

                log_info(kernel_logger, "Se libero el recurso: [%s] y se desbloquea el PID [%d]", recurso_asig, pcb_desbloqueado->pid);
                deteccion_deadlock(pcb_desbloqueado, recurso_asig);
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
