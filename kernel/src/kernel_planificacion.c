#include "kernel.h"

//=============================================== Variables Globales ========================================================
t_dictionary_int *diccionario_colas;
t_dictionary_int *diccionario_estados;

t_list *cola_NEW;
t_list *cola_READY;
t_list *cola_BLOCKED;
t_list *cola_EXEC;
t_list *cola_EXIT;

pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_exec;
pthread_mutex_t mutex_exit;
pthread_mutex_t mutex_colas;
pthread_mutex_t mutex_corriendo;
pthread_mutex_t mutex_corriendo_pf;
pthread_mutex_t mutex_contexto;
pthread_mutex_t mutex_cpu;
pthread_cond_t cond_corriendo = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_corriendo_pf = PTHREAD_COND_INITIALIZER;


sem_t hay_proceso_nuevo;
sem_t grado_multiprogramacion;
sem_t hay_procesos_ready;
sem_t mutex_pid;

char* motivo_de_devolucion;

int pf_listo;
int corriendo_pf = 1;
int corriendo = 1;
int id_evento_cpu;
static t_pcb* comparar_prioridad(t_pcb* proceso1, t_pcb* proceso2);
static void a_mimir(t_pcb* proceso);
static void atender_round_robin(int* evento_para_interrupt);
static void tratamiento_archivos(t_pcb* proceso);
static void aumentar_evento_cpu();
//====================================================== Planificadores ========================================================
void inicializar_planificador()
{
    id_evento_cpu = 0;

    inicializar_colas();

    inicializar_diccionarios();

    inicializar_semaforos();
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
    pthread_mutex_init(&mutex_contexto , NULL);
    pthread_mutex_init(&mutex_cpu , NULL);
    pthread_mutex_init(&mutex_corriendo_pf , NULL);


    sem_init(&grado_multiprogramacion, 0, config_valores_kernel.grado_multiprogramacion_ini);
    sem_init(&(hay_proceso_nuevo), 0, 0);
    sem_init(&(hay_procesos_ready), 0, 0);
    sem_init(&(mutex_pid), 0, 1);
}

void planificador_largo_plazo()
{
    while (1)
    {
        sem_wait(&hay_proceso_nuevo);
        // si el grado de multiprogramacion lo permite
        sem_wait(&grado_multiprogramacion);

        detener_planificacion();
        // elegimos el que va a pasar a ready, o sea el primero porque es FIFO
        t_pcb *proceso_nuevo = obtener_siguiente_new();

        // metemos el proceso en la cola de ready
        pthread_mutex_lock(&mutex_ready);
        meter_en_cola(proceso_nuevo, READY, cola_READY);
        mostrar_lista_pcb(cola_READY,"READY");
        pthread_mutex_unlock(&mutex_ready);

        sem_post(&hay_procesos_ready);
    }

    
}

void planificador_corto_plazo()
{
    crear_colas_bloqueo();

    while (1)
    {
        sem_wait(&hay_procesos_ready);

        detener_planificacion();
 
        proceso_en_ready();

    }
}

//======================================================== Estados ==================================================================

void proceso_en_ready()
{
    t_pcb *siguiente_proceso = obtener_siguiente_ready();

    // metemos el proceso en la cola de execute
    pthread_mutex_lock(&mutex_exec);
    meter_en_cola(siguiente_proceso, EXEC, cola_EXEC);
    pthread_mutex_unlock(&mutex_exec);

    proceso_en_execute(siguiente_proceso);
}

