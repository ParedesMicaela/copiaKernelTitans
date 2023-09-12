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