/*
ni idea, todavia no creo que sea de importancia esto

Detección y resolución de Deadlocks
Frente a acciones donde los procesos lleguen al estado Block (por recursos o archivos)
se debe realizar una detección automática de deadlock (proceso bloqueados entre sí).
En caso que se detecte afirmativamente un deadlock se debe informar el mismo por consola.
En dicho mensaje se debe informar cuales son los procesos (PID) que se encuentran en deadlocks,
cuales son los recursos (o archivos) tomados y cuales son los recursos (o archivos) requeridos
(por los que fueron bloqueados).
La resolución de deadlocks se realizará de forma manual por la consola del Kernel utilizando el
mensaje “Finalizar proceso” donde el proceso finalizado deberá liberar los recursos (o archivos) tomados.
Una vez realizada esta acción se debe volver a realizar una nueva detección de deadlock.


*/