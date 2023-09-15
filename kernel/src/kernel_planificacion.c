// largo plazo

/*

Generarse la estructura PCB y asignar este PCB al estado NEW.


En caso de que el grado máximo de multiprogramación lo permita
    los procesos pasarán al estado READY
    enviando un mensaje al módulo Memoria para que inicialice sus estructuras necesarias.

La salida de NEW será mediante el algoritmo FIFO.


Cuando se reciba un mensaje de CPU con motivo de finalizar el proceso
    se deberá pasar al mismo al estado EXIT
    liberar todos los recursos que tenga asignados
    dar aviso al módulo Memoria para que éste libere sus estructuras.

*/

// corto plazo
/*

Los procesos que estén en estado READY serán planificados mediante uno de los siguientes algoritmos:
    -FIFO
    -Round Robin
    -Prioridades (con desalojo)


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

#include "kernel.h"

//=============================================== Variables Globales ========================================================
t_dictionary* diccionario_colas;
t_dictionary* diccionario_estados;

t_list* cola_NEW;
t_list* cola_READY;
//creo que va a haber 2 colas de bloqueados, dice algo en el enunciado
t_list* cola_BLOCKED;
t_list* cola_EXEC;
t_list* cola_EXIT;

pthread_t thread_ready;
pthread_t thread_exec;
pthread_t thread_blocked;

//semáforos en planificación (inserte emoji de calavera)

/*estos los estoy usando en la parte de hilos para los procesos en ready, exec  y exit del corto plazo
para acceder a las listas y sacarles el tamanio o agregar/eliminar procesos*/
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_exec;
pthread_mutex_t mutex_exit;


sem_t gradoMultiprogramacion;
sem_t dispatchPermitido;
//pthread_mutex_t mutexSocketMemoria; 
//pthread_mutex_t mutexSocketFileSystem; los comento porque son terreno inexplorado por ahora
sem_t semFRead;
sem_t semFWrite;
bool fRead;
bool fWrite;

//====================================================== Planificadores ========================================================
void inicializar_planificador()
{
    //creamos todas las colas que vamos a usar
    inicializar_colas();
    log_info(kernel_logger, "Iniciando colas.. \n");

    //creamos los diccionarios donde vamos a meter las distintas colas
    inicializar_diccionarios();
    log_info(kernel_logger, "Iniciando diccionarios.. \n");

    inicializar_semaforos();
    log_info(kernel_logger, "Preparando planificacion.. \n");
}

void inicializarSemaforos(){   
    grado = config_get_int_value(config, "GRADO_MULTIPROGRAMACION_INI")
    pthread_mutex_init(&mutex_ready, NULL);
    pthread_mutex_init(&mutex_exec,NULL); 

    sem_init(&gradoMultiprogramacion, 0, grado);
}

void planificador_largo_plazo()
{
    sem_wait (&gradoMultiprogramacion);
}

void planificador_corto_plazo()
{
    /*vamos a usar hilos para manejar toda la parte de ready execute y blocked porque sabemos que 
    el planificador tiene que poder hacer varias cosas a la vez*/
	pthread_create(&thread_ready, NULL, (void *)proceso_en_ready, NULL);
	pthread_create(&thread_exec, NULL, (void *)proceso_en_execute, NULL);
    //pthread_create(&thread_blocked, NULL, (void *)proceso_en_blocked, NULL);

    //y le mando esto porque despues quiero que ellos solos me borren las porquerias que hacen
	pthread_detach(thread_ready);
	pthread_detach(thread_exec);
	//pthread_detach(thread_blocked);

  log_info(kernel_logger, "Iniciando planificador de corto plazo..\n");

}


//======================================================== Estados ==================================================================
void proceso_en_ready()
{
    while(1)
    {
        log_info(kernel_logger, "estoy en proceso ready\n");

        //creamos un proceso, que va a ser el elegido por obtener_siguiente_ready
        t_pcb* siguiente_proceso = obtener_siguiente_ready();

        //lo metemos en la cola de ready y avisamos que lo metimos ahi
        meter_en_cola(siguiente_proceso,READY);
        mostrar_lista_pcb(READY);

        log_info(kernel_logger, "PID[%d] ingresando a EXEC\n", siguiente_proceso->pid);

        proceso_en_execute(siguiente_proceso);
    }
}

