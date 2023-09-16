
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

// nos van a decir la prioridad, el archivo de pseudocodigo a ejecutar y el tamanio de memoria swap que va a ejecutar
void iniciar_proceso(char *path, int tam_proceso_swap, int prioridad)
{
    log_info(kernel_logger, "Iniciando proceso.. \n");

    // nos llega de la consola interactiva que tenemos que iniciar un proceso
    // inicializamos el proceso con su pcb respectivo
    t_pcb *pcb = crear_pcb(prioridad, tam_proceso_swap);

    // necesitamos que la memoria tenga el path que nos pasaron para poder leersela al cpu
    enviar_path_a_memoria(path);

    // el new no lo tenemos en memoria

    planificar(pcb);

    // en caso de que el grado máximo de multiprogramación lo permita
    proceso_en_exit(pcb);
}
