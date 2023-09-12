
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

#include "kernel.h"

//===============================================================================================================================

//nos van a decir la prioridad, el archivo de pseudocodigo a ejecutar y el tamanio de memoria swap que va a ejecutar
void iniciar_proceso (char* path, uint32_t tam_proceso_swap, int prioridad)
{
  //nos llega de la consola interactiva que tenemos que iniciar un proceso
  //inicializamos el proceso con su pcb respectivo
  t_pcb* pcb = crear_pcb(prioridad,tam_proceso_swap);

  //necesitamos que la memoria tenga el path que nos pasaron para poder leersela al cpu
  enviar_path_a_memoria(path);

  //en caso de que el grado máximo de multiprogramación lo permita
  planificador_largo_plazo();

  while(1)
  {
    planificador_corto_plazo();
  }

  //una que vez que ejecutamos, lo mandamos a exit
  meter_en_cola(pcb, EXIT);

  //cuando el proceso finalice tenemos que liberar el espacio que le dimos en memoria    
  t_paquete* paquete_para_memoria = crear_paquete(FINALIZAR_EN_MEMORIA);

  //solamente le pasamos el pid del proceso porque en memoria vamos a tener toda otra estructura con las cosas que ocupa el proceso
  //entonces con solo el pid podriamos acceder a este
  agregar_entero_a_paquete(paquete_para_memoria,pcb->pid);
  enviar_paquete(paquete_para_memoria, socket_memoria);
    
  log_info(kernel_logger, "Se manda mensaje a memoria para liberar estructuras del proceso: %d", pcb->pid);

  }
