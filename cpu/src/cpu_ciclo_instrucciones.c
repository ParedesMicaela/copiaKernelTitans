#include "cpu_utils.h"

/*
Fetch // Consiste en buscar la próxima instrucción a ejecutar
    -Cada instrucción pedida al módulo Memoria con el Program Counter (número de instrucción a buscar)
    -Al finalizar el ciclo, este último se le deberá sumar 1 si corresponde.
*/

t_instruccion *pcb_fetch_siguiente_instruccion(t_pcb *pcb) 
{
    //t_list *listaInstrucciones = lista de instrucciones del pcb;
    uint32_t programCounter = pcb->program_counter;

    //t_instruccion *siguienteInstruccion = list_get(listaInstrucciones, programCounter);
    //cuando tengamos la lista de instrucciones

    //return siguienteInstruccion;
}typedef struct instruccion t_instruccion;



/*
Decode
    - interpretar qué instrucción es la que se va a ejecutar
    - verificar si requiere de traducción de lógica a física.

Execute
    -Ejecutar lo correspondiente a cada instrucción:
    - // funciones en cpu_ciclo_instrucciones_utils.c

Todos los valores a leer/escribir en memoria serán numéricos enteros no signados de 4 bytes, uint32_t.

Check Interrupt
    -se deberá chequear si el Kernel nos envió una interrupción al PID que se está ejecutando,
        --en caso afirmativo
            ---se devuelve el Contexto de Ejecución actualizado al Kernel con motivo de la interrupción.
        --Caso contrario
            ---se descarta la interrupción.

Todos los casos el Contexto de Ejecución debe ser devuelto a través de la conexión de dispatch,
quedando la conexión de interrupt dedicada solamente a recibir mensajes de interrupción.

*/