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
t_pcb* crear_pcb(int prioridad, int tam_swap) 
{
    //esto lo ponemos aca para no tener que hacerlo en la funcion iniciar_proceso si total lo vamos a hacer siempre
    t_pcb* pcb = malloc(sizeof(t_pcb)); //nota de Martín: este malloc después se libera en cpu cuando termina el proceso junto al pcb (posible memory leak?)
    //Chquear si el sizeof está tomando correctamente, o lo debugeo printf del sizeof

    //el indice lo vamos a estar modificando cada vez que tengamos que crear un pcb entonces conviene ponerlo como variable global
    //cosa que todos sabemos cuanto vale y no repetimos pid
    sem_wait(&(mutex_pid));
    indice_pid ++;
    sem_post(&(mutex_pid)); 
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
    en memoria del proceso
    t_paquete* paquete = crear_paquete(CREACION_ESTRUCTURAS_MEMORIA);

    //a la memoria solamente le pasamos el pid y el tamanio que va a ocupar en swap, despues se encarga ella
    agregar_entero_a_paquete(paquete,pcb-> pid);
    agregar_entero_a_paquete(paquete,tam_swap);

    enviar_paquete(paquete, socket_memoria);
    log_info(kernel_logger, "Se manda mensaje a memoria para inicializar estructuras del proceso \n");
    eliminar_paquete(paquete);

    int respuesta;
    recv(socket_memoria, &respuesta,sizeof(int),0);

    if (respuesta != 1)
    {
        log_error(kernel_logger, "No se pudieron crear estructuras en memoria");
    }
*/
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
    

    //agregar_entero_a_paquete(paquete, pcb_a_enviar->archivosAbiertos); ///hay que ver como mandamos esto

    enviar_paquete(paquete, socket_cpu_dispatch);
    log_info(kernel_logger, "Se envio el PCB %d a la CPU \n", pcb_a_enviar->pid);
    eliminar_paquete(paquete);
    return;
}

//hay que ver bien esto porque le faltan cosas
 char* recibir_contexto(t_pcb* proceso)
 {
    //esta funcion lo que hace es recibir un paquete, si ese paquete es un pcb lo abre y nos dice el motivo por el cual se devolvio
    char* motivo_de_devolucion = NULL;
    t_paquete* paquete = recibir_paquete(socket_cpu_dispatch);
    void* stream = paquete->buffer->stream;
    int program_counter =-1;
    

    //si lo que recibimos es en efecto un pcb, lo abrimos
	if(paquete->codigo_operacion == PCB)
	{
		//nosotros solamente vamos a sacar el contexto
        program_counter = sacar_entero_de_paquete(&stream);
        AX = sacar_entero_sin_signo_de_paquete(&stream);
        BX = sacar_entero_sin_signo_de_paquete(&stream);
        CX = sacar_entero_sin_signo_de_paquete(&stream);
        DX = sacar_entero_sin_signo_de_paquete(&stream);
        motivo_de_devolucion = sacar_cadena_de_paquete(&stream);
    }
    else{
        log_error(kernel_logger, "Falla al recibir PCB, se cierra el Kernel \n");
        abort();
    }

	//actualizamos el pc y los registros
	proceso->program_counter = program_counter;
	//proceso->registros_cpu = registros;

    log_info(kernel_logger, "Recibi el pcb de la CPU con program counter = %d \n", program_counter);

    //retornamos el motivo de devolucion para que ejecutar se encargue de manejarlo
    return motivo_de_devolucion;
 }

//eliminamos el pcb, sus estructuras, y lo de adentro de esas estructuras
void eliminar_pcb(t_pcb* proceso)
{
    /*free(proceso->pid);
    //free(proceso->pid); ta tirando error al hacer el make, voy a ver que onda
	eliminar_registros_pcb(proceso->registros_cpu);
	free(proceso->prioridad);   
	//free(proceso->estado_pcb); tira error 
	//free(proceso->estado_pcb); acá también tira error con el make también
	//eliminar_archivos_abiertos(proceso->archivosAbiertos);
	//eliminar_mutex(proceso->mutex);
*/
    free(proceso); //intuyo con que al hacerle free van a haber cosas del pcb que vuelan pero me hace ruido 
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