void proceso_en_execute(t_pcb* proceso_seleccionado)
{
    while(1)
    {
		//le enviamos el pcb a la cpu para que ejecute y recibimos el pcb resultado de su ejecucion
		enviar_pcb_a_cpu(proceso_seleccionado);
		log_info(kernel_logger, "PCB enviado cpu para ejecucion");

        /*despues la cpu nos va a devolver el contexto en caso de que haya finalizado el proceso
        haya pedido un recurso (wait/signal), por desalojo o por page fault. Ahora para probar vamos
        a hacer el caso en que lo haya devuelto por finalizacion, despues agregamos el resto*/
        char* devuelto_por = recibir_contexto(proceso_seleccionado);

        if(string_equals_ignore_case(devuelto_por, "exit")){
            proceso_en_exit(proceso_seleccionado);
		}

        //y por ultimo, en cualquiera de los casos, vamos a sacar de exec al proceso que ya termino de ejecutar
        pthread_mutex_lock(&mutex_exec);
		proceso_seleccionado = list_remove((t_list*)dictionary_int_get(diccionario_colas, EXEC), 0);
		pthread_mutex_unlock(&mutex_exec);

    }
}

void proceso_en_exit(t_pcb* proceso){
	while(1) {

	//sacamos el proceso de la lista de exit
  	pthread_mutex_lock(&mutex_exit);
  	proceso = list_remove(dictionary_int_get(diccionario_colas, EXIT), 0);
  	pthread_mutex_unlock(&mutex_exit);

  	log_info(kernel_logger, "[EXIT]Sale de EXIT y Finaliza el  PCB de ID: %d\n", proceso->pid);

    //le mandamos esto a memoria para que destruya las estructuras
	enviar_pcb_a_memoria(proceso, socket_memoria, FINALIZAR_EN_MEMORIA);
	log_info(kernel_logger, "Enviando a memoria liberar estructuras del proceso \n");
	op_code codigo = esperar_respuesta_memoria(socket_memoria);

	//si la respuesta que conseguimos de memoria es que se finalice la memoria, le avisamos a la consola que ya finaliza el proceso
	log_info(kernel_logger, "Respuesta memoria de estructuras liberadas del proceso recibida \n");
	if(codigo != FINALIZAR_EN_MEMORIA) {
		log_error(kernel_logger, "No se pudo eliminar memoria de PID[%d]\n", proceso->pid);
	}

 	eliminar_pcb(proceso);
 	free(proceso);
 	sem_post(&gradoMultiprogramacion);
	}
}

//======================================================== Algoritmos ==================================================================

/*esta funcion la voy a poner para que me haga todo el calculo de que proceso deberia ir primero dependiendo
del algoritmo que estoy usando. Y la pongo aca porque es mas facil solamente poner una linea en la parte de 
proceso_en_ready, que hacer todo este calculo alla arriba.*/
t_pcb* obtener_siguiente_ready()
{
    //creamos un proceso para seleccionar
	t_pcb* proceso_seleccionado;
	int tamanio_cola_ready = 0;
    int ejecutando = 0;
    
    /*necesito saber la cantidad de procesos que estan listos para ejecutar y para eso bloqueo sino capaz
    cuento y al final resulta que entraron 4 procesos mas*/
    pthread_mutex_lock(&mutex_ready);
	tamanio_cola_ready = dictionary_int_size(READY);
    pthread_mutex_unlock(&mutex_ready);

	//despues vemos cual seria el grado maximo pero supongamos que es esto
    int grado = config_valores_kernel.grado_multiprogramacion_ini;

    /*el grado de multiprogramacion es el que yo tengo que fijarme para saber si puedo admitir mas procesos 
    en ready, o no. Entonces para saber eso necesito saber cuantos procesos estan esperando en ready
    y cuantos procesos estan ejecutando justo ahora. Con eso puedo comparar con el grado de multi que tengo
    y ver si podemos meter un proceso mas.*/

    //quiero saber el algoritmo con el que estoy trabajando

    log_info(kernel_logger, "estoy por meterme a obtener algoritmo\n");

 	algoritmo algoritmo_elegido = obtener_algoritmo();

 	//obtenemos el tamaño de la cola de ejecutando, nuevamente pongo un semaforo
 	pthread_mutex_lock(&mutex_exec);
	ejecutando = dictionary_int_size(EXEC);
	pthread_mutex_unlock(&mutex_exec);

 	/*vemos si todavia hay procesos en ready y si el grado de multiprogramacion me permite ejecutar 
    los procesos que estoy ejecutando justo ahora con un proceso mas*/
 	if (tamanio_cola_ready > 0 && ejecutando < grado){
 		switch(algoritmo_elegido){
 		case FIFO:
 			proceso_seleccionado = obtener_siguiente_FIFO();
 			break;
 		case PRIORIDADES:
 			proceso_seleccionado = obtener_siguiente_PRIORIDADES();
 			break;
 		default:
 			break;
 		}
 	}

 	//devolvemos el proceso seleccionado segun el algoritmo que elegimos
 	return proceso_seleccionado;
}