void proceso_en_execute(t_pcb *proceso_seleccionado)
{
    pf_listo = 1;

    // La CPU despues nos dice pq regresa
    enviar_pcb_a_cpu(proceso_seleccionado);

    if(strcmp(config_valores_kernel.algoritmo_planificacion, "RR") == 0)
    {
        aumentar_evento_cpu();

        pthread_t hilo_round_robin;

        int evento_para_interrupt = id_evento_cpu;
        
        pthread_create(&hilo_round_robin, NULL, (void*)atender_round_robin, (void*)&evento_para_interrupt);
        pthread_detach(hilo_round_robin);

        log_info(kernel_logger, "PID[%d] ha agotado su quantum de RR y se mueve a READY\n", proceso_seleccionado->pid);
    }


    //La CPU nos dice pq finalizo
    pthread_mutex_lock(&mutex_contexto);
    char *devuelto_por = string_duplicate(recibir_contexto(proceso_seleccionado));
    pthread_mutex_unlock(&mutex_contexto);

    aumentar_evento_cpu();

    // Observamos los motivos de devolucion

     if (string_equals_ignore_case(devuelto_por, "f_open"))
    {
        atender_peticiones_al_fs(proceso_seleccionado);
    }
    if (string_equals_ignore_case(devuelto_por, "f_close"))
    {
        atender_peticiones_al_fs(proceso_seleccionado);
    }
    if (string_equals_ignore_case(devuelto_por, "f_seek"))
    {
        atender_peticiones_al_fs(proceso_seleccionado);
    }
    if (string_equals_ignore_case(devuelto_por, "f_read"))
    {
        atender_peticiones_al_fs(proceso_seleccionado);
    }
    if (string_equals_ignore_case(devuelto_por, "f_write"))
    {
        atender_peticiones_al_fs(proceso_seleccionado);
    }
    if (string_equals_ignore_case(devuelto_por, "f_truncate"))
    {
        atender_peticiones_al_fs(proceso_seleccionado);
    }

    if (string_equals_ignore_case(devuelto_por, "sleep"))
    {
        // Lo mandamos a dormir
        a_mimir(proceso_seleccionado);
    }

     if (string_equals_ignore_case(devuelto_por, "page_fault"))
    {
        a_mimir(proceso_seleccionado);
    }

    if (string_equals_ignore_case(devuelto_por, "exit"))
    {
        free(proceso_seleccionado->motivo_bloqueo);
        proceso_seleccionado->motivo_bloqueo = NULL;

        proceso_en_exit(proceso_seleccionado);
    }

    if (string_equals_ignore_case(devuelto_por, "wait"))
    {
        free(proceso_seleccionado->motivo_bloqueo);
        proceso_seleccionado->motivo_bloqueo = NULL;

        asignacion_recursos(proceso_seleccionado);
    }

    if (string_equals_ignore_case(devuelto_por, "signal"))
    {
        free(proceso_seleccionado->motivo_bloqueo);
        proceso_seleccionado->motivo_bloqueo = NULL;

        liberacion_recursos(proceso_seleccionado);
    }

     if (string_equals_ignore_case(devuelto_por, "desalojo"))
    {   
        free(proceso_seleccionado->motivo_bloqueo);
        proceso_seleccionado->motivo_bloqueo = NULL;

        // Lo agregamos nuevamente a la cola de Ready
        pthread_mutex_lock(&mutex_ready);
        meter_en_cola(proceso_seleccionado, READY, cola_READY);
        mostrar_lista_pcb(cola_READY,"READY");
        pthread_mutex_unlock(&mutex_ready);

        proceso_en_ready();
    }
    
    if (string_equals_ignore_case(devuelto_por, "finalizacion"))
    {
        free(proceso_seleccionado->motivo_bloqueo);
        proceso_seleccionado->motivo_bloqueo = NULL;

        proceso_en_exit(proceso_seleccionado);
    }

    free(devuelto_por);
}

static void atender_round_robin(int* evento_para_interrupt) {
    
    int local_evento_interrupt = *evento_para_interrupt;
    
    usleep(1000 * config_valores_kernel.quantum);

    pthread_mutex_lock(&mutex_cpu);
    if (local_evento_interrupt == id_evento_cpu) {
        t_paquete *paquete = crear_paquete(DESALOJO);
        agregar_entero_a_paquete(paquete, 1);
        enviar_paquete(paquete, socket_cpu_interrupt);
        eliminar_paquete(paquete);
    }
    pthread_mutex_unlock(&mutex_cpu);
}

