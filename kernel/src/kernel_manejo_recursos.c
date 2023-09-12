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