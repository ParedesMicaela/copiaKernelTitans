#include "kernel.h"

//=============================================== Variables Globales ========================================================
t_dictionary_int *diccionario_colas;
t_dictionary_int *diccionario_estados;
// t_dictionary* diccionario_tipo_instrucciones;

t_list *cola_NEW;
t_list *cola_READY;
// creo que va a haber 2 colas de bloqueados, dice algo en el enunciado
t_list *cola_BLOCKED;
t_list *cola_EXEC;
t_list *cola_EXIT;

pthread_t thread_ready;
pthread_t thread_exec;
pthread_t thread_blocked;

// semáforos en planificación (inserte emoji de calavera)

/*estos los estoy usando en la parte de hilos para los procesos en ready, exec  y exit del corto plazo
para acceder a las listas y sacarles el tamanio o agregar/eliminar procesos*/
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_exec;
pthread_mutex_t mutex_exit;
pthread_mutex_t mutex_colas;

sem_t hay_proceso_nuevo;
sem_t grado_multiprogramacion;
// sem_t dispatchPermitido;
// pthread_mutex_t mutexSocketMemoria;
// pthread_mutex_t mutexSocketFileSystem; los comento porque son terreno inexplorado por ahora
// sem_t semFRead;
// sem_t semFWrite;
// sem_t mutex_colas;
sem_t hay_procesos_ready;

sem_t mutex_pid;

// bool fRead;
// bool fWrite;

//====================================================== Planificadores ========================================================
void inicializar_planificador()
{
    // creamos todas las colas que vamos a usar
    inicializar_colas();
    log_info(kernel_logger, "Iniciando colas.. \n");

    // creamos los diccionarios donde vamos a meter las distintas colas
    inicializar_diccionarios();
    log_info(kernel_logger, "Iniciando diccionarios.. \n");

    inicializar_semaforos();
    log_info(kernel_logger, "Preparando planificacion.. \n");
}

void inicializar_semaforos()
{
    pthread_mutex_init(&mutex_ready, NULL);
    pthread_mutex_init(&mutex_exec, NULL);
    pthread_mutex_init(&mutex_exit, NULL);
    pthread_mutex_init(&mutex_new, NULL);
    pthread_mutex_init(&mutex_colas, NULL);

    sem_init(&grado_multiprogramacion, 0, 1);
    sem_init(&(hay_proceso_nuevo), 0, 1);
    sem_init(&(hay_procesos_ready), 0, 1);
    sem_init(&(mutex_pid), 0, 1);
}

void planificador_largo_plazo()
{
    while (1)
    {

        sem_wait(&hay_proceso_nuevo);
        // si el grado de multiprogramacion lo permite
        sem_wait(&grado_multiprogramacion);

        // elegimos el que va a pasar a ready, o sea el primero porque es FIFO
        t_pcb *proceso_nuevo = obtener_siguiente_new();

        // metemos el proceso en la cola de ready
        pthread_mutex_lock(&mutex_ready);
        meter_en_cola(proceso_nuevo, READY, cola_READY);
        pthread_mutex_unlock(&mutex_ready);

        mostrar_lista_pcb(cola_READY, "READY");

        /*le avisamos al corto plazo que puede empezar a planificar. Aca solamente vamos a poner el proceso
        en la cola de ready pero no vamos a elegir cual va a ejecutar el de corto plazo porque no hacemos eso
        y le estamos robando el trabajo. Solamente vamos a poner el proceso en la cola de ready si el grado de
        multiprogramacion lo permite y despues que se arregle el corto plazo*/
        sem_post(&hay_procesos_ready);
    }
}

void planificador_corto_plazo()
{
    while (1)
    {
        sem_wait(&hay_procesos_ready);

        proceso_en_ready();
    }
}

//======================================================== Estados ==================================================================
void proceso_en_ready()
{
    // creamos un proceso, que va a ser el elegido por obtener_siguiente_ready
    t_pcb *siguiente_proceso = obtener_siguiente_ready();

    log_info(kernel_logger, "PID[%d] ingresando a EXEC\n", siguiente_proceso->pid);

    // metemos el proceso en la cola de execute
    pthread_mutex_lock(&mutex_exec);
    meter_en_cola(siguiente_proceso, EXEC, cola_EXEC);
    pthread_mutex_unlock(&mutex_exec);

    proceso_en_execute(siguiente_proceso);
}