static void tratamiento_archivos(t_pcb* proceso) {

    pthread_t tratamiento_fs;
    pthread_create(&tratamiento_fs, NULL, (void *)atender_peticiones_al_fs, (void *)proceso);
    pthread_detach(tratamiento_fs);
}

static void a_mimir(t_pcb* proceso){

    log_info(kernel_logger, "PID[%d] bloqueado por %s\n", proceso->pid, proceso->motivo_bloqueo);

    //motivo_de_devolucion = string_duplicate(proceso->motivo_bloqueo);

    //Desalojamos el proceso
    pthread_mutex_lock(&mutex_exec);
    list_remove_element(dictionary_int_get(diccionario_colas, EXEC), proceso);
    pthread_mutex_unlock(&mutex_exec);

    //Movemos el proceso a la cola de BLOCKED
    pthread_mutex_lock(&mutex_blocked);
    meter_en_cola(proceso, BLOCKED, cola_BLOCKED);
    pthread_mutex_unlock(&mutex_blocked);

    detener_pf();

    if(pf_listo == 1)
    {

        if (string_equals_ignore_case(proceso->motivo_bloqueo, "sleep")){

            pthread_t pcb_en_sleep;
            if (!pthread_create(&pcb_en_sleep, NULL, (void *)proceso_en_sleep, (void *)proceso)){
                pthread_detach(pcb_en_sleep);
            } else {
                log_error(kernel_logger,"Error en la creacion de hilo \n");
                abort();
            }  

        } else if (string_equals_ignore_case(proceso->motivo_bloqueo, "page_fault")){
        
            pthread_t pcb_page_fault;
            if (!pthread_create(&pcb_page_fault, NULL, (void *)proceso_en_page_fault, (void *)proceso)){
                pthread_detach(pcb_page_fault);
            } else {
                log_error(kernel_logger,"Error en la creacion de hilo \n");
                abort();
            }

        }
        free(proceso->motivo_bloqueo);
        proceso->motivo_bloqueo = NULL;
    }
}
void proceso_en_exit(t_pcb *proceso)
{
    algoritmo algoritmo_elegido = obtener_algoritmo();
    
    if(algoritmo_elegido != RR) {
        //obtenemos el proceso de execute
        pthread_mutex_lock(&mutex_exec);
        list_remove_element(dictionary_int_get(diccionario_colas, EXEC), proceso);
        pthread_mutex_unlock(&mutex_exec);
    } 

    if(proceso->estado_pcb == BLOCKED)
    {
        pthread_mutex_lock(&mutex_blocked);
        list_remove_element(dictionary_int_get(diccionario_colas, BLOCKED), proceso);
        pthread_mutex_unlock(&mutex_blocked);
    }

    //lo metemos en exit
    pthread_mutex_lock(&mutex_exit);
    meter_en_cola(proceso, EXIT, cola_EXIT);
    pthread_mutex_unlock(&mutex_exit);

    //detener_planificacion();

    // sacamos el proceso de la lista de exit
    pthread_mutex_lock(&mutex_exit);
    list_remove_element(dictionary_int_get(diccionario_colas, EXIT), proceso);
    pthread_mutex_unlock(&mutex_exit);

    // le mandamos esto a memoria para que destruya las estructuras
    enviar_pcb_a_memoria(proceso, socket_memoria, FINALIZAR_EN_MEMORIA);

    int fin_ok = 0;
    recv(socket_memoria, &fin_ok, sizeof(int), 0);

    if (fin_ok != 1)
    {
        log_error(kernel_logger, "No se pudieron eliminar estructuras en memoria del proceso PID[%d]\n", proceso->pid);
    }else
    {
        log_info(kernel_logger, "Finaliza el proceso %d - Motivo: SUCCESS\n", proceso->pid);
    }
    
    eliminar_pcb(proceso);
    if(list_size(cola_READY) > 0)
        {
            proceso_en_ready();
        }
    sem_post(&grado_multiprogramacion);
}

