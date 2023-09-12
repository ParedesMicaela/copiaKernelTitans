/*

SET (Registro, Valor)
Asigna al registro el valor pasado como parámetro.

SUM (Registro Destino, Registro Origen)
Suma ambos registros
Deja el resultado en el Registro Destino.

SUB (Registro Destino, Registro Origen)
Resta al Registro Destino el Registro Origen
Deja el resultado en el Registro Destino.

JNZ (Registro, Instrucción)
Si el valor del registro es distinto de cero
    actualiza el program counter al número de instrucción pasada por parámetro.

SLEEP (Tiempo)
Esta instrucción representa una syscall bloqueante.
Se deberá devolver
    -el Contexto de Ejecución actualizado al Kernel
    -la cantidad de segundos que va a bloquearse el proceso.

WAIT (Recurso)
soicita a kernel Asignar una instancia del recurso indicado por parámetro.

SIGNAL (Recurso)
solicitar a kernel Liberar una instancia del recurso indicado por parámetro.

MOV_IN (Registro, Dirección Lógica)
Lee el valor de memoria correspondiente a la Dirección Lógica y lo almacena en el Registro.

MOV_OUT (Dirección Lógica, Registro)
Lee el valor del Registro
lo escribe en la dirección física de memoria obtenida a partir de la Dirección Lógica.

F_OPEN (Nombre Archivo, Modo Apertura)
Solicita al kernel que abra el archivo pasado
por parámetro con el modo de apertura indicado.

F_CLOSE (Nombre Archivo)
Esta instrucción solicita al kernel que cierre el archivo pasado por parámetro.

F_SEEK (Nombre Archivo, Posición)
Esta instrucción solicita al kernel actualizar el puntero del archivo
a la posición pasada por parámetro.

F_READ (Nombre Archivo, Dirección Lógica): Esta instrucción solicita al Kernel que se lea del archivo
indicado y se escriba en la dirección física de Memoria la información leída.

F_WRITE (Nombre Archivo, Dirección Lógica): Esta instrucción solicita al Kernel que se escriba en el archivo
indicado la información que es obtenida a partir de la dirección física de Memoria.

F_TRUNCATE (Nombre Archivo, Tamaño): Esta instrucción solicita al Kernel que se modifique el tamaño del
archivo al indicado por parámetro.

EXIT: Esta instrucción representa la syscall de finalización del proceso. Se deberá devolver el Contexto 
de Ejecución actualizado al Kernel para su finalización.

*/