/*

Fetch // Consiste en buscar la próxima instrucción a ejecutar
    -Cada instrucción pedida al módulo Memoria con el Program Counter (número de instrucción a buscar)
    -Al finalizar el ciclo, este último se le deberá sumar 1 si corresponde.

Decode
    - interpretar qué instrucción es la que se va a ejecutar
    - verificar si requiere de traducción de lógica a física.

Execute
    -Ejecutar lo correspondiente a cada instrucción:
    - // funciones en cpu_ciclo_instrucciones_utils.c

Cabe aclarar que todos los valores a leer/escribir en memoria serán numéricos enteros no signados de 4 bytes,
considerar el uso de uint32_t.

Check Interrupt
    -se deberá chequear si el Kernel nos envió una interrupción al PID que se está ejecutando,
        --en caso afirmativo
            ---se devuelve el Contexto de Ejecución actualizado al Kernel con motivo de la interrupción.
        --Caso contrario
            ---se descarta la interrupción.

Cabe aclarar que en todos los casos el Contexto de Ejecución debe ser devuelto a través de la conexión de dispatch,
quedando la conexión de interrupt dedicada solamente a recibir mensajes de interrupción.

*/