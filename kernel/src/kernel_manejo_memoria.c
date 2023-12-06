/*

Gestionar las peticiones de memoria para:
    -creación y eliminación de:
        --procesos y sustituciones de páginas dentro del sistema.


//Creación de Procesos

Ante la solicitud de la consola de crear un nuevo proceso el Kernel deberá informarle a la memoria:
    -crear proceso con:
        --nombre de archivo pseudocódigo
        --tamaño en bytes que ocupará el mismo.



//Eliminación de Procesos

Ante la llegada de un proceso al estado de EXIT: (solicitud de CPU o ejecución desde la consola)
    -solicitar a la memoria que libere todas las estructuras asociadas al proceso
    -marque como libre todo el espacio que este ocupaba.
    -En caso de que el proceso se encuentre ejecutando en CPU
        --enviar señal de interrupción a través de la conexión de interrupt
        --aguardar a que éste retorne el Contexto de Ejecución antes de iniciar la liberación de recursos. 


//Page Fault

En caso de que el módulo CPU devuelva un PCB desalojado por Page Fault:
    -crear un hilo específico para atender esta petición

La resolución del Page Fault:
    - Mover al proceso al estado Bloqueado 
        --Este estado será independiente de los demás porque solo afecta al proceso y no compromete recursos compartidos.
    - Solicitar al módulo memoria que se cargue en memoria principal la página correspondiente
        --la misma será obtenida desde el mensaje recibido de la CPU.
    - Esperar la respuesta del módulo memoria.
    - Al recibir la respuesta del módulo memoria:
        --desbloquear el proceso
        --colocarlo en la cola de ready.
*/

#include "kernel.h"

//============================================================================================================================================================================

//lo vamos a usar cuando tengamos que iniciar un proceso nuevo
void enviar_path_a_memoria(char* path)
{
    t_paquete* paquete = crear_paquete(RECIBIR_PATH);

    agregar_cadena_a_paquete(paquete,path);
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);

    log_info(kernel_logger, "Mandando a memoria el PATH: %s\n", path);
}

//lo vamos a usar cuando finaliza el proceso y le tenemos que decir a memoria que borre las estructuras
void enviar_pcb_a_memoria(t_pcb* proceso, int socket_memoria, op_code codigo)
{
    t_paquete* paquete = crear_paquete(codigo);

    agregar_entero_a_paquete(paquete,proceso->pid);
    
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
}

//lo vamos a usar cuando querramos saber si memoria pudo eliminar las estructuras, es solamente un aviso
op_code esperar_respuesta_memoria(int socket_memoria) {
 	op_code respuesta;

    //usamos este porque solamente quiero saber si pudo o no
 	recibir_datos(socket_memoria,&respuesta,sizeof(op_code));
 	return respuesta;
 }

void atender_page_fault(t_pcb *proceso)
{
    t_paquete* paquete = crear_paquete(SOLUCIONAR_PAGE_FAULT_MEMORIA);

    //acá no haría falta agregarle el motivo de bloqueo, medio redundante sería
    agregar_entero_a_paquete(paquete,proceso->pid);
    agregar_entero_a_paquete(paquete,proceso->pagina_pedida);

    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);

    // acá esperamos que memoria no mande el final de page fault
    int a = 0;
    recv(socket_memoria, &a,sizeof(int),0);

    log_info(kernel_logger, "Volvio del tratamiento de Page Fault, proceso:  %d", proceso -> pid);
}