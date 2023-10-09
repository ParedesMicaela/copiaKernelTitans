#include "kernel.h"

pthread_mutex_t mutex_blocked;
t_list *lista_recursos;
int *instancias_del_recurso;
char **recursos;

//===========================================================================================================================================
void asignacion_recursos(t_pcb* proceso, char* recurso)
{
    int indice_pedido = indice_recurso(recurso);

    if (indice_recurso == -1)
    {
        //si el recurso no existe, mando el proceso a exit
        proceso_en_exit(proceso);
        return;
    }

    //actualizo la cantidad de instancias para el recurso que me pidio el proceso
    int instancias = instancias_del_recurso[indice_pedido];
    instancias--;
    instancias_del_recurso[indice_pedido]=instancias;

    log_info(kernel_logger,"PID: <%d> - Wait: <%s> - Instancias: <%d>",proceso->pid, recurso, instancias); 

    if(instancias < 0){
        //si no hay instancias de ese recurso, tengo que bloquear el proceso

        pthread_mutex_lock(&mutex_new);
        meter_en_cola(proceso, BLOCKED, cola_BLOCKED);
        pthread_mutex_unlock(&mutex_new);

        /*tambien tengo que agregarlo a la cola de bloqueados de ese recurso, entonces voy a agarrar la cola
        del indice del recurso que me piden. Como la lista_recursos es una lista de punteros a otras colas,
        lo que voy a hacer es buscar dentro de esa lista, el indice del recurso que me pasan por parametro
        y agarrar la cola del recurso al que nos estamos refiriendo*/
        t_list *cola_bloqueados_recurso = (t_list *)list_get(lista_recursos, indice_pedido);

        //y agregamos a la cola que agarre, el proceso que pidio ese recurso
        list_add(cola_bloqueados_recurso, (void *)proceso);  
        log_info(kernel_logger,"PID: <%d> - Bloqueado por: %s", proceso->pid, recurso); 
    } 
    else {

        /*importantisimo esto: le pongo duplicate para crear una copia independiente de la cadena,
        lo que garantiza que cada elemento de la lista tenga su propia copia y que modificar una
        copia no afecte a otras. Si agregas directamente el puntero a una cadena a una lista, estarias 
        compartiendo la misma cadena en diferentes partes del programa y esta feo eso.*/
        list_add(proceso->recursos_asignados, (void*)string_duplicate (recurso)); 

        //despues vamos a mandar el proceso a execute para que siga su camino
        proceso_en_execute(proceso);
    }
}

//en un principio iba a ser un bool pero me sirve mas que me diga el indice donde esta el recurso que busco
int indice_recurso (char* recurso_buscado){

    /*buscamos en el array de recursos que tenemos en la config si existe el recurso que llega por parametro
    y si no existe, devolvemos 1 */
    int tamanio = string_array_size(config_valores_kernel.recursos);
    for (int i = 0; i < tamanio; i++)
        if (!strcmp(recurso_buscado, config_valores_kernel.recursos[i]))
            return i;
    return -1;
}

void crear_colas_bloqueo()
{
    /*para crear las colas de bloqueo hay que crear una lista de punteros a estructuras
     de datos t_list, que a su vez van a ser utilizadas como colas de bloqueo para cada recurso.*/
    lista_recursos = list_create();

    instancias_del_recurso = NULL;

    //aca voy a guardar en un char** los nombres de los recursos que tengo para usar [R1,R2,R3]
    recursos = config_valores_kernel.recursos;
    
    //aca voy a guardar en otro char** la cantidad de instancias que tengo para usar [1,2,3]
    char** cant_recursos = config_valores_kernel.instancias_recursos;

    //saco la cantidad de elementos que tengo en mi array de recursos
    int tamanio = string_array_size(recursos);
    
    instancias_del_recurso = malloc(tamanio * sizeof(int));


    //por cada recurso del array de recursos que tengo, voy a hacer una cola de bloqueo
    for (int i = 0; i < tamanio; i++)
    {
        /* en cant_recursos yo me hago un array con las cantidades de recursos unicamente pero esta en un
        array como un string cada numero. Entonces para poder guardar la cantidad de recursos totales que tengo
        de cada recurso en un int* (), necesito pasar cada elemento de cant_recursos a int.*/
        int instancia_en_entero = atoi(cant_recursos[i]);

        //en i=0 el recurso que este en recursos[i] va a tener la cantidad que me marque en cant_recursos[i] :)
        instancias_del_recurso[i] = instancia_en_entero;

        //voy a crear una lista para cada recurso y la voy a guardar dentro de otra lista
        t_list* cola_bloqueo = list_create();
        
        //agrego la cola de bloqueo a la lista de recursos
        list_add(lista_recursos, cola_bloqueo);
    }
    free(instancias_del_recurso);
}














/*
Archivo de configuración con 2 variables con la información inicial de los mismos:
    -La primera llamada RECURSOS
        --listará los nombres de los recursos disponibles en el sistema.
    -La segunda llamada INSTANCIAS_RECURSOS
        --cantidad de instancias de cada recurso del sistema
        --ordenadas de acuerdo a la lista anterior


A la hora de recibir de la CPU un Contexto de Ejecución desalojado por WAIT:
    -el Kernel deberá verificar primero que exista el recurso solicitado
        --si existe restarle 1 a la cantidad de instancias del mismo
        --si el número es menor a 0
            ---el proceso que realizó WAIT se bloqueará en la cola de bloqueados correspondiente al recurso.


A la hora de recibir de la CPU un Contexto de Ejecución desalojado por SIGNAL:
    -el Kernel deberá verificar:
        --primero que exista el recurso solicitado
        --luego que el proceso cuente con una instancia del recurso (solicitada por WAIT)
        --por último sumarle 1 a la cantidad de instancias del mismo.
            ---En caso de que corresponda
                ----desbloquea al primer proceso de la cola de bloqueados de ese recurso.
                ----se devuelve la ejecución al proceso que peticiona el SIGNAL.

Para las operaciones de WAIT y SIGNAL donde: el recurso no exista OR no haya sido solicitado por ese proceso
    -se deberá enviar el proceso a EXIT.

*/