void proceso_en_execute(t_pcb *proceso_seleccionado)
{
    // le enviamos el pcb a la cpu para que ejecute y recibimos el pcb resultado de su ejecucion
    enviar_pcb_a_cpu(proceso_seleccionado);

    /*despues la cpu nos va a devolver el contexto en caso de que haya finalizado el proceso
    haya pedido un recurso (wait/signal), por desalojo o por page fault. Ahora para probar vamos
    a hacer el caso en que lo haya devuelto por finalizacion, despues agregamos el resto*/
    char *devuelto_por = recibir_contexto(proceso_seleccionado);

    if (string_equals_ignore_case(devuelto_por, "exit"))
    {

        proceso_en_exit(proceso_seleccionado);
    }

    // y por ultimo, en cualquiera de los casos, vamos a sacar de exec al proceso que ya termino de ejecutar
    pthread_mutex_lock(&mutex_exec);
    proceso_seleccionado = list_remove(dictionary_int_get(diccionario_colas, EXEC), 0);
    pthread_mutex_unlock(&mutex_exec);
}

void proceso_en_exit(t_pcb *proceso)
{

    pthread_mutex_lock(&mutex_exit);
    meter_en_cola(proceso, EXIT, cola_EXIT);
    pthread_mutex_unlock(&mutex_exit);

    // sacamos el proceso de la lista de exit
    pthread_mutex_lock(&mutex_exit);
    proceso = list_remove(dictionary_int_get(diccionario_colas, EXIT), 0);
    pthread_mutex_unlock(&mutex_exit);

    log_info(kernel_logger, "[EXIT]Sale de EXIT y Finaliza el  PCB de ID: %d\n", proceso->pid);

    // le mandamos esto a memoria para que destruya las estructuras
    enviar_pcb_a_memoria(proceso, socket_memoria, FINALIZAR_EN_MEMORIA);
    log_info(kernel_logger, "Enviando a memoria liberar estructuras del proceso \n");

    int fin_ok;
    recv(socket_memoria, &fin_ok, sizeof(int), 0);

    if (fin_ok != 1)
    {
        log_error(kernel_logger, "No se pudieron eliminar estructuras en memoria del proceso PID[%d]\n", proceso->pid);
    }
    // si la respuesta que conseguimos de memoria es que se finalice la memoria, le avisamos a la consola que ya finaliza el proceso
    log_info(kernel_logger, "Respuesta memoria de estructuras liberadas del proceso recibida \n");

    eliminar_pcb(proceso);
    // free(proceso);
    sem_post(&grado_multiprogramacion);
}

//======================================================== Algoritmos ==================================================================

t_pcb *obtener_siguiente_new()
{
    // int size = list_size(cola_NEW);   if (size > 0) {}

    pthread_mutex_lock(&mutex_new);
    t_pcb *proceso_seleccionado = list_remove(dictionary_int_get(diccionario_colas, NEW), 0);
    pthread_mutex_unlock(&mutex_new);

    log_info(kernel_logger, "PID[%d] sale de NEW para planificacion \n", proceso_seleccionado->pid);

    return proceso_seleccionado;
}

/*esta funcion la voy a poner para que me haga todo el calculo de que proceso deberia ir primero dependiendo
del algoritmo que estoy usando. Y la pongo aca porque es mas facil solamente poner una linea en la parte de
proceso_en_ready, que hacer todo este calculo alla arriba.*/
t_pcb *obtener_siguiente_ready()
{
    // creamos un proceso para seleccionar
    t_pcb *proceso_seleccionado;

    int tamanio_cola_ready = 0;
    int ejecutando = 0;

    /*necesito saber la cantidad de procesos que estan listos para ejecutar y para eso bloqueo sino capaz
    cuento y al final resulta que entraron 4 procesos mas*/
    pthread_mutex_lock(&mutex_ready);
    tamanio_cola_ready = list_size(cola_READY);
    pthread_mutex_unlock(&mutex_ready);

    // despues vemos cual seria el grado maximo pero supongamos que es esto
    int grado = config_valores_kernel.grado_multiprogramacion_ini;

    /*el grado de multiprogramacion es el que yo tengo que fijarme para saber si puedo admitir mas procesos
    en ready, o no. Entonces para saber eso necesito saber cuantos procesos estan esperando en ready
    y cuantos procesos estan ejecutando justo ahora. Con eso puedo comparar con el grado de multi que tengo
    y ver si podemos meter un proceso mas.*/

    // quiero saber el algoritmo con el que estoy trabajando

    algoritmo algoritmo_elegido = obtener_algoritmo();

    // obtenemos el tamaño de la cola de ejecutando, nuevamente pongo un semaforo
    pthread_mutex_lock(&mutex_exec);
    ejecutando = list_size(cola_EXEC);
    pthread_mutex_unlock(&mutex_exec);

    /*vemos si todavia hay procesos en ready y si el grado de multiprogramacion me permite ejecutar
    los procesos que estoy ejecutando justo ahora con un proceso mas*/
    if (tamanio_cola_ready >= 0 && ejecutando < grado)
    {
        switch (algoritmo_elegido)
        {
        case FIFO:
            proceso_seleccionado = obtener_siguiente_FIFO();
            break;
        case PRIORIDADES:
            proceso_seleccionado = obtener_siguiente_PRIORIDADES();
            break;
        case RR:
            proceso_seleccionado = obtener_siguiente_RR();
            break;
        default:
            break;
        }
    }

    // devolvemos el proceso seleccionado segun el algoritmo que elegimos
    return proceso_seleccionado;
}