algoritmo obtener_algoritmo(){ 

 	algoritmo switcher;
    char* algoritmo_actual = NULL;
 	algoritmo_actual = config_valores_kernel.algoritmo_planificacion;

    log_info(kernel_logger, "El algoritmo actual de planificacion es %s \n",algoritmo_actual);

 	    //FIFO
 	if (strcmp(algoritmo_actual,"FIFO") == 0)
 	{
 		switcher = FIFO;
 		//log_info(kernel_logger, "El algoritmo de planificacion elegido es FIFO \n");
 	}
 	    //PRIORIDADES
 	if (strcmp(algoritmo_actual,"PRIORIDADES") == 0)
 	{
 		switcher = PRIORIDADES;
 		log_info(kernel_logger, "El algoritmo de planificacion elegido es PRIORIDADES \n");
 	}
 	return switcher;
}

t_pcb* obtener_siguiente_FIFO()
{
    log_info(kernel_logger, "Inicio la planificacion FIFO \n");

    //mostramos los que estan en ready
 	mostrar_lista_pcb(cola_READY);

    /*voy a seleccionar el primer proceso que esta en ready usando esta funcion porque me retorna el proceso 
    que le pido y tambien me lo borra. Como FIFO va a ejecutar todo hasta terminar, me biene barbaro*/
	pthread_mutex_lock(&mutex_ready);
	t_pcb* proceso_seleccionado = list_remove(dictionary_int_get(diccionario_colas, READY), 0);
	pthread_mutex_unlock(&mutex_ready);

 	log_info(kernel_logger, "PID[%d] sale de READY por planificacion FIFO \n", proceso_seleccionado->pid);
	return proceso_seleccionado;
}

t_pcb* obtener_siguiente_PRIORIDADES()
{

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

//vamos a usar esta funcion cada vez que el proceso cambie de estado
void meter_en_cola(t_pcb* pcb, estado ESTADO)
{
	/*creamos una cola con el estado en el que esta el proceso usando la funcion int_get
    pasandole el estado del proceso (key) nos va a devolver la cola en la que esta*/
    t_list* cola = dictionary_int_get(diccionario_colas, pcb->estado_pcb);

    //recorremos la cola y buscamos el pid del pcb
    for(int i=0;i<list_size(cola) ;i++)
        {
            if(pcb->pid == ((t_pcb*) list_get(cola, i))->pid)
            {
                /*y cuando los encontramos lo vamos a sacar porque va a cambiar de estado entonces ya
                no lo quiero en esa cola*/
                list_remove(cola, i);
            }
      }

    //el estado viejo va a ser el estado original en que estaba el pcb
    estado estado_viejo = pcb->estado_pcb;

    //nuestro nuevo estado va a ser el estado al cual queremos cambiarlo
    pcb->estado_pcb= ESTADO;

    //finalmente lo agregamos a la cola de nuestro nuevo estado
    list_add(dictionary_int_get(diccionario_colas, ESTADO), pcb);
    log_info(kernel_logger, "PID: %d - Estado Anterior: %s - Estado Actual %s\n", pcb->pid,(char *)dictionary_int_get(diccionario_estados, estado_viejo),(char *)dictionary_int_get(diccionario_estados, ESTADO));
}

//esta funcion es para que nos muestre los pcb que estan en una cola, medio accesorio pero sirve
void mostrar_lista_pcb(t_list* cola){

	//creamos un string vacio llamado pid y recorremos la cola que le pasamos por parametro
	char* pids = string_new();
	  for (int i=0; i < list_size(cola);i++){

	    //creamos el string_pid donde vamos a poner el pid de cada proceso que se va leyendo de la cola
	    char* string_pid = string_itoa(((t_pcb*) list_get(cola, i))->pid);

	    //unimos cada valor almacenado en string_pid en el char de pids
	    string_append(&pids, string_pid);

	    //separamos todos los pids con una coma
	    if (i < list_size(cola) -1)
	         string_append(&pids, ", ");
	    }

	//mostramos la lista con los pids en la cola dada
	log_info(kernel_logger, " Cola %s %s : [%s]\n",(char *)dictionary_int_get(diccionario_estados, cola),config_valores_kernel.algoritmo_planificacion, pids);
	free(pids);
 }
