#include "kernel.h"

pthread_t largo_plazo;
pthread_t corto_plazo;

//semaforo
sem_t mutex_otras_colas;



//===============================================================================================================================

// nos van a decir la prioridad, el archivo de pseudocodigo a ejecutar y el tamanio de memoria swap que va a ejecutar
void iniciar_proceso(char *path, int tam_proceso_swap, int prioridad)
{
    log_info(kernel_logger, "Iniciando proceso.. \n");

    // nos llega de la consola interactiva que tenemos que iniciar un proceso
    // inicializamos el proceso con su pcb respectivo
    t_pcb* pcb = crear_pcb(prioridad, tam_proceso_swap);

    // necesitamos que la memoria tenga el path que nos pasaron para poder leersela al cpu
    enviar_path_a_memoria(path);

	pthread_create(&largo_plazo, NULL, (void* ) planificador_largo_plazo, NULL);
	pthread_create(&corto_plazo, NULL, (void* ) planificador_corto_plazo, NULL);

    pthread_join(largo_plazo,NULL);
    pthread_join(corto_plazo,NULL);

    // en caso de que el grado máximo de multiprogramación lo permita
    //proceso_en_exit(pcb);
}


void consola_finalizar_proceso(int pid){
    
    printf("Entramos a finalizar proceso \n");
    int tam_cola_BLOCKED = list_size(cola_BLOCKED);
    int tam_cola_EXEC= list_size(cola_EXEC);
    int bandera_cola = 0; // Si queda en 1, el pcb esta en blocked; si esta en 2, el pcb esta en exec.

    //BUSQUEDA DE PCB

    //recorremos la cola de bloqueados y buscamos el pid del pcb
    for(int i=0;i<tam_cola_BLOCKED ;i++)
        {
            //encontramos el pcb con el pid que mandamos
            if(pid == ((t_pcb*) list_get(tam_cola_BLOCKED, i))->pid)
            {
                t_pcb* pcb = ((t_pcb*) list_get(cola_BLOCKED, i)); // hacemos esto para llamarlo directamente pcb
                bandera_cola = 1;
                printf("Encontramos pcb en bloqueado\n");            
            }
        }

    //si no, lo buscamos en la cola de ejecutandose
    for(int i=0;i<tam_cola_EXEC ;i++)
        {
            //encontramos el pcb con el pid que mandamos
            if(pid == ((t_pcb*) list_get(tam_cola_BLOCKED, i))->pid)
            {
                t_pcb* pcb = ((t_pcb*) list_get(cola_BLOCKED, i)); // hacemos esto para llamarlo directamente pcb
                bandera_cola = 2; 
                printf("Encontramos pcb ejecutandose \n");            
            }
        }
    
    //CAMBIO DE ESTADO DE PCB
    switch(bandera_cola)
    {
        case 1: cambiar_estado_de_proceso_dado_un_pid(pid, EXIT, cola_BLOCKED, cola_EXIT); break;
        case 2: cambiar_estado_de_proceso_dado_un_pid(pid, EXIT, cola_EXEC, cola_EXIT); break;
        default: printf("Proceso equivocado, no se encontro. Intente nuevamente"); inicializar_consola_interactiva(); break;
    }
}


void cambiar_estado_de_proceso_dado_un_pid(int pid, estado ESTADO_NUEVO, t_list* cola_origen, t_list* cola_destino) // recibimos pid de proceso, estado nuevo y cola origen del pcb y destino para cambiarlo
{   
    //int posicion_pcb; // guarda posicion del proceso, cuando se encuentra
    //estado estado_anterior; // guarda el estado anterior del proceso
    
    //si la cola esta vacia 
    int tam_cola_origen = list_size(cola_origen);
    if(tam_cola_origen == 0) log_info(kernel_logger, "Esta vacia esta cola origen de procesos");
    
    pthread_mutex_lock(&mutex_new);

    //recorremos la cola y buscamos el pid del pcb
    for(int i=0;i<tam_cola_origen ;i++)
        {
            //encontramos el pcb con el pid que mandamos
            if(pid == ((t_pcb*) list_get(cola_origen, i))->pid)
            {
                t_pcb* pcb = ((t_pcb*) list_get(cola_origen, i)); // hacemos esto para llamarlo directamente pcb
                
                //nadie nos rompe los quinotos con el semaforo
                sem_wait(&(mutex_otras_colas));

                //agregar el pcb a la cola destino que le mandamos
                list_add(cola_destino,pcb);

                //FALTA VERIFICAR SI HAY QUE HACER ALGO MAS ANTES DE LIBERAR RECURSOS

                //liberamos recursos del pcb en la cola anterior
                //eliminamos de la lista en la que estaba
                eliminar_pcb(pcb);
                list_remove(cola_origen, i);

                sem_post(&(mutex_otras_colas));            
            }
        }

    pthread_mutex_unlock(&mutex_new);
}

/*

Iniciar proceso // INICIAR_PROCESO [PATH] [SIZE] [PRIORIDAD]
    - Ejecutar un nuevo proceso en base a un archivo dentro del file system de linux
    - Dicho mensaje se encargará de:
        --creación del proceso (PCB)
        --Dejar el mismo en el estado NEW.


Finalizar proceso // FINALIZAR_PROCESO [PID]
    - Es un mensaje
    - Finalizar un proceso que se encuentre dentro del sistema
    - Realiza las mismas operaciones como si el proceso llegara a EXIT por sus caminos habituales
    - (deberá liberar recursos, archivos y memoria).


Detener planificación // DETENER_PLANIFICACION
    - Es un mensaje
    - Pausar la planificación de corto y largo plazo.
    - El proceso que se encuentra en ejecución NO es desalojado
    - Una vez que salga de EXEC se va a pausar su transición al siguiente estado.


Iniciar planificación // INICIAR_PLANIFICACION
    - Es un mensaje
    - en caso que se encuentre pausada:
        --retoma la planificación de corto y largo plazo
    - En caso que no se encuentre pausada
        --se debe ignorar el mensaje.

Modificar grado multiprogramación //  MULTIPROGRAMACION [VALOR]
    - Actualiza del grado de multiprogramación configurado inicialmente por archivo de configuración.
    - En caso que dicho valor sea inferior al actual
        --NO se debe desalojar ni finalizar los procesos.


Listar procesos por estado // PROCESO_ESTADO
    - Es un mensaje
    - Muestra por consola listado de los estados con los procesos que se encuentran dentro de cada uno de ellos.


!!!Se debe tener en cuenta que!!!
    - frente a un fallo en la escritura de un comando en consola
        --el sistema debe permanecer estable sin reacción alguna.

*/
