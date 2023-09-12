/*
Logs mínimos y obligatorios


Creación de Proceso:
Se crea el proceso <PID> en NEW

Fin de Proceso:
Finaliza el proceso <PID>
Motivo: <SUCCESS / INVALID_RESOURCE / INVALID_WRITE>

Cambio de Estado:
PID: <PID>
Estado Anterior: <ESTADO_ANTERIOR>
Estado Actual: <ESTADO_ACTUAL>

Motivo de Bloqueo:
PID: <PID> 
Bloqueado por: <SLEEP / NOMBRE_RECURSO / NOMBRE_ARCHIVO>

Fin de Quantum:
PID: <PID> 
Desalojado por fin de Quantum

Ingreso a Ready:
Cola Ready <ALGORITMO>: [<LISTA DE PIDS>]

Wait:
ID:<PID> 
Wait: <NOMBRE RECURSO> 
Instancias: <INSTANCIAS RECURSO> 
Nota: El valor de las instancias es después de ejecutar el Wait

Signal: 
PID: <PID> 
Signal: <NOMBRE RECURSO> 
Instancias: <INSTANCIAS RECURSO>
Nota: El valor de las instancias es después de ejecutar el Signal

Page Fault:
Page Fault PID: <PID> 
Pagina: <Página>

Abrir Archivo:
PID: <PID> 
Abrir Archivo: <NOMBRE ARCHIVO>

Cerrar Archivo:
PID: <PID> 
Cerrar Archivo: <NOMBRE ARCHIVO>

Actualizar Puntero Archivo: 
PID: <PID> 
Actualizar puntero Archivo: <NOMBRE ARCHIVO> 
Puntero <PUNTERO>
Nota: El valor del puntero debe ser luego de ejecutar F_SEEK.

Truncar Archivo:
PID: <PID> 
Archivo: <NOMBRE ARCHIVO> 
Tamaño: <TAMAÑO>

Leer Archivo: 
PID: <PID> 
Leer Archivo: <NOMBRE ARCHIVO> 
Puntero <PUNTERO> 
Dirección Memoria <DIRECCIÓN MEMORIA> 
Tamaño <TAMAÑO>

Escribir Archivo:
PID: <PID> 
Escribir Archivo: <NOMBRE ARCHIVO> 
Puntero <PUNTERO> 
Dirección Memoria <DIRECCIÓN MEMORIA> 
Tamaño <TAMAÑO>

Proceso de detección de deadlock:
ANÁLISIS DE DETECCIÓN DE DEADLOCK

Detección de deadlock (por cada proceso dentro del deadlock):
Deadlock detectado: <PID> 
Recursos en posesión <RECURSO_1>, <RECURSO_2>, <RECURSO_N> 
Recurso requerido: <RECURSO_REQUERIDO>

Pausa planificación:
PAUSA DE PLANIFICACIÓN

Inicio de planificación: 
INICIO DE PLANIFICACIÓN

*/