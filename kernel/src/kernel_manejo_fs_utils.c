#include "kernel.h"

bool es_una_operacion_con_archivos(char* motivo_bloqueo) {
        string_equals_ignore_case(motivo_bloqueo, "F_WRITE") 
        || string_equals_ignore_case(motivo_bloqueo, "F_TRUNCATE")
        || string_equals_ignore_case(motivo_bloqueo, "F_READ");
}

/*

capaz es medio alpedo este archivo... lo dejo por las dudas
F_OPEN: abrir o crear el archivo pasado por parámetro
    -Modo Lectura
        --se deberá validar si existe un lock de escritura activo
                ---En caso de existir
                    ----se debe bloquear el proceso
                    ----esperar a que finalice dicha operación.
                ---En caso de no existir
                    ----Ya existe un lock de lectura en curso, entonces
                        -----agregar proceso como "participante" de dicho lock
                        -----el mismo finalizará cuando se ejecuten todos los F_CLOSE de los participantes.
                    ----Que no exista un lock de lectura
                        -----crear dicho lock como único participante.
    -Modo Escritura
        --crear lock de escritura exclusivo para el proceso
            --Si ya existe un lock activo
                ---se bloqueará el proceso encolando dicho lock.


F_CLOSE: Esta función será la encargada de cerrar el archivo pasado por parámetro.
    -En caso que dicho archivo se encuentre abierto en Modo Lectura
        --reducirá la cantidad de participantes en uno del lock:
            --reducir en 1 la cantidad
            --en caso que la cantidad llegue a 0
                ---se cerrará dicho lock dando lugar a que otro se active en caso que haya encolados
    -Si dicho archivo se encuentra abierto en Modo Escritura
        --se liberará el lock actual dando lugar al siguiente lock en la cola.


F_SEEK: Actualiza puntero de archivo en tabla_archivos_abiertos de proceso hacia ubicación de parámetro.
    -Se deberá devolver el contexto de ejecución a la CPU para que continúe el mismo proceso.


F_TRUNCATE: Solicita a File System:
    - acutualizar tamaño de archivo a nuevo pasado por parámetro
    - bloquear proceso hasta que el File System informe de la finalización de la operación.


F_READ: Se solicita al módulo File System:
    -que lea desde el puntero del archivo pasado por parámetro la cantidad de bytes indicada
    -que grabe eso en la dirección física de memoria recibida por parámetro.
    
    -El proceso que llamó a F_READ deberá permanecer en estado bloqueado hasta que:
        --File System informe de la finalización de la operación.


F_WRITE: En caso de que el proceso haya solicitado un lock de escritura
    -Solicitará a fs que escriba desde dirección fisica recibida por parámetro:
        --la cantidad de bytes indicada.

    -El proceso que llamó a F_WRITE, deberá permanecer en estado bloqueado hasta que:
        --el módulo File System informe de la finalización de la operación. 
    
    -En caso de que el proceso haya solicitado un lock de lectura:
        --se deberá cancelar la operación y enviar el proceso a EXIT con motivo de INVALID_WRITE.


Nota
F_READ y F_WRITE siempre pasan tamaños y punteros válidos.
No se van a ingresar operaciones de File System sin haber abierto el archivo previamente con F_OPEN.

*/