void proceso_en_sleep(t_pcb *proceso) 
{
    if(list_size(cola_EXEC) == 0 && list_size(cola_READY) > 0)
    {
        proceso_en_ready();
    }
    sleep(proceso->sleep);
    if(list_size(cola_EXEC) == 0 && list_size(cola_READY) > 0)
    {
        proceso_en_ready();
    }
    pthread_mutex_lock(&mutex_ready);
    meter_en_cola(proceso, READY, cola_READY);
    pthread_mutex_unlock(&mutex_ready);
    
    if(list_size(cola_EXEC) == 0 && list_size(cola_READY) > 0)
    {
        proceso_en_ready();
    }
}

void proceso_en_page_fault(t_pcb* proceso){

    if(pf_listo == 1) {

        log_info(kernel_logger, "Page Fault PID: %d - Pagina: %d", proceso->pid, proceso->pagina_pedida); 

        atender_page_fault(proceso);

        //una vez se atienda, el proceso vuelve a ready
        //obtener_siguiente_blocked(proceso);
        pthread_mutex_lock(&mutex_ready);
        meter_en_cola(proceso, READY, cola_READY);
        pthread_mutex_unlock(&mutex_ready);

        if(list_size(cola_EXEC) == 0 && list_size(cola_READY) > 0)
        {
            proceso_en_ready();
        }
    }
}

void meter_en_ready(t_pcb *proceso)
{
    pthread_mutex_lock(&mutex_exec);
    list_remove_element(dictionary_int_get(diccionario_colas, EXEC), proceso);
    pthread_mutex_unlock(&mutex_exec);

    pthread_mutex_lock(&mutex_ready);
    meter_en_cola(proceso, READY, cola_READY);
    mostrar_lista_pcb(cola_READY,"READY");
    pthread_mutex_unlock(&mutex_ready);
}

//======================================================== Algoritmos ==================================================================

t_pcb *obtener_siguiente_new()
{
    pthread_mutex_lock(&mutex_new);
    t_pcb *proceso_seleccionado = list_remove(dictionary_int_get(diccionario_colas, NEW), 0);
    pthread_mutex_unlock(&mutex_new);

    return proceso_seleccionado;
}


t_pcb *obtener_siguiente_ready()
{
    t_pcb *proceso_seleccionado;

    int tamanio_cola_ready;
    int tamanio_cola_exec;

    //obtenemos el tamanio de la cola ready
    pthread_mutex_lock(&mutex_ready);
    tamanio_cola_ready = list_size(cola_READY);
    pthread_mutex_unlock(&mutex_ready);

    int grado = config_valores_kernel.grado_multiprogramacion_ini;

    //obtenemos el algoritmo
    algoritmo algoritmo_elegido = obtener_algoritmo();

    //obtenemos el tamaño de la cola de ejecutando
    pthread_mutex_lock(&mutex_exec);
    tamanio_cola_exec = list_size(cola_EXEC);
    pthread_mutex_unlock(&mutex_exec);

    /*si todavia hay procesos en ready y si el grado de multiprogramacion me permite ejecutar
    los procesos que estoy ejecutando*/
    if (tamanio_cola_ready >= 0 && tamanio_cola_exec < grado)
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

    //segun el algoritmo
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
    }
    // PRIORIDADES
    if (strcmp(algoritmo_actual, "PRIORIDADES") == 0)
    {
        switcher = PRIORIDADES;
    }
    // RR
    if (strcmp(algoritmo_actual, "RR") == 0)
    {
        switcher = RR;
    }
    return switcher;
}

