#include "kernel.h"

//=============================================== Variables Globales ========================================================
t_dictionary_int *diccionario_colas;
t_dictionary_int *diccionario_estados;
// t_dictionary* diccionario_tipo_instrucciones;

t_list *cola_NEW;
t_list *cola_READY;
t_list *cola_BLOCKED;
t_list *cola_EXEC;
t_list *cola_EXIT;

// semáforos en planificación (inserte emoji de calavera)

/*estos los estoy usando en la parte de hilos para los procesos en ready, exec  y exit del corto plazo
para acceder a las listas y sacarles el tamanio o agregar/eliminar procesos*/
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_exec;
pthread_mutex_t mutex_exit;
pthread_mutex_t mutex_colas;
pthread_mutex_t mutex_corriendo;
pthread_cond_t cond_corriendo;

sem_t hay_proceso_nuevo;
sem_t grado_multiprogramacion;
//sem_t sigue_corriendo_corto;
//sem_t sigue_corriendo_largo;
sem_t hay_procesos_ready;
sem_t mutex_pid;

// sem_t dispatchPermitido;
// pthread_mutex_t mutexSocketMemoria;
// pthread_mutex_t mutexSocketFileSystem; los comento porque son terreno inexplorado por ahora
// sem_t semFRead;
// sem_t semFWrite;
// sem_t mutex_colas;




// bool fRead;
// bool fWrite;

int corriendo = 1;

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
    pthread_mutex_init(&mutex_blocked, NULL);
    pthread_mutex_init(&mutex_exit, NULL);
    pthread_mutex_init(&mutex_new, NULL);
    pthread_mutex_init(&mutex_colas, NULL);
    pthread_mutex_init(&mutex_recursos, NULL);
    pthread_mutex_init(&mutex_corriendo , NULL);
    pthread_mutex_init(&cond_corriendo , NULL);

    sem_init(&grado_multiprogramacion, 0, config_valores_kernel.grado_multiprogramacion_ini);
    sem_init(&(hay_proceso_nuevo), 0, 0);
    sem_init(&(hay_procesos_ready), 0, 0);
    //sem_init(&(sigue_corriendo_corto), 0, 0);
    //sem_init(&(sigue_corriendo_largo), 0, 0);
    sem_init(&(mutex_pid), 0, 1);
}

void planificador_largo_plazo()
{
    while (1)
    {

        sem_wait(&hay_proceso_nuevo);
        // si el grado de multiprogramacion lo permite
        sem_wait(&grado_multiprogramacion);

        pthread_mutex_lock(&mutex_corriendo);
        while (corriendo == 0) { // Sea 0
           
            pthread_cond_wait(&cond_corriendo, &mutex_corriendo);
        }
        pthread_mutex_unlock(&mutex_corriendo);

        // elegimos el que va a pasar a ready, o sea el primero porque es FIFO
        t_pcb *proceso_nuevo = obtener_siguiente_new();

        // metemos el proceso en la cola de ready
        pthread_mutex_lock(&mutex_ready);
        meter_en_cola(proceso_nuevo, READY, cola_READY);
        pthread_mutex_unlock(&mutex_ready);

        mostrar_lista_pcb(cola_READY, "READY");

        pthread_mutex_lock(&mutex_corriendo);
        while (corriendo == 0) { // Sea 0
           
            pthread_cond_wait(&cond_corriendo, &mutex_corriendo);
        }
        pthread_mutex_unlock(&mutex_corriendo);

         /*le avisamos al corto plazo que puede empezar a planificar. Aca solamente vamos a poner el proceso
        en la cola de ready pero no vamos a elegir cual va a ejecutar el de corto plazo porque no hacemos eso
        y le estamos robando el trabajo. Solamente vamos a poner el proceso en la cola de ready si el grado de
        multiprogramacion lo permite y despues que se arregle el corto plazo*/
        sem_post(&hay_procesos_ready);
    }

    
}

void planificador_corto_plazo()
{
    crear_colas_bloqueo();

    while (1)
    {
        sem_wait(&hay_procesos_ready);

        pthread_mutex_lock(&mutex_corriendo);
        while (corriendo == 0) {
            pthread_cond_wait(&cond_corriendo, &mutex_corriendo);
        }
        pthread_mutex_unlock(&mutex_corriendo);

        proceso_en_ready();

    }
}

