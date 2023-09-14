#include "kernel.h"

//=============================================== Variables Globales =========================================================================================================
uint32_t indice_pid = 0;

//============================================================================================================================================================================
//cada vez que la consola interactiva nos dice de crear un pcb, nos va a pasar la prioridad, el pid lo podemos poner nosotros
t_pcb *crear_pcb(int prioridad, int tam_swap) 
{
    //esto lo ponemos aca para no tener que hacerlo en la funcion iniciar_proceso si total lo vamos a hacer siempre
    t_pcb *pcb = malloc(sizeof(*pcb)); 

    //el indice lo vamos a estar modificando cada vez que tengamos que crear un pcb entonces conviene ponerlo como variable global
    //cosa que todos sabemos cuanto vale y no repetimos pid
    indice_pid ++;
    pcb->pid = indice_pid;
    pcb->program_counter = 0;
    pcb->prioridad = prioridad;

    //cuando lo creamos el estado siempre es new
    pcb->estado_pcb = NEW;
    pcb->program_counter = 0;
    pcb->registros_cpu.AX = 0;
    pcb->registros_cpu.BX = 0;
    pcb->registros_cpu.CX = 0;
    pcb->registros_cpu.DX = 0;
    //pcb->tabla_archivos_abiertos = diccionario;
    pcb->archivosAbiertos = dictionary_create();
    pthread_mutex_t *mutex = malloc(sizeof(*(pcb->mutex))); 
    pthread_mutex_init (mutex, NULL);
    pcb->mutex = mutex;  

    meter_en_cola(pcb, NEW);

    log_info(kernel_logger, "Se crea el proceso %d en NEW", pcb->pid);

    /*cada vez que creamos un proceso le tenemos que avisar a memoria que debe crear la estructura
    en memoria del proceso*/
    t_paquete* paquete = crear_paquete(CREACION_ESTRUCTURAS_MEMORIA);

    //a la memoria solamente le pasamos el pid y el tamanio que va a ocupar en swap, despues se encarga ella
    agregar_entero_a_paquete(paquete,pcb-> pid);
    agregar_entero_a_paquete(paquete,tam_swap);

    enviar_paquete(paquete, socket_memoria);
    log_info(kernel_logger, "Se manda mensaje a memoria para inicializar estructuras del proceso");

    return pcb;
}

void enviar_pcb_a_cpu(t_pcb* pcb_a_enviar)
{
    t_paquete *paquete = crear_paquete(PCB);

    agregar_entero_a_paquete(paquete, pcb_a_enviar->pid);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->program_counter);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->pid);

    // ojo al piojo, es un struct, tengo que mandar los registros por separado
    agregar_entero_a_paquete(paquete, pcb_a_enviar->program_counter);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->registros_cpu.AX); 
    agregar_entero_a_paquete(paquete, pcb_a_enviar->registros_cpu.BX);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->registros_cpu.CX);
    agregar_entero_a_paquete(paquete, pcb_a_enviar->registros_cpu.DX);
    

    agregar_entero_a_paquete(paquete, pcb_a_enviar->archivosAbiertos); ///hay que ver como mandamos esto

    enviar_paquete(paquete, socket_cpu_dispatch);
    log_info(kernel_logger, "Se envio el PCB %d a la CPU", pcb_a_enviar->pid);

    return;
}


void destruir_pcb (t_pcb* pcb){
    pthread_mutex_lock(pcb_get_mutex(pcb));

//ir agregando las cosas que sean necesarias eliminar

    t_dictionary *archivosAbiertos = pcb->archivosAbiertos;
    if (archivosAbiertos != NULL){
        dictionary_destroy(archivosAbiertos);
    }

    t_registros_cpu *registros_cpu = pcb->registros_cpu.AX;
    if (registros_cpu->AX != NULL){
        free(pcb->registros_cpu.AX);
    }
     t_registros_cpu *registros_cpu = pcb->registros_cpu.BX;
    if (registros_cpu->BX != NULL){
        free(pcb->registros_cpu.BX);
    }
     t_registros_cpu *registros_cpu = pcb->registros_cpu.CX;
    if (registros_cpu->CX != NULL){
        free(pcb->registros_cpu.CX);
    }
     t_registros_cpu *registros_cpu = pcb->registros_cpu.DX;
    if (registros_cpu->DX != NULL){
        free(pcb->registros_cpu.DX);
    }
    //eliminar solo el diccionario sirve o también hay que eliminar sus elementos?
    //en tal caso tendría que ser un dictionary_destroy_and_destroy_elements
    pthread_mutex_unlock(pcb_get_mutex(pcb));

    pthread_mutex_destroy(pcb->mutex);
    free(pcb->mutex); //libero el malloc de la linea 29

    free(pcb); // libero el malloc de la linea 11

}


//get mutex pcb

pthread_mutex_t* pcb_get_mutex(t_pcb* pcb)
{
    return pcb->mutex;
}

