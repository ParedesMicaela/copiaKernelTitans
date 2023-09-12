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