//======================================================== Estados ==================================================================

// aca agarramos el proceso que nos devuelve obtener_siguiente_ready y lo mandamos a ejecutar
void proceso_en_ready()
{

    pthread_mutex_lock(&mutex_corriendo);
    while (corriendo == 0) {
        pthread_cond_wait(&cond_corriendo, &mutex_corriendo);
    }
    pthread_mutex_unlock(&mutex_corriendo);
    
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
    // necesitamos que la memoria tenga el path que nos pasaron para poder leersela al cpu
    enviar_path_a_memoria(proceso_seleccionado->path_proceso);

    // le enviamos el pcb a la cpu para que ejecute y recibimos el pcb resultado de su ejecucion
    enviar_pcb_a_cpu(proceso_seleccionado);

    /*despues la cpu nos va a devolver el contexto en caso de que haya finalizado el proceso
    haya pedido un recurso (wait/signal), por desalojo o por page fault*/
    char *devuelto_por = recibir_contexto(proceso_seleccionado);

    if (string_equals_ignore_case(devuelto_por, "exit"))
    {
        pthread_mutex_lock(&mutex_corriendo);
        while (corriendo == 0) { // Sea 0
           
            pthread_cond_wait(&cond_corriendo, &mutex_corriendo);
        }
        pthread_mutex_unlock(&mutex_corriendo);
        proceso_en_exit(proceso_seleccionado);
    }

    if (string_equals_ignore_case(devuelto_por, "wait"))
    {
        asignacion_recursos(proceso_seleccionado);
    }

    if (string_equals_ignore_case(devuelto_por, "signal"))
    {
        liberacion_recursos(proceso_seleccionado);
    }

    if (string_equals_ignore_case(devuelto_por, "sleep"))
    {
        usleep(proceso_seleccionado->sleep);
    }

    if (string_equals_ignore_case(devuelto_por, "page_fault"))
    {
        //si tenemos page_fault, hay que bloquear el proceso
        pthread_mutex_lock(&mutex_corriendo);
        while (corriendo == 0) { // Sea 0
           
            pthread_cond_wait(&cond_corriendo, &mutex_corriendo);
        }
        pthread_mutex_unlock(&mutex_corriendo);
        proceso_en_blocked(proceso_seleccionado);
    }
    free(devuelto_por);
}

void proceso_en_exit(t_pcb *proceso)
{

    pthread_mutex_lock(&mutex_exec);
    proceso = list_remove(dictionary_int_get(diccionario_colas, EXEC), 0);
    pthread_mutex_unlock(&mutex_exec);

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
    sem_post(&grado_multiprogramacion);
}

void proceso_en_blocked(t_pcb *proceso) //nota: no uso obtener_siguiente_blocked ya que de blocked directamente se va ready, para el page fault tengo que trabajar un poco más esa parte
{
    //el motivo de bloqueo debe ser para page_fault
    if (proceso->motivo_bloqueo == PAGE_FAULT)
    {
    // Movemos el proceso a la cola de BLOCKED
    pthread_mutex_lock(&mutex_blocked);
    meter_en_cola(proceso, BLOCKED, cola_BLOCKED);
    pthread_mutex_unlock(&mutex_blocked);

    log_info(kernel_logger, "PID[%d] bloqueado por page fault\n");
    /*Mover al proceso al estado Bloqueado. Este estado bloqueado será 
    independiente de todos los demás ya que solo afecta al proceso 
    y no compromete recursos compartidos.*/
    atender_page_fault(proceso);
    //liberacion_recursos(proceso); ver bien esto porque me hace ruido
    //entonces al proceso bloqueado le debo liberar los recursos
    //si bloqueo un proceso debo aumentar el grado de multiprogramación
    //sem_post(&grado_multiprogramacion); lo mismo que a liberar recursos


    //en el .4 se menciona que se coloca al proceso en ready después de solucionar el page fault
    /*pthread_mutex_lock(&mutex_ready);
    meter_en_cola(proceso, READY, cola_READY);
    pthread_mutex_unlock(&mutex_ready);*/
    } //más adelante vamos a agregar los otros casos por los que se puede bloquear
}