algoritmo obtener_algoritmo()
{

    algoritmo switcher;
    char *algoritmo_actual = NULL;
    algoritmo_actual = config_valores_kernel.algoritmo_planificacion;

    // FIFO
    if (strcmp(algoritmo_actual, "FIFO") == 0)
    {
        switcher = FIFO;
        log_info(kernel_logger, "El algoritmo de planificacion elegido es FIFO \n");
    }
    // PRIORIDADES
    if (strcmp(algoritmo_actual, "PRIORIDADES") == 0)
    {
        switcher = PRIORIDADES;
        log_info(kernel_logger, "El algoritmo de planificacion elegido es PRIORIDADES \n");
    }
    // RR
    if (strcmp(algoritmo_actual, "RR") == 0)
    {
        switcher = RR;
        log_info(kernel_logger, "El algoritmo de planificacion elegido es RR \n");
    }
    return switcher;
}

t_pcb *obtener_siguiente_FIFO()
{
    log_info(kernel_logger, "Inicio la planificacion FIFO \n");

    // mostramos los que estan en ready
    mostrar_lista_pcb(cola_READY, "READY");

    /*voy a seleccionar el primer proceso que esta en ready usando esta funcion porque me retorna el proceso
    que le pido y tambien me lo borra. Como FIFO va a ejecutar todo hasta terminar, me biene barbaro*/
    pthread_mutex_lock(&mutex_ready);
    t_pcb *proceso_seleccionado = list_remove(dictionary_int_get(diccionario_colas, READY), 0);
    pthread_mutex_unlock(&mutex_ready);

    log_info(kernel_logger, "PID[%d] sale de READY por planificacion FIFO \n", proceso_seleccionado->pid);
    return proceso_seleccionado;
}

t_pcb *obtener_siguiente_PRIORIDADES()
{
    /// recordar que es con desalojo
    printf("<3");
    // return proceso_seleccionado;
}

t_pcb *obtener_siguiente_RR() // tener en cuenta como implementar con quantum
{
    printf("<3");
    // return proceso_seleccionado;
}

//=================================================== Diccionarios y Colas ==================================================================
void inicializar_diccionarios()
{
    diccionario_colas = dictionary_int_create();

    dictionary_int_put(diccionario_colas, NEW, cola_NEW);
    dictionary_int_put(diccionario_colas, READY, cola_READY);
    // dictionary_int_put(diccionario_colas, BLOCKED, cola_BLOCKED);
    dictionary_int_put(diccionario_colas, EXEC, cola_EXEC);
    dictionary_int_put(diccionario_colas, EXIT, cola_EXIT);

    diccionario_estados = dictionary_int_create();

    dictionary_int_put(diccionario_estados, NEW, "New");
    dictionary_int_put(diccionario_estados, READY, "Ready");
    // dictionary_int_put(diccionario_estados, BLOCKED, "Blocked");
    dictionary_int_put(diccionario_estados, EXEC, "Exec");
    dictionary_int_put(diccionario_estados, EXIT, "Exit");

    // diccionario_tipo_instrucciones = dictionary_create();

    // dictionary_put(diccionario_tipo_instrucciones, "SET", (void*)SET);
    // dictionary_put(diccionario_tipo_instrucciones, "EXIT", (void*)EXIT);
}

void inicializar_colas()
{
    cola_NEW = list_create();
    cola_READY = list_create();
    // cola_BLOCKED = list_create();
    cola_EXEC = list_create();
    cola_EXIT = list_create();
}

// vamos a usar esta funcion cada vez que el proceso cambie de estado
void meter_en_cola(t_pcb *pcb, estado ESTADO, t_list *cola)
{
    /*creamos una cola con el estado en el que esta el proceso usando la funcion int_get
    pasandole el estado del proceso (key) nos va a devolver la cola en la que esta*/

    // recorremos la cola y buscamos el pid del pcb
    for (int i = 0; i < list_size(cola); i++)
    {
        if (pcb->pid == ((t_pcb *)list_get(cola, i))->pid)
        {
            pthread_mutex_lock(&mutex_colas);
            /*y cuando los encontramos lo vamos a sacar porque va a cambiar de estado entonces ya
            no lo quiero en esa cola*/
            list_remove(cola, i); // yendome del indice maximo de la lista
            pthread_mutex_unlock(&mutex_colas);
        }
    }

    // el estado viejo va a ser el estado original en que estaba el pcb
    estado estado_viejo = pcb->estado_pcb;

    // nuestro nuevo estado va a ser el estado al cual queremos cambiarlo
    pcb->estado_pcb = ESTADO;

    // finalmente lo agregamos a la cola de nuestro nuevo estado
    pthread_mutex_lock(&mutex_colas);
    // list_add(dictionary_int_get(diccionario_colas, ESTADO), pcb);
    list_add(cola, pcb);
    log_info(kernel_logger, "PID: %d - Estado Anterior: %s - Estado Actual %s\n", pcb->pid, dictionary_int_get(diccionario_estados, estado_viejo), dictionary_int_get(diccionario_estados, ESTADO));
    pthread_mutex_unlock(&mutex_colas);
}

// esta funcion es para que nos muestre los pcb que estan en una cola, medio accesorio pero sirve
void mostrar_lista_pcb(t_list *cola, char *nombre_cola)
{
    int tam_cola = list_size(cola);

    if (tam_cola == 0)
    {
        log_info(kernel_logger, "esta vacia la cola %s", nombre_cola);
    }
    // creamos un string vacio llamado pid y recorremos la cola que le pasamos por parametro

    char *pids = string_new();
    for (int i = 0; i < list_size(cola); i++)
    {

        pthread_mutex_lock(&mutex_colas);

        // creamos el string_pid donde vamos a poner el pid de cada proceso que se va leyendo de la cola
        char *string_pid = string_itoa(((t_pcb *)list_get(cola, i))->pid);

        pthread_mutex_unlock(&mutex_colas);

        // unimos cada valor almacenado en string_pid en el char de pids
        string_append(&pids, string_pid);

        // separamos todos los pids con una coma
        if (i < list_size(cola) - 1)
            string_append(&pids, ", ");
    }

    // mostramos la lista con los pids en la cola dada

    log_info(kernel_logger, "Cola %s %s : [%s]\n", nombre_cola, config_valores_kernel.algoritmo_planificacion, pids);
    free(pids);
}

/*
Cuando se reciba un mensaje de CPU con motivo de finalizar el proceso
    se deberá pasar al mismo al estado EXIT
    liberar todos los recursos que tenga asignados
    dar aviso al módulo Memoria para que éste libere sus estructuras.
*/

// corto plazo
/*

Una vez seleccionado el siguiente proceso a ejecutar:
    -se lo transicionará al estado EXEC
    -se enviará su Contexto de Ejecución al CPU a través del puerto de dispatch
    -queda a la espera de recibir:
        --contexto actualizado después de la ejecución
        --un motivo de desalojo por el cual fue desplazado a manejar.


En caso que el algoritmo requiera desalojar al proceso en ejecución
   - enviar interrupción a través de la conexión de interrupt para forzar el desalojo del mismo.


Al recibir el Contexto de Ejecución del proceso en ejecución
    -en caso de que el motivo de desalojo implique replanificar
        --se seleccionará el siguiente proceso a ejecutar según indique el algoritmo.
            ---Durante este período la CPU se quedará esperando el nuevo contexto (es de esta parte o de todo el corto plazo???)
*/