//agarramos el siguiente de la cola de bloqueados y metemos el proceso seleccionado a la cola ready
void obtener_siguiente_blocked(t_pcb* proceso)
{
    detener_planificacion();

    //sacamos el primero de la cola de blocked
    pthread_mutex_lock(&mutex_blocked);
    list_remove_element(dictionary_int_get(diccionario_colas, BLOCKED), proceso);
    pthread_mutex_unlock(&mutex_blocked);

    //aca ya de una lo mandamos a ready porque sabemos que en el diagrama va directo a ready
    pthread_mutex_lock(&mutex_ready);
    meter_en_cola(proceso, READY, cola_READY);
    mostrar_lista_pcb(cola_READY,"READY");
    pthread_mutex_unlock(&mutex_ready);

}

t_pcb *obtener_siguiente_FIFO()
{
    //FIRST IN
    pthread_mutex_lock(&mutex_ready);
    t_pcb *proceso_seleccionado = list_remove(dictionary_int_get(diccionario_colas, READY), 0);
    pthread_mutex_unlock(&mutex_ready);

    //FIRST OUT
    return proceso_seleccionado;
}

static t_pcb* comparar_prioridad(t_pcb* proceso1, t_pcb* proceso2)
{
    if (proceso1 == NULL) {
        return proceso2;
    } else if (proceso2 == NULL) {
        return proceso1;
    } else if (proceso1->prioridad < proceso2->prioridad) {
        return proceso1;
    } else if (proceso1->prioridad > proceso2->prioridad) {
        return proceso2;
    } else {
        // En caso de empate, devuelve el primero de la lista
        return proceso1;
    }
}

t_pcb *obtener_siguiente_PRIORIDADES()
{

    // Nos fijamos si hay procesos
    pthread_mutex_lock(&mutex_exec);
    int tam_cola_execute = list_size(cola_EXEC);
    pthread_mutex_unlock(&mutex_exec);

    // Obtenemos el proceso de mayor prioridad
    pthread_mutex_lock(&mutex_ready);
    t_pcb *proceso_mayor_prioridad = list_fold(dictionary_int_get(diccionario_colas, READY), NULL, (void *)comparar_prioridad);
    pthread_mutex_unlock(&mutex_ready);

    log_info(kernel_logger, "PID[%d] es el de mas prioridad\n", proceso_mayor_prioridad->pid);

    // Desalojar el proceso actual (si lo hay)
    if (tam_cola_execute > 0)
    {

        // Obtengo el proceso que esta ejecutando
        pthread_mutex_lock(&mutex_exec);
        t_pcb *proceso_ejecutando = list_get(dictionary_int_get(diccionario_colas, EXEC), 0);
        pthread_mutex_unlock(&mutex_exec);

        if (proceso_mayor_prioridad->prioridad < proceso_ejecutando->prioridad)
        {

            // Desalojamos
            pthread_mutex_lock(&mutex_exec);
            list_remove_element(dictionary_int_get(diccionario_colas, EXEC), proceso_ejecutando);
            pthread_mutex_unlock(&mutex_exec);

            // Enviamos el Desalojo a la CPU
            t_paquete *paquete = crear_paquete(DESALOJO);
            agregar_entero_a_paquete(paquete, 1);
            enviar_paquete(paquete, socket_cpu_interrupt);
            eliminar_paquete(paquete);

            // Eliminar el proceso de mayor prioridad de la cola de Ready
            pthread_mutex_lock(&mutex_ready);
            list_remove_element(dictionary_int_get(diccionario_colas, READY), (void*) proceso_mayor_prioridad);
            pthread_mutex_unlock(&mutex_ready);

            // Devolver el proceso de mayor prioridad para la ejecución
            return proceso_mayor_prioridad;
        }
        else
        {
            log_info(kernel_logger, "Sigue ejecutando el mismo proceso \n");
            return proceso_ejecutando;
        }
    }
    else
    {
        log_info(kernel_logger, "No hay otro proceso ejecutando \n");
        // Eliminar el proceso de mayor prioridad de la cola de Ready
        pthread_mutex_lock(&mutex_ready);
        list_remove_element(dictionary_int_get(diccionario_colas, READY), (void*) proceso_mayor_prioridad);
        pthread_mutex_unlock(&mutex_ready);
        return proceso_mayor_prioridad;
    }
}