//======================================================== Algoritmos ==================================================================

t_pcb *obtener_siguiente_new()
{
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

//agarramos el siguiente de la cola de bloqueados y metemos el proceso seleccionado a la cola ready
t_pcb* obtener_siguiente_blocked()
{
    pthread_mutex_lock(&mutex_blocked);
    t_pcb* proceso_seleccionado = list_remove(dictionary_int_get(diccionario_colas, BLOCKED), 0);
    pthread_mutex_unlock(&mutex_blocked);

    //aca ya de una lo mandamos a ready porque sabemos que en el diagrama va directo a ready
    pthread_mutex_lock(&mutex_ready);
    meter_en_cola(proceso_seleccionado, READY, cola_READY);
    pthread_mutex_unlock(&mutex_ready);

    log_info(kernel_logger, "PID[%d] sale de BLOCKED para meterse en READY\n", proceso_seleccionado->pid);

    return proceso_seleccionado;
}

t_pcb *obtener_siguiente_FIFO()
{
    log_info(kernel_logger, "Inicio la planificacion FIFO \n");

    //mostramos los que estan en ready
    mostrar_lista_pcb(cola_READY, "READY");

    /*voy a seleccionar el primer proceso que esta en ready usando esta funcion porque me retorna el proceso
    que le pido y tambien me lo borra. Como FIFO va a ejecutar todo hasta terminar, me biene barbaro*/
    pthread_mutex_lock(&mutex_ready);
    t_pcb *proceso_seleccionado = list_remove(dictionary_int_get(diccionario_colas, READY), 0);
    pthread_mutex_unlock(&mutex_ready);

    log_info(kernel_logger, "PID[%d] sale de READY por planificacion FIFO \n", proceso_seleccionado->pid);
    return proceso_seleccionado;
}

t_pcb* obtener_siguiente_PRIORIDADES()
{
    log_info(kernel_logger, "Inicio la planificacion PRIODIDADES \n");

	pthread_mutex_lock(&mutex_ready);
	t_pcb* proceso_seleccionado = list_remove(dictionary_int_get(diccionario_colas, READY), 0);
	pthread_mutex_unlock(&mutex_ready);

    if (proceso_seleccionado->estado_pcb == EXIT) 
        {
            log_info(kernel_logger, "PID[%d] ha finalizado\n", proceso_seleccionado->pid);
            return proceso_seleccionado; 
        }
    else if (  
        //ante cada entrada a la cola de ready fijarse si tiene mas prioridad, y en ese caso, ejecutarlo
        proceso_seleccionado->estado_pcb == EXEC 
        && 
        proceso_seleccionado->prioridad > list_get_minimum(cola_READY, proceso_seleccionado->prioridad)//no se si seta bien este list_get_minimum
            )
        {
            //cambiar proceso actual de menor priodidad a la cola de ready
            //(por ahora no se si esta bien mandarlo asi nomas, porque este proceso esta desalojado
            //y capaz deberiamos directamente volver a ejecutarlo apenas termine el nuevo)
            proceso_seleccionado->estado_pcb = READY;
            list_add(cola_READY,proceso_seleccionado);

             //ponemos proceso nuevo, con mas prioridad, como el seleccionado
             //revisar si esta bien
             pthread_mutex_lock(&mutex_ready);
             t_pcb* proceso_seleccionado_prioritario = list_remove(list_get_minimum(cola_READY, proceso_seleccionado->prioridad),0);
             pthread_mutex_unlock(&mutex_ready);
             return proceso_seleccionado_prioritario;     
        }   



    //meto lock y unlock del mutex de ready para poder sacar el proceso de la cola de ready con mas priodidad


    //cuando termines vol
}

t_pcb *obtener_siguiente_RR()
{
    int quantum = config_valores_kernel.quantum; // obtiene el quantum de la config del kernel

    log_info(kernel_logger, "Inicio la planificación RR\n");

    // meto lock y unlock del mutex de ready para poder sacar el primer proceso de la cola tranqui
    pthread_mutex_lock(&mutex_ready);
    t_pcb *proceso_seleccionado = list_remove(dictionary_int_get(diccionario_colas, READY), 0);
    pthread_mutex_unlock(&mutex_ready);

    log_info(kernel_logger, "PID[%d] sale de READY por planificación RR", proceso_seleccionado->pid);

    // ahora acá se viene mi truquito

    int tiempo_transcurrido = 0; // arranca en 0 porque todavía no empieza jeje
    // simulo(a.k.a para los simuladores) la ejecución del tiempo mientras se va chequeando el quantum
    while (tiempo_transcurrido < quantum)
    {
        usleep(2); // con esto me estoy librando de la espera activa ya que usleep lo que hace es pausar la ejecución dentro del while

        // si el proceso finaliza durante su ejecución es porque está en exit
        if (proceso_seleccionado->estado_pcb == EXIT)
        {
            log_info(kernel_logger, "PID[%d] ha finalizado durante su quantum de RR\n", proceso_seleccionado->pid);
            return proceso_seleccionado;
        }
        log_info(kernel_logger, "Todavia no termino el quantum\n");
        tiempo_transcurrido++; // aumento el tiempo que pasa en 1 milisegundo

    }
    // ahora contemplo el caso en el que el tiempo que pasa sea igual al quantum, por lo que pasa de nuevo a la cola de READY(en última posición)
    if (tiempo_transcurrido == quantum)
    {
        pthread_mutex_lock(&mutex_ready);
        meter_en_cola(proceso_seleccionado,READY,cola_READY);
        pthread_mutex_unlock(&mutex_ready);

        // ahora reinicio el quantum para el siguiente proceso :) uwu
        proceso_seleccionado->quantum = config_valores_kernel.quantum; // Reinicia el quantum para el siguiente proceso.
        log_info(kernel_logger, "PID[%d] ha agotado su quantum de RR y se mueve a READY\n", proceso_seleccionado->pid);
        
        //hay que enviar por socket interrupt
        t_paquete* paquete = crear_paquete(DESALOJO);
        enviar_paquete(paquete, socket_cpu_interrupt);
        eliminar_paquete(paquete);

        //agarrar proceso para ejecutar
        proceso_seleccionado = obtener_siguiente_FIFO();

        return proceso_seleccionado;

    }

}

//=================================================== Diccionarios y Colas ==================================================================
void inicializar_diccionarios()
{
    diccionario_colas = dictionary_int_create();

    dictionary_int_put(diccionario_colas, NEW, cola_NEW);
    dictionary_int_put(diccionario_colas, READY, cola_READY);
    dictionary_int_put(diccionario_colas, BLOCKED, cola_BLOCKED);
    dictionary_int_put(diccionario_colas, EXEC, cola_EXEC);
    dictionary_int_put(diccionario_colas, EXIT, cola_EXIT);

    diccionario_estados = dictionary_int_create();

    dictionary_int_put(diccionario_estados, NEW, "New");
    dictionary_int_put(diccionario_estados, READY, "Ready");
    dictionary_int_put(diccionario_estados, BLOCKED, "Blocked");
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
    cola_BLOCKED = list_create();
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
    char *string_pid = NULL;
    char *pids = string_new();

    pthread_mutex_lock(&mutex_colas);
    int tam_cola = list_size(cola);
    pthread_mutex_unlock(&mutex_colas);

     if (tam_cola == 0) {
        log_info(kernel_logger, "esta vacia la cola %s", nombre_cola);
     } 
      else { 
        for (int i = 0; i < tam_cola; i++) {
            pthread_mutex_lock(&mutex_colas);

            //Acceso al elemento en el indice i, guardo en pcb local
            t_pcb *pcb = list_get(cola, i);
            string_pid = string_itoa(pcb->pid);

            pthread_mutex_unlock(&mutex_colas);

            // Junto los pids
            string_append(&pids, string_pid);

            // Separo los PIDs con comas
            if (i < tam_cola - 1)
                string_append(&pids, ", ");
        }
         // mostramos la lista con los pids en la cola dada
        log_info(kernel_logger, "Cola %s %s : [%s]\n", nombre_cola, config_valores_kernel.algoritmo_planificacion, pids);
        free(string_pid);
    }
   
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
