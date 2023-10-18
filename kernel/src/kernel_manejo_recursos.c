#include "kernel.h"

pthread_mutex_t mutex_blocked;
pthread_mutex_t mutex_recursos;


t_list *lista_recursos;
int *instancias_del_recurso;

//====================================================== WAIT/SIGNAL =====================================================================================
void asignacion_recursos(t_pcb* proceso)
{
    //descubri que era mas facil si lo pasaba directamente por aca
    char* recurso = proceso->recurso_pedido;
    int instancias = 0;

    int indice_pedido = indice_recurso(recurso);

    if (indice_pedido == -1)
    {
        //si el recurso no existe, mando el proceso a exit
        proceso->recurso_pedido = NULL;
        log_error(kernel_logger, "El recurso solicitado no existe\n");
        proceso_en_exit(proceso);
        return;
    }

    //actualizo la cantidad de instancias para el recurso que me pidio el proceso y lo borro de recurso_pedido
    proceso->recurso_pedido = NULL;

    //si o si lo tenia que poner con mutex porque sino habia condicion de carrera
    pthread_mutex_lock(&mutex_recursos);
    instancias = instancias_del_recurso[indice_pedido];
    instancias--;
    instancias_del_recurso[indice_pedido]=instancias;
    pthread_mutex_unlock(&mutex_recursos);

    log_info(kernel_logger,"PID: %d - Wait: %s - Instancias: %d\n",proceso->pid, recurso, instancias); 

    if(instancias < 0){

        /*voy a agarrar la cola del indice del recurso que me piden. Como la lista_recursos es una lista 
        de punteros a otras colas, lo que voy a hacer es buscar dentro de esa lista, el indice del recurso 
        que me pasan por parametro y agarrar la cola del recurso al que nos estamos refiriendo*/
        t_list *cola_bloqueados_recurso = (t_list *)list_get(lista_recursos, indice_pedido);

        //y agregamos a la cola que agarre, el proceso que pidio ese recurso
        list_add(cola_bloqueados_recurso, (void *)proceso);  
        log_info(kernel_logger,"PID: %d - Bloqueado por: %s\n", proceso->pid, recurso);
        deteccion_deadlock(proceso); 
    } 
    else {
        
        /*aca hice como dijo lean porque al final era mas facil. Solamente pongo el nombre del recurso
        que acabo de asignar en mi estructura de t_recurso y le sumo una instancia a la cantidad de
        instancias que tiene el proceso de ese recurso. El truco aca es manejarse con indices porque
        sino termino sumando una instancia siempre al mismo. Si pongo el indice del recurso del que 
        estamos hablando, cambia la cosa*/

        strcpy(proceso->recursos_asignados[indice_pedido].nombre_recurso, recurso);
        proceso->recursos_asignados[indice_pedido].instancias_recurso++;

        //despues vamos a mandar el proceso a execute para que siga su camino
        proceso_en_execute(proceso);
    }
    free(recurso);
}

void liberacion_recursos(t_pcb* proceso)
{
    //voy a robar vilmente lo que hice arriba y lo voy a copiar aca, porque por suerte no estamos en pdep
    char* recurso = proceso->recurso_pedido;
    int instancias = 0;

    int indice_pedido = indice_recurso(recurso);

    if (indice_pedido == -1)
    {
        //si el recurso no existe, mando el proceso a exit
        proceso->recurso_pedido = NULL;
        log_error(kernel_logger, "El recurso solicitado no existe\n");
        proceso_en_exit(proceso);
        return;
    }

    //actualizo la cantidad de instancias para el recurso que me pidio el proceso y lo borro de recurso_pedido
    proceso->recurso_pedido = NULL;

    //si o si lo tenia que poner con mutex porque sino habia condicion de carrera
    pthread_mutex_lock(&mutex_recursos);
    instancias = instancias_del_recurso[indice_pedido];
    instancias++;
    instancias_del_recurso[indice_pedido]=instancias;
    pthread_mutex_unlock(&mutex_recursos);

    log_info(kernel_logger,"PID: %d - Signal: %s - Instancias: %d\n",proceso->pid, recurso, instancias); 

    /*aca vemos que pasa si hay procesos esperando a que ese recurso se libere. Si esta en negativo, es que
    hay un proceso esperando en la cola de bloqueado*/
    if(instancias <= 0){
        
        t_list* cola_bloqueados_recurso = (t_list *)list_get(lista_recursos, indice_pedido);

        /*esta funcion ya la habre hecho como 10 veces en lo que vamos de codigo, no hace falta presentacion
        esta cola se va a desbloquear por FIFO, para no perder la costumbre. Nos llega por parametro la cola
        del recurso y de ahi vamos a sacar nuestro proceso*/
        t_pcb* pcb_desbloqueado = obtener_bloqueado_por_recurso(cola_bloqueados_recurso);

        /*una vez que lo desbloqueamos porque justo se libero el recurso que este proceso estaba buscando,
        vamos a mandar a nuestro amigo a ready porque no se puede mandar solo a exec. Que nuestro plani
        decida si quiere mandarlo a ejecutar, para algo lo cree*/
        pthread_mutex_lock(&mutex_ready);
        meter_en_cola(pcb_desbloqueado, READY, cola_READY);
        pthread_mutex_unlock(&mutex_ready);
    }

    /*ahora voy a tener que hacer lo mismo pero al revez para sacar el recurso. Pero si tiene mas de una
    instancia de ese recurso, entonces significa que no lo voy a tener que borrar del todo porque todavia 
    tiene asignada al menos una instancia del mismo..*/
    int cantidad_instancias_proceso = proceso->recursos_asignados->instancias_recurso;
    cantidad_instancias_proceso--;
    proceso->recursos_asignados->instancias_recurso = cantidad_instancias_proceso;

    /*si ya no le queda ninguna instancia entonces lo volamos, si tiene al menos una instancia, lo dejamos*/
    if(cantidad_instancias_proceso == 0)
    {
        proceso->recursos_asignados->nombre_recurso[0] = '\0';
    }

    //por ultimo mandamos el proceso a exec para que siga su camino
    proceso_en_execute(proceso);
    free(recurso);
}

//==================================================== Accesorios =====================================================
//en un principio iba a ser un bool pero me sirve mas que me diga el indice donde esta el recurso que busco
int indice_recurso (char* recurso_buscado){

    /*buscamos en el array de recursos que tenemos en la config si existe el recurso que llega por parametro
    y si no existe, devolvemos 1 */
    int tamanio = string_array_size(config_valores_kernel.recursos);
    for (int i = 0; i < tamanio; i++)
        if (!strcmp(recurso_buscado, config_valores_kernel.recursos[i]))
        {
            return i;
        }
    return -1;
}

t_pcb* obtener_bloqueado_por_recurso(t_list* cola_recurso)
{
    //saco el primero de la cola que me llega por parametro
    return (t_pcb *)list_remove(cola_recurso, 0);
}

void crear_colas_bloqueo()
{
    /*para crear las colas de bloqueo hay que crear una lista de punteros a estructuras
     de datos t_list, que a su vez van a ser utilizadas como colas de bloqueo para cada recurso.*/
    lista_recursos = list_create();
    instancias_del_recurso = NULL;
    
    //aca voy a guardar en otro char** la cantidad de instancias que tengo para usar [1,2,3]
    char** cant_recursos = config_valores_kernel.instancias_recursos;

    //saco la cantidad de elementos que tengo en mi array de recursos
    int tamanio = string_array_size(config_valores_kernel.recursos);
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

    free_array(cant_recursos);
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