t_pcb *obtener_siguiente_RR()
{
    //Como FIFO
    pthread_mutex_lock(&mutex_ready);
    t_pcb *proceso_seleccionado = list_remove(dictionary_int_get(diccionario_colas, READY), 0);
    pthread_mutex_unlock(&mutex_ready);

    return proceso_seleccionado;
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
}

void inicializar_colas()
{
    cola_NEW = list_create();
    cola_READY = list_create();
    cola_BLOCKED = list_create();
    cola_EXEC = list_create();
    cola_EXIT = list_create();
}

void meter_en_cola(t_pcb *pcb, estado ESTADO, t_list *cola)
{
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

    if(estado_viejo != NEW)
    {
        list_remove_element(dictionary_int_get(diccionario_colas, estado_viejo), pcb);
    }

    // nuestro nuevo estado va a ser el estado al cual queremos cambiarlo
    pcb->estado_pcb = ESTADO;

    // Marcamos cambio
    pthread_mutex_lock(&mutex_colas);
    list_add(cola, pcb);
    log_info(kernel_logger, "PID: %d - Estado Anterior: %s - Estado Actual %s\n", pcb->pid, (char*)dictionary_int_get(diccionario_estados, estado_viejo), (char*)dictionary_int_get(diccionario_estados, ESTADO));
    pthread_mutex_unlock(&mutex_colas);
}

// esta funcion es para que nos muestre los pcb que estan en una cola, medio accesorio pero sirve
void mostrar_lista_pcb(t_list *cola, char *nombre_cola)
{
    char *string_pid = NULL;
    char *pids = NULL;

    pthread_mutex_lock(&mutex_colas);
    int tam_cola = list_size(cola);
    pthread_mutex_unlock(&mutex_colas);

     if (tam_cola == 0) {
        log_info(kernel_logger, "esta vacia la cola %s", nombre_cola);
     } 
      else { 
        pids = string_new();
        for (int i = 0; i < tam_cola; i++) {
            pthread_mutex_lock(&mutex_colas);

            //Acceso al elemento en el indice i, guardo en pcb local
            t_pcb *pcb = list_get(cola, i);
            string_pid = string_itoa(pcb->pid);

            pthread_mutex_unlock(&mutex_colas);

            // Junto los pids
            string_append(&pids, string_pid);

            free(string_pid);

            // Separo los PIDs con comas
            if (i < tam_cola - 1)
                string_append(&pids, ", ");
        }
         // mostramos la lista con los pids en la cola 
        log_info(kernel_logger, "Cola %s %s : [%s]\n", nombre_cola, config_valores_kernel.algoritmo_planificacion, pids);
    }
    if (pids != NULL) {
        free(pids);  // Free pids if it was allocated
    }
}

void detener_planificacion () {
          pthread_mutex_lock(&mutex_corriendo);
        while (corriendo == 0) { // Mientras no se detenga
           
            pthread_cond_wait(&cond_corriendo, &mutex_corriendo);
        }
        pthread_mutex_unlock(&mutex_corriendo);
}

void detener_pf() {
    pthread_mutex_lock(&mutex_corriendo_pf);
    while (corriendo_pf == 0) { // Mientras no se detenga
           
        pthread_cond_wait(&cond_corriendo_pf, &mutex_corriendo_pf);
    }
    pthread_mutex_unlock(&mutex_corriendo_pf);
}

static void aumentar_evento_cpu() {
    pthread_mutex_lock(&mutex_cpu);
    id_evento_cpu++;
    pthread_mutex_unlock(&mutex_cpu